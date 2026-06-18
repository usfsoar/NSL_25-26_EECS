# Main file to be run on startup for launches

import bmp
import bno
import pid
import rocket_data
import time
import state_machine
import motor_control
import datetime
import math
import os
import sys
import traceback

class NoneError(Exception):
    pass

REBOOT_LOG = "reboot_count.txt"
REBOOT_LIMIT = 3

start_time = time.time() # start time of program
elapsed_time = time.time() - start_time # time passed since start of program

try:
    # Initialize bmp
    bmpsensor = bmp.BMP()
    # Initialize bno
    bnosensor = bno.BNO()
    # Initialize servo controller
    motor = motor_control.Motor()
    # Create State Machine object
    states = state_machine.StateMachine()
    bmpsensor.set_sea_level()
    # Create PID object
    mypid = pid.PID(0.1,0.01,0.01)
    # Create Rocket Data object
    data = rocket_data.RocketData(
        'airbrakes' + str(datetime.datetime.now().strftime("%Y-%b-%d-%H-%M-%S")) + 
        '.csv', ['Time', 'Altitude', 'Velocity', 'Acceleration X', 'Acceleration Y',
        'Acceleration Z', 'State', 'Steps', 'Predicted Apogee', 'Error', 'Pressure', 'Temp', 'Density', 'Current Step']
    )
    data.createFile()

    with open(REBOOT_LOG, "w") as f:
        f.write("0")
    
except Exception as e:
    traceback.print_exc()

    log_name = './ErrorLogs/ab_error_' + str(datetime.datetime.now().strftime("%Y-%b-%d-%H-%M-%S")) + '.txt'

    with open(log_name,'w') as f:
        f.write(f"Restart triggered by error: {traceback.format_exc()}")
    
    if os.path.exists(REBOOT_LOG):
        with open(REBOOT_LOG, "r") as f:
            try:
                count = int(f.read().strip())
            except ValueError:
                count = 0
    else:
        count = 0

    if count >= REBOOT_LIMIT:
        print(f"CRITICAL: Reboot limit of {REBOOT_LIMIT} reached. Manual intervention required.")
        # We exit here instead of rebooting to break the loop
        sys.exit(1) 

    with open(REBOOT_LOG, "w") as f:
        f.write(str(count + 1))
    
    time.sleep(5)
    os.system('sudo reboot')
    sys.exit(0)

# Initial values

target_apogee = 3048 # Our target max height in meters
dt = 0.1 # time between iterations
extra_dt = 0.1 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height
acceleration = None
quat = None
previous_time = time.time()
accelerationX, accelerationY, accelerationZ = 0.0, 0.0, 0.0

previous_altitude = bmpsensor.altitude()
if previous_altitude is None:

    previous_altitude = 0
steps = 0
accelerationX, accelerationY, accelerationZ = None, None, None
predicted_apogee = 0
error = 0
pressure = 1013.25
temperature = 15.0
state = 0

# --- COMPLEMENTARY FILTER VARIABLES ---
ALPHA = 0.96  
estimated_velocity = 0.0
prev_filter_alt = None
prev_filter_time = None

def get_velocity(current_altitude, accel_xyz, current_time):
    global estimated_velocity, prev_filter_alt, prev_filter_time
    
    if prev_filter_alt is None or prev_filter_time is None:
        prev_filter_alt = current_altitude
        prev_filter_time = current_time
        return 0.0

    dt = current_time - prev_filter_time
    if dt <= 0.001:  
        return estimated_velocity

    # Default vertical acceleration to 0.0 if sensors are missing data
    vertical_acceleration = 0.0

    # Calculate True Earth-Frame Vertical Acceleration using the Quaternion
    if accel_xyz is not None:
        vertical_acceleration = accel_xyz[2]

    # 1. Barometer Velocity
    baro_velocity = (current_altitude - prev_filter_alt) / dt

    # 2. Accelerometer Velocity Prediction (using true vertical acceleration)
    accel_velocity = estimated_velocity + (vertical_acceleration * dt)

    # 3. Sensor Fusion Step
    estimated_velocity = (ALPHA * accel_velocity) + ((1.0 - ALPHA) * baro_velocity)

    prev_filter_alt = current_altitude
    prev_filter_time = current_time

    return estimated_velocity

while True:
    # Update dt
    curr_time = time.time()
    dt = curr_time - previous_time
    dt = max(dt, .001)

    elapsed_time = curr_time - start_time
    # Get sensor data
    try:
        altitude = bmpsensor.altitude()
        if altitude is None:
            raise NoneError("Altitude is None!")
        
    except Exception as e:
        print(e)
        time.sleep(extra_dt)
        continue
    
    if altitude > apogee:
        apogee = altitude

     

    try:
        acceleration = bnosensor.linear_acceleration()
        quat = bnosensor.quaternion()  # <-- Grab orientation frame
        if acceleration is None or None in acceleration or quat is None or None in quat:
            raise NoneError("IMU data incomplete!")
        accelerationX, accelerationY, accelerationZ = acceleration
    except Exception as e:
        print(e)
        acceleration = None  # Explicitly pass None so the filter handles it safely
        quat = None
        time.sleep(extra_dt)
        continue

    velocity =  get_velocity(altitude, acceleration, curr_time)
    try:
        pressure = bmpsensor.pressure()
    except:
        pass
    try:
        temperature = bmpsensor.temperature()
    except:
        pass
    try:
        air_density = (pressure*100) / (287.058 * (temperature+273.15))
    except:
        air_density = 1
    # Update State Machine
    try:
        state = states.transition(altitude, velocity, accelerationZ, apogee)
    except Exception as e:
        print(f"Error determining state: {e}")
    try:
        match state:
            # If motor is burning or rocket is descending -> close airbrakes
            case 1 | 4 | 0:
                motor.move_to(0)
            # When in active state -> run PID
            case 2:
                mypid.update(altitude, velocity, accelerationZ, dt)
                error = mypid.error()
                pid_output = mypid.pidSum()
                predicted_apogee = mypid.projHeight
                mypid.motorInput(pid_output)
                motor.move_to(int(mypid.currStep))
                steps = int(mypid.currStep)
            # If rocket has passed target -> fully open airbrakes
            case 3:
                motor.move_to(motor.max_step)
                print("State 3")
            # When rocket has landed -> break loop
            case 5:
                break
            case _:
                motor.move_to(0)
    except Exception as e:
        print(f"Error controlling airbrakes: {e}")
    
       
    try:   
        # Save all data to CSV file
        samuel_johnson = {
            'Time' : [round(elapsed_time,4)],
            'Altitude' : [round(altitude,4)],
            'Velocity' : [round(velocity,4)],
            'Acceleration X': [round(accelerationX,4)],
            'Acceleration Y': [round(accelerationY,4)],
            'Acceleration Z': [round(accelerationZ,4)],
            'State' : [round(state,4)],
            'Steps' : [round(steps,4)],
            'Predicted Apogee' : [round(predicted_apogee,4)],
            'Error' : [round(error,4)],
            'Pressure': [round(pressure,4)],
            'Temp' : [round(temperature,4)],
            'Air Density' : [round(air_density,4)],
            'Current Step' : [round(motor.current_pos)]
        }
        data.writeFile(samuel_johnson)
        print(f'Time: {elapsed_time}, Altitude: {altitude}, Velocity: {velocity}, Acceleration: {accelerationZ}, State: {state}, Steps: {motor.current_pos}')
    except Exception as e:
        print(f"Error logging data: {e}")
    
    # Delay before next iteration
    time.sleep(extra_dt)
    # Update dt
    previous_time = curr_time
    
print("Landed! Creating Plots of Airbrakes Data!")
# Create graphs and save to SD card
try:
    data.masterPlot()
except:
    print("Failed to create plots!")
    print("Time for excel!")
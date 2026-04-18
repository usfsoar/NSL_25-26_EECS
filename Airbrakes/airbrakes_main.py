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
    mypid = pid.PID(0.1,0.01,0.01,bmpsensor.pressure(), bmpsensor.temperature())
    # Create Rocket Data object
    data = rocket_data.RocketData(
        'airbrakes' + str(datetime.datetime.now().strftime("%Y-%b-%d-%H-%M-%S")) + 
        '.csv', ['Time', 'Altitude', 'Velocity', 'Acceleration X', 'Acceleration Y',
        'Acceleration Z', 'State', 'Steps', 'Predicted Apogee', 'Error', 'Pressure', 'Temp', 'Density', 'Current Step']
    )
    data.createFile()
    raise ValueError
    
except Exception as e:
    traceback.print_exc()

    log_name = './ErrorLogs/ab_error_' + str(datetime.datetime.now().strftime("%Y-%b-%d-%H-%M-%S")) + '.txt'

    with open(log_name,'w') as f:
        f.write(f"Restart triggered by error: {traceback.format_exc()}")
    with open(REBOOT_LOG, "w") as f:
        f.write("0")
    
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

with open(REBOOT_LOG, "w") as f:
        f.write("0")
# Initial values

target_apogee = 1.5 # Our target max height in meters
dt = 0.1 # time between iterations
extra_dt = 0.1 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height
previous_altitude = 0
steps = 0
accelerationX, accelerationY, accelerationZ = None, None, None
predicted_apogee = 0
error = 0



while True:
    # Update dt
    elapsed_time = time.time() - start_time
    # Get sensor data
    try:
        altitude = bmpsensor.altitude()
        if not altitude:
            raise NoneError("Altitude is None!")
        
    except Exception as e:
        print(e)
        time.sleep(extra_dt)
        dt = dt + time.time() - start_time - elapsed_time
        continue
    velocity = (altitude - previous_altitude) / dt
   

    try:
        acceleration = bnosensor.linear_acceleration()
        if None in acceleration:
            raise NoneError("Acceleration is None!")
        accelerationX, accelerationY, accelerationZ = acceleration
    except Exception as e:
        print(e)
        time.sleep(extra_dt)
        dt = dt + time.time() - start_time - elapsed_time
        continue
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
            case 1 | 4:
                motor.move_to(0)
            # When in active state -> run PID
            case 2:
                mypid.update(elapsed_time, altitude, velocity, accelerationZ, air_density, dt)
                error = mypid.error(steps)
                pid_output = mypid.pidSum()
                predicted_apogee = mypid.projHeight
                steps = int(mypid.motorInput(pid_output))
                motor.move_to(motor.max_step)
            # If rocket has passed target -> fully open airbrakes
            case 3:
                motor.move_to(motor.max_step)
                print("State 3")
            # When rocket has landed -> break loop
            case 5:
                break
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
    # Update values
    previous_altitude = altitude
    
    # Delay before next iteration
    time.sleep(extra_dt)
    # Update dt
    dt = time.time() - start_time - elapsed_time
    
print("Landed! Creating Plots of Airbrakes Data!")
# Create graphs and save to SD card
try:
    data.masterPlot()
except:
    print("Failed to create plots!")
    print("Time for excel!")

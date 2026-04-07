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
class NoneError(Exception):
    pass


start_time = time.time() # start time of program
elapsed_time = time.time() - start_time # time passed since start of program

# Initialize bmp
bmpsensor = bmp.BMP()
# Initialize bno
bnosensor = bno.BNO()
# Initialize servo controller
motor = motor_control.Motor()
# Create State Machine object
states = state_machine.StateMachine()
# Create PID object
mypid = pid.PID(0.1,0.01,0.01)
# Create Rocket Data object
data = rocket_data.RocketData(
    'airbrakes' + str(datetime.datetime.now().strftime("%Y-%b-%d-%H-%M-%S")) + 
    '.csv', ['Time', 'Altitude', 'Velocity', 'Acceleration X', 'Acceleration Y',
    'Acceleration Z', 'State', 'Steps', 'Predicted Apogee', 'Error']
)
data.createFile()

# Initial values
target_apogee = 1.5 # Our target max height in meters
dt = 0.1 # time between iterations
extra_dt = 0.05 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height
previous_altitude = 0
steps = 0
accelerationX, accelerationY, accelerationZ = None, None, None
predicted_apogee = 0
error = 0

bmpsensor.set_sea_level()

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
    # pressure = bmpsensor.pressure()
    # temperature = bmpsensor.temperature()
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

    # Update State Machine
    state = states.transition(altitude, velocity, accelerationZ, apogee)
    
    match state:
        # If motor is burning or rocket is descending -> close airbrakes
        case 1 | 4:
            motor.move_to(0)
        # When in active state -> run PID
        case 2:
            mypid.update(elapsed_time, altitude, velocity, accelerationZ, dt)
            error = mypid.error()
            pid_output = mypid.pidSum()
            predicted_apogee = mypid.projHeight
            steps = int(mypid.motorInput(pid_output))
            motor.move_to(steps)
        # If rocket has passed target -> fully open airbrakes
        case 3:
            # motor.move_to(motor.max_step)
            print("State 3")
        # When rocket has landed -> break loop
        case 5:
            break

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
        'Error' : [round(error,4)]
    }
    data.writeFile(samuel_johnson)
    print(f'Time: {elapsed_time}, Altitude: {altitude}, Velocity: {velocity}, Acceleration: {accelerationZ}, State: {state}')
    # Update values
    previous_altitude = altitude
    
    # Delay before next iteration
    time.sleep(extra_dt)
    # Update dt
    dt = time.time() - start_time - elapsed_time
    

# Create graphs and save to SD card
data.masterPlot()
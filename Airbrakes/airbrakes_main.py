# Main file to be run on startup for launches

import bmp
import bno
import pid
import rocket_data
import time
import state_machine
import motor_control
import datetime
class NoneError(Exception):
    pass

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
target_apogee = 3048 # Our target max height in meters
dt = 0.1 # time between iterations
extra_dt = 0.05 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height
previous_altitude = 0

start_time = time.time() # start time of program
elapsed_time = time.time() - start_time # time passed since start of program

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
    states = state_machine.transition(altitude, velocity, accelerationZ, apogee)
    
    match states:
        # If motor is burning or rocket is descending -> close airbrakes
        case 1 | 4:
            motor.moveToStep(0)
        # When in active state -> run PID
        case 2:
            mypid.update(elapsed_time, altitude, velocity, accelerationZ)
            error = mypid.error()
            pid_output = mypid.pidSum()
            predicted_apogee = mypid.projHeight
            steps = mypid.motorInput(pid_output)
            motor.moveToStep(0)
        # If rocket has passed target -> fully open airbrakes
        case 3:
            motor.moveToStep(motor.max_step)
        # When rocket has landed -> break loop
        case 5:
            break

    # Save all data to CSV file
    samuel_johnson = {
        'Time' : [elapsed_time],
        'Altitude' : [altitude],
        'Velocity' : [velocity],
        'Acceleration X': [accelerationX],
        'Acceleration Y': [accelerationY],
        'Acceleration Z': [accelerationZ],
        'State' : [states],
        'Steps' : [steps],
        'Predicted Apogee' : [predicted_apogee],
        'Error' : [error]
    }
    data.writeFile(samuel_johnson)
    # Update values
    previous_altitude = altitude
    
    # Delay before next iteration
    time.sleep(extra_dt)
    # Update dt
    dt = time.time() - start_time - elapsed_time
    

# Create graphs and save to SD card
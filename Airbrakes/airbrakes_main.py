# Main file to be run on startup for launches

import bmp
import bno
import pid
import rocket_data
import time
import state_machine
import servo_control

# Initialize bmp
bmpsensor = bmp.BMP()
# Initialize bno
bnosensor = bno.BNO()
# Initialize servo controller
servo = servo_control.Servo()
# Create State Machine object
states = state_machine.StateMachine()
# Create PID object
mypid = pid.PID()
# Create Rocket Data object
data = rocket_data.RocketData("flight.csv", ["Time", "Altitude", "Velocity", 
                                             "Acceleration X", "Acceleration Y", "Acceleration Z",
                                             "State", "Servo Angle", "Predicted Apogee"])
data.createFile()

# Initial values
target_apogee = 1219.20 # Our target max height in meters
dt = 0 # time between iterations
extra_dt = 0.05 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height
previous_altitude = 0

start_time = time.time() #
elapsed_time = time.time() - start_time

while True:
    # Update dt
    dt = time.time() - start_time - elapsed_time
    elapsed_time = time.time() - start_time
    # Get sensor data
    altitude = bmpsensor.altitude()
    velocity = (altitude - previous_altitude) / dt
    # pressure = bmpsensor.pressure()
    # temperature = bmpsensor.temperature()
    acceleration = bnosensor.acceleration()
    states = state_machine.transition(altitude, velocity, acceleration)
    
    # Update State Machine

    # If in state 1
        # Keep airbrakes closed

    # If in state 2
        # Run PID
        # Actuate Servo
    
    # If in state 3
        # Fully actuate servo

    # if in state 4
        # Keep airbrakes closed

    # if in state 5
        # break loop

    # Save all data to CSV file
    # angle = PID something or other
    # predicted_apogee = ^
    stuff = {
        'Time' : [elapsed_time],
        'Altitude' : [altitude],
        'Velocity' : [velocity],
        'Acceleration X': [acceleration[0]],
        'Acceleration Y': [acceleration[1]],
        'State' : [states],
        'Servo Angle' : [],
        'Predicted Apogee' : []
    }
    data.writeFile(stuff)
    # Update values
    previous_altitude = altitude
    
    # Delay before next iteration
    

# Create graphs and save to SD card
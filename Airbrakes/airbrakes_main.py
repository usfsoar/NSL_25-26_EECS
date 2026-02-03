# Main file to be run on startup for launches

import bmp
import bno
import pid
import rocket_data
import time
import state_machine

# Initialize bmp

# Initialize bno

# Initialize servo controller

# Create State Machine object

# Create PID object

# Create Rocket Data object

# Initial values
target_apogee = 1219.20 # Our target max height in meters
dt = 0 # time between iterations
extra_dt = 0.05 # added time between iterations to remove errors in velocity
apogee = 0 # our current max height

start_time = time.time() #
elapsed_time = time.time() - start_time

while True:
    # Update dt
    dt = time.time() - start_time - elapsed_time
    elapsed_time = time.time() - start_time
    # Get sensor data


    data = {
        'Time' : [elapsed_time],
        'Altitude' : [],
        'Velocity' : [],
        'Acceleration X' : [],
        'Acceleration Y' : [],
        'Acceleration Z' : [],
        'State' : [],
        'Motor Steps' : [],
        'Predicted Apogee' : []
    }
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

    # Update values

    # Delay before next iteration
    pass

# Create graphs and save to SD card
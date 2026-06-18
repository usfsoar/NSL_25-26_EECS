#!/usr/bin/env python3

"""
TO DO:
add motor dimensions from cad

add p control for distance control
-see if while loop will mess up multithreading

add euler angle calculation for landing orientation and rover turning

ndvi calculation and plant classification

add kalman filter
"""

#----IMPORTS----    
#import libraries
import os
import csv
import time
import multiprocessing as mp
import multiprocessing.shared_memory as shared_memory
import numpy as np
import subprocess

#import classes
from payload_pipeline.state_machine import StateMachine
from payload_pipeline.telemetry_logger import TelemetryLogger

from payload_sensor.bmp580 import BMP
from payload_sensor.sensor_simulation import Sensor_Data_Simulator
from payload_sensor.servo import ServoControl

from payload_rover.rover_main import startRoverProcess, startAIProcess, startPlantProcess, startSensorProcess

#----GLOBAL VARIABLES----
#mode: launch, drop, hand, sim
MODE = "hand"

#three type of thresholds: hand, sim, and launch
#hand thresholds
if MODE == "launch" or MODE == "sim":
    LAUNCH_GFORCE_THRESHOLD     = 2.0   #G  gs needed to launch
    LAUNCH_ALTITUDE_THRESHOLD   = 3000.0  #m  height needed to launch
    DESCENT_ALTITUDE_THRESHOLD  = 10.0  #m  below apogee to call descent
    DESCENT_APOGEE_THRESHOLD    = 500.0 #m  minimum apogee to care
    LANDING_GFORCE_THRESHOLD    = 0.8   #G  gs needed to call landing
    LANDING_VEL_THRESHOLD       = 0.3  #m/s velocity needed to call landing
    LANDING_ALTITUDE_THRESHOLD  = 50  #m height needed to call landing
    MIN_LANDING_TIME = 180
elif MODE == "drop":
    LAUNCH_GFORCE_THRESHOLD     = 1.5   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 0.75   #m
    DESCENT_ALTITUDE_THRESHOLD  = .1   #m
    DESCENT_APOGEE_THRESHOLD    = 1.5   #m
    LANDING_GFORCE_THRESHOLD    = 0.2   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = -1.0   #m
    MIN_LANDING_TIME = 60
elif MODE == "hand":
    LAUNCH_GFORCE_THRESHOLD     = 1.3   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 0.1   #m
    DESCENT_ALTITUDE_THRESHOLD  = 0.2   #m
    DESCENT_APOGEE_THRESHOLD    = 0.5   #m
    LANDING_GFORCE_THRESHOLD    = 0.2   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = 0.5   #m
    MIN_LANDING_TIME = 10
else:
    exit("Invalid MODE selected")

#ema constants
ALPHA_ALT = 0.1 # Calculate by hand before go/no go
ALPHA_PRESSURE = 0.8

#state transition evaluation constants
STABLE_READINGS = 10
STABLE_READINGS_FOR_LANDING = 20

#timeout constants
POWER_CYCLE_TIME = 45 # seconds
FLIGHT_TIMEOUT = 15 * 60
ROVER_SCAN_TIMEOUT = 900
if MODE == "sim":
    ROVER_SCAN_TIMEOUT = 60
ROVER_EXIT_TIMEOUT = 60

#rover dimensions
WHEEL_RADIUS = 0
WHEEL_BASE = 0
WHEEL_CIRCUM = 2 * 3.14 * WHEEL_RADIUS

AI_MODEL_PATH = "model.rpk"

#data storage
data = {
    "timestamp": 0,
    "state": "READY",
    "raw_altitude": 0,
    "altitude": 0,
    "velocity": 0,
    "apogee": 0,
    "start_pressure": 0,
    }



#----INITIALIZE CLASSES----
if MODE == "sim":
    bmp = None
    sim = Sensor_Data_Simulator()
    servo = None
else:
    bmp = BMP()
    sim = None
    servo = ServoControl()
    
sm = StateMachine(
    LAUNCH_GFORCE_THRESHOLD,
    LAUNCH_ALTITUDE_THRESHOLD,
    DESCENT_APOGEE_THRESHOLD,
    DESCENT_ALTITUDE_THRESHOLD,
    LANDING_VEL_THRESHOLD,
    LANDING_GFORCE_THRESHOLD,
    LANDING_ALTITUDE_THRESHOLD,
    STABLE_READINGS,
    STABLE_READINGS_FOR_LANDING,
    MIN_LANDING_TIME,
    FLIGHT_TIMEOUT
)

# Log variable
log = None

def check_power_loss():
    if os.path.exists(".running.txt"):
        # Check if running file is there
        # If it is, then we lost power
        with open(".running.txt", "r") as file:
            log_file = file.readline()

        # Overwrite file with new logging path
        with open(".running.txt", "w") as file:
            file.write(log.LOGGING_FILE_PATH)
            file.flush()
            os.fsync(file.fileno())

        if os.path.exists(log_file):
            return (True, log_file)
        else:
            return (False, "")
    else:
        # If not, we are starting from blank, then make a new running file
        with open(".running.txt", "w") as file:
            file.write(log.LOGGING_FILE_PATH)
            file.flush()
            os.fsync(file.fileno())
        
        return (False, "")


def power_loss_recovery():
    powerLoss, logFile = check_power_loss()

    if powerLoss:
        with open(logFile, newline='') as csvfile:
            rows = csv.reader(csvfile)
            mappings = next(rows)
            state_index = mappings.index('state')

        with open(logFile, "rb") as csvfile:
            # Get start time:
            start_time = None

            pos = csvfile.tell()
            while csvfile.read(1) != b'\n':
                pos += 1
                csvfile.seek(pos)
            pos += 1
            csvfile.seek(pos)
            first_row = csvfile.readline().decode()
            for col in csv.reader([first_row]):
                try:
                    data["timestamp"] = col[0] # Only getting previous start time
                except Exception as e:
                    print(f"Failed to recovery power: {e}")
                    return
                start_time = TelemetryLogger.string_to_sec(data["timestamp"])
            
            csvfile.seek(0, os.SEEK_END)
            pos = csvfile.tell() - 1

            newline_count = 0
            while pos > 0:
                csvfile.seek(pos)
                if csvfile.read(1) == b"\n":
                    newline_count += 1
                    if newline_count >= 2:
                        break
                pos -= 1

            last_data = csvfile.readline().decode() # Last row of complete data
        
            for row in csv.reader([last_data]):
                last_time = TelemetryLogger.string_to_sec(col[0])
                current_time = TelemetryLogger.string_to_sec(TelemetryLogger.get_timestamp())
                offset = current_time - last_time - POWER_CYCLE_TIME
                start_time += offset

                data["timestamp"] = TelemetryLogger.sec_to_string(start_time)
                print(f"Starting time: {data["timestamp"]}")

                print(f"Recovering from {logFile}")
                data['state'] = row[state_index]
                data["raw_altitude"] = float(row[2])
                data["altitude"] = float(row[3])
                data["velocity"] = float(row[4])
                data["apogee"] = float(row[5])
                data["start_pressure"] = float(col[6])
                print(f"Recovery values: {data}")
        

        # Recovery state machine with relative start time
        sm.recover(data["timestamp"])
        log.log_sensor(data=data)

    return powerLoss


#----FUNCTIONS----
def initialize_sensors():
    if MODE == "sim":
        pass
    else:
        try:
            bmp.initialize(ALPHA_ALT, ALPHA_PRESSURE)
        except Exception as error:
            print(error)

        try:
            servo.initialize(16, -45, 45)
        except Exception as error:
            print(error)
        

def validate_data():
    #need to figure out what to do if data is None
    #probably where kalman filter goes
    if data["altitude"] is None:
        data["altitude"] = 0
    if data["velocity"] is None:
        data["velocity"] = 0

def get_sensor_data():
    data["timestamp"] = TelemetryLogger.get_timestamp()

    if MODE == "sim":
        sim.updateValues()
        data["raw_altitude"] = data["altitude"] = sim.getAlt()
        data["raw_velocity"] = data["velocity"] = sim.getVelocity()
        data["apogee"] = max(data["apogee"], data["altitude"])
    else:
        data["velocity"], data["raw_altitude"], data["altitude"] = bmp.get_vertical_velocity()
        data["apogee"] = max(data["apogee"], data["altitude"])

def set_zero_altitude(power_loss):
    _, prev = bmp.get_pressure()
    while True:
        _, curr = bmp.get_pressure()
        if abs(curr - prev) < 0.5:
            break
        prev = curr

    if not power_loss:
        _, data["start_pressure"] = bmp.get_pressure()

    bmp.set_sea_level_pressure(data["start_pressure"])


def main():
    global log
    log = TelemetryLogger(sensor_data=data)

    initialize_sensors()

    power_loss = power_loss_recovery()
    if MODE != "sim":
        set_zero_altitude(power_loss)
        
    if MODE != "sim":
            servo.lock()

    while data["state"] != "LANDING":
        # start_loop = time.perf_counter()
        get_sensor_data()
        validate_data()
        data["state"] = sm.update(
            data["timestamp"],
            data["state"],
            data["altitude"],
            data["velocity"],
            data["apogee"]
        )

        # Log data to file: (Time, Current State, G-Force, Altitute, Velocity, Apogee)
        log.log_sensor(data=data)
        

        # loop_time = time.perf_counter() - start_loop
    #end of loop

    # TODO** Check if reached due to power recovery
        # Handle issues if needed

    # Presumably not power recovered
    if MODE == "launch":
        time.sleep(250) # Wait the full parachute descent time
    elif MODE == "hand":
        time.sleep(20)

    #Servo Retract
    if MODE != "sim":
        servo.retract()
    time.sleep(5)

    sensor_shm = mp.shared_memory.SharedMemory(create=True, 
        size=np.zeros(1, dtype=[('velocity', np.float64), ('lin_accel', np.float64), ('temperature', np.float64), ('current', np.float64), ('distance', np.float64)]).nbytes)

    # Start processes
    timeout_time = time.time() + ROVER_SCAN_TIMEOUT
    processes = list()
    processes.append(startRoverProcess((None, timeout_time, sensor_shm.name))) # rover
    processes.append(startAIProcess((None, timeout_time, AI_MODEL_PATH))) # ai cam
    processes.append(startSensorProcess((timeout_time, sensor_shm.name))) # sensor process

    # wait on all 3 to finish
    for p in processes:
        p.join()
    
    sensor_shm.close()
    sensor_shm.unlink()
    
    #plot data here
    #run rover stuff here
    # Rover main loop needs to select plant target

    log.kill()

    # On finish, remove running file
    if os.path.exists(".running.txt"):
        os.remove(".running.txt")

    subprocess.run(["sudo poweroff"], check=True)
        
if __name__ == '__main__':
    main()

#!/usr/bin/env python3

"""
TO DO:
-drop test thresholds?


-clarify if sim and launch thresholds
-implement actual rocket thresolds

-implement power loss backup
--may have to grab from csv file

-backup sensor data from alternate sensors

-Store non-ema data
--check intial data values. may have to set 1 for ema
"""

#----IMPORTS----    
#import libraries
import os
import csv
import time

#import classes
from payload_pipeline.state_machine import StateMachine

from payload_sensor.bmp580 import BMP
from payload_sensor.bno085 import BNO
from payload_sensor.sensor_simulation import Sensor_Data_Simulator
    
from payload_pipeline.telemetry_logger import TelemetryLogger

#----GLOBAL VARIABLES----
#mode: launch, drop, hand, sim
MODE = "launch"

#three type of thresholds: hand, sim, and launch
#hand thresholds
if MODE == "sim":
    LAUNCH_GFORCE_THRESHOLD     = 1.5   #G  gs needed to launch
    LAUNCH_ALTITUDE_THRESHOLD   = 1.0   #m  height needed to launch
    DESCENT_ALTITUDE_THRESHOLD  = 0.5   #m  below apogee to call descent
    DESCENT_APOGEE_THRESHOLD    = 2.0   #m  minimum apogee to care
    LANDING_GFORCE_THRESHOLD    = 0.2   #G  gs needed to call landing
    LANDING_VEL_THRESHOLD       = 0.8   #m/s velocity needed to call landing
    LANDING_ALTITUDE_THRESHOLD  = 3.0   #m height needed to call landing
elif MODE == "drop":
    LAUNCH_GFORCE_THRESHOLD     = 1.5   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 0.75   #m
    DESCENT_ALTITUDE_THRESHOLD  = .1   #m
    DESCENT_APOGEE_THRESHOLD    = 1.5   #m
    LANDING_GFORCE_THRESHOLD    = 0.2   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = -1.0   #m

elif MODE == "hand":
    LAUNCH_GFORCE_THRESHOLD     = 1.3   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 0.1   #m
    DESCENT_ALTITUDE_THRESHOLD  = 0.2   #m
    DESCENT_APOGEE_THRESHOLD    = 1   #m
    LANDING_GFORCE_THRESHOLD    = 0.2   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = 0.5   #m

elif MODE == "launch":
    LAUNCH_GFORCE_THRESHOLD     = 2.0   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 30.0   #m
    DESCENT_ALTITUDE_THRESHOLD  = 10.0   #m
    DESCENT_APOGEE_THRESHOLD    = 500.0  #m
    LANDING_GFORCE_THRESHOLD    = 0.8   #G
    LANDING_VEL_THRESHOLD       = 1.0   #m/s
    LANDING_ALTITUDE_THRESHOLD  = 20.0   #m
else:
    exit("Invalid MODE selected")

#ema constants
ALPHA_GFORCE   = 0.8
ALPHA_ALTITUDE = 0.5 # Calculate by hand before go/no go
ALPHA_VELOCITY = 0.8

#state transition evaluation constants
STABLE_READINGS = 3
STABLE_READINGS_FOR_LANDING = 10

#timeout constants
FLIGHT_TIMEOUT = 90000
# ROVER_TIMEOUT = 900
POWER_CYCLE_TIME = 45 # seconds

#data storage
data = {
    "timestamp": 0,
    "state": "READY",
    "raw_g_force": 0,
    "g_force": 0,
    "raw_altitude": 0,
    "altitude": 0,
    "raw_velocity": 0,
    "velocity": 0,
    "apogee": 0,
    "start_pressure": 0,
    "pressure": 0,
    "temperature": 0
    }



#----INITIALIZE CLASSES----
if MODE == "sim":
    bno = None
    bmp = None
    sim = Sensor_Data_Simulator()
else:
    bno = BNO()
    bmp = BMP()
    sim = None
    
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

        return (True, log_file)
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
                data["timestamp"] = col[0] # Only getting previous start time
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
                data["raw_g_force"] = float(row[2])
                data["g_force"] = float(row[3])
                data["raw_altitude"] = float(row[4])
                data["altitude"] = float(row[5])
                data["raw_velocity"] = float(row[6])
                data["velocity"] = float(row[7])
                data["apogee"] = float(row[8])
                data["start_pressure"] = float(col[9])
                print(f"Recovery values: {data}")
        

        # Recovery state machine with relative start time
        sm.recover(data["timestamp"])
        log.log_sensor(data=data)

    return powerLoss


def initialize_sensors():
    if MODE == "sim":
        pass
    else:
        try:
            bmp.initialize()
        except Exception as error:
            print(error)

        try: 
            bno.initialize()
        except Exception as error:
            print(error)

def validate_data():
    #need to figure out what to do if data is None
    #probably where kalman filter goes
    if data["g_force"] is None:
        data["g_force"] = 0
    if data["altitude"] is None:
        data["altitude"] = 0
    if data["velocity"] is None:
        data["velocity"] = 0

def get_sensor_data():
    data["timestamp"] = TelemetryLogger.get_timestamp()

    if MODE == "sim":
        sim.updateValues()
        data["raw_g_force"] = data["g_force"] = abs(sim.getAccel()) / 9.81
        data["raw_altitude"] = data["altitude"] = sim.getAlt()
        data["raw_velocity"] = data["velocity"] = sim.getVelocity()
        data["apogee"] = max(data["apogee"], data["altitude"])
    else:
        data["raw_acceleration"], data["acceleration"] = bno.get_acceleration()
        data["raw_g_force"], data["g_force"] = bno.convert_gforce(data["raw_acceleration"]), bno.convert_gforce(data["acceleration"])
        data["raw_velocity"], data["velocity"] = bmp.get_vertical_velocity()
        data["raw_altitude"], data["altitude"] = bmp.get_altitude()


        data["apogee"] = max(data["apogee"], data["altitude"])

        data["pressure"], _ = bmp.get_pressure()
        data["temperature"], _ = bmp.get_temperature()


def set_zero_altitude(power_loss):
    prev = bmp.get_pressure()
    while True:
        curr = bmp.get_pressure()
        if abs(curr - prev) < 5:
            break
        prev = curr

    if not power_loss:
        data["start_pressure"] = bmp.get_pressure()

    bmp.set_sea_level_pressure(data["start_pressure"])

    # for i in range(15):
    #     bmp.get_pressure()  
    # bmp.set_sea_level_pressure(bmp.get_pressure())

    # for i in range(5):
    #     time.sleep(0.5)
    #     bmp.get_pressure()
    # bmp.set_sea_level_pressure(bmp.get_pressure())

def main():
    global log
    log = TelemetryLogger(sensor_data=data)

    initialize_sensors()

    power_loss = power_loss_recovery()
    if MODE != "sim":
        set_zero_altitude(power_loss)
        

    while data["state"] != "LANDING":
        # start_loop = time.perf_counter()
        get_sensor_data()
        validate_data()
        data["state"] = sm.update(
            data["timestamp"],
            data["state"],
            data["g_force"],
            data["altitude"],
            data["velocity"],
            data["apogee"]
        )
        
        # Log data to file: (Time, Current State, G-Force, Altitute, Velocity, Apogee)
        log.log_sensor(data=data)
        

        # loop_time = time.perf_counter() - start_loop

        if MODE == "sim":
            time.sleep(0.04)
    
    #plot data here
    #run rover stuff here
    # Rover main loop needs to select plant target

    log.kill()

    # On finish, remove running file
    if os.path.exists(".running.txt"):
        os.remove(".running.txt")

        
if __name__ == '__main__':
    main()
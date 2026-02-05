"""
TO DO:
-data saving class- need to save sensor data to file
-save print statements to log file
-implement simulation mode
-clarify if sim and launch thresholds are the same. if so then switch mode to 1 and 0.
-implement power loss
-implement actual rocket thresolds
-filter data: ema or kalman filter. maybe ema until kalman is done
----ema=value*alpha + prev_ema*(1-alpha)
-need to evaluate transition validity
----stick to stable_readings method
-rover stuff after landing
-decide on velocity- vertical or magnitude

--classes to create: data_saving_and_logger, filter, power_loss
"""

#----IMPORTS----    
#import libraries
import time

#import classes
from payload_pipeline.state_machine import StateMachine

from payload_sensor.bmp580 import BMP
from payload_sensor.bno055 import BNO
from payload_sensor.sensor_simulation import Sensor_Data_Simulator
    
from payload_pipeline.telemetry_logger import TelemetryLogger

#----CONSTANTS----
#mode: launch, hand, sim
MODE = "sim"

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
elif MODE == "hand":
    LAUNCH_GFORCE_THRESHOLD     = 1.5   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 1.0   #m
    DESCENT_ALTITUDE_THRESHOLD  = 0.5   #m
    DESCENT_APOGEE_THRESHOLD    = 2.0   #m
    LANDING_GFORCE_THRESHOLD    = 0.2   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = 3.0   #m
elif MODE == "launch":
    LAUNCH_GFORCE_THRESHOLD     = 1.5   #G
    LAUNCH_ALTITUDE_THRESHOLD   = 1.0   #m
    DESCENT_ALTITUDE_THRESHOLD  = 2.0   #m
    DESCENT_APOGEE_THRESHOLD    = 0.2  #m
    LANDING_GFORCE_THRESHOLD    = 0.8   #G
    LANDING_VEL_THRESHOLD       = 0.8   #m/s
    LANDING_ALTITUDE_THRESHOLD  = 3.0   #m
else:
    exit("Invalid MODE selected")

#----GLOBAL VARIABLES----
ALPHA_GFORCE   = 0.8
ALPHA_ALTITUDE = 0.5
ALPHA_VELOCITY = 0.8
#if ema, set to 1.0
data = {
    "time": 0,
    "state": 0,
    "g_force": 0,
    "altitude": 0,
    "velocity": 0,
    "apogee": 0
    }
# current_state = 0
# current_g_force = 0
# current_altitude = 0
# current_velocity = 0
# current_apogee = -1
#decide between vertical velocity or velocity magnitude


# #state transition evaluation constants
# STABLE_READINGS_FOR_LAUNCH  = 3
# STABLE_READINGS_FOR_DESCENT = 3
# STABLE_READINGS_FOR_LANDING = 10
# STABLE_READINGS_FOR_LANDING_VG = 3

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
    LANDING_ALTITUDE_THRESHOLD
)

# Initialize Logger
log = TelemetryLogger()

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
    if MODE == "sim":
        sim.updateValues()
        data["g_force"] = abs(sim.getAccel()) / 9.81
        data["altitude"] = sim.getAlt()
        data["velocity"] = sim.getVelocity()
        data["apogee"] = max(data["apogee"], data["altitude"])
    else:
        g_force = bno.get_g_force()
        alt= bmp.get_altitude()
        vel_z = bno.get_vertical_velocity()

        data["g_force"]   = ALPHA_GFORCE   * g_force     + (1 - ALPHA_GFORCE)   * data["g_force"]
        data["altitude"] = ALPHA_ALTITUDE * alt         + (1 - ALPHA_ALTITUDE) * data["altitude"]
        data["velocity"]  = ALPHA_VELOCITY * abs(vel_z)  + (1 - ALPHA_VELOCITY) * data["velocity"]
        data["apogee"] = max(data["apogee"], data["altitude"])


def set_zero_altitude():
    for i in range(5):
        time.sleep(0.5)
        bmp.get_pressure()

def main():
    initialize_sensors()
    if MODE == "sim":
        pass
    else:
        set_zero_altitude()

    while data["state"] != 3:
        get_sensor_data()
        validate_data()
        data["state"] = sm.update(
            data["state"],
            data["g_force"],
            data["altitude"],
            data["velocity"],
            data["apogee"]
        )
        
        # Log (Time, Current State, G-Force, Altitute, Velocity, Apogee)
        # TODO: More human readable timestamp? Right now is Unix time
        log.log_sensor(data={"time": time.time(), "state": data["state"], 
                             "g_force": data["g_force"], "altitude": data["altitude"], 
                             "velocity": data["velocity"], "apogee": data["apogee"]})
        
        # print(f"Current State: {data['state']}, G-Force: {data['g_force']:.2f} g, Altitude: {data['altitude']:.2f} m, Velocity: {data['velocity']:.2f} m/s, Apogee: {data['apogee']:.2f} m")
        

        #save data to file here
        #save log statements to log file here
        time.sleep(0.1)
    
    #plot data here
    #run rover stuff here

        
if __name__ == '__main__':
    main()
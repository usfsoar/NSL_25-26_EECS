#----IMPORTS----    
#import libraries
import time

#import classes
from payload_sensor.bmp580 import BMP
from payload_sensor.bno055 import BNO
from payload_sensor.sensor_simulation import Sensor_Data_Simulator

#----INITIALIZE CLASSES----
bno = BNO()
bmp = BMP()
sim = Sensor_Data_Simulator()

#----CONSTANTS----
#simulation
SIM = 1

#hand test
HAND_TEST = 0

#sample rate
HZ = 20.0
DT = 1.0 / HZ

#two type of thresholds: hand test, launch
#hand test thresholds
LAUNCH_ACCEL_THRESHOLD      = 1.5   # EMA G-force > 1.5 G
LAUNCH_ALTITUDE_THRESHOLD   = 1.0  # m AGL
DESCENT_ALTITUDE_THRESHOLD  = 0.5  # m below apogee to call descent
DESCENT_APOGEE_THRESHOLD    = 2.0   # m minimum apogee to care
LANDING_ACCEL_THRESHOLD     = 0.2   # |EMA G - 1| < 0.2
LANDING_VEL_THRESHOLD       = 0.8   # m/s
LANDING_ALTITUDE_THRESHOLD  = 3.0   # m

CURRENT_STATE = 0
current_g_force = 0
current_altitude = 0
current_velocity = 0
#decide between vertical velocity or velocity magnitude


# #state transition evaluation constants
# STABLE_READINGS_FOR_LAUNCH  = 3
# STABLE_READINGS_FOR_DESCENT = 3
# STABLE_READINGS_FOR_LANDING = 10
# STABLE_READINGS_FOR_LANDING_VG = 3

def initialize_sensors():
    if SIM:
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
    if current_g_force is None:
        current_g_force = 0
    if current_altitude is None:
        current_altitude = 0
    if current_velocity is None:
        current_velocity = 0

def get_sensor_data():
    if SIM:
        pass
    else:
        current_g_force = bno.get_acceleration()[2] / 9.81
        current_altitude = bmp.get_altitude()
        current_velocity = bno.get_vertical_velocity()

def set_zero_altitude():
    for i in range(5):
        time.sleep(0.5)
        bmp.get_pressure()

def main():
    initialize_sensors()
    set_zero_altitude()

    while True:
        get_sensor_data()
        validate_data()
        



if __name__ == '__main__':
    main()
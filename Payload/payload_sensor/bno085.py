# Wrapper class for the BNO085 IMU Sensor
# API Reference: https://docs.circuitpython.org/projects/bno08x/en/latest/

from adafruit_bno08x.i2c import BNO08X_I2C
from abstract_bno import abstract_BNO
import adafruit_bno08x
import board
import busio
import time
import math

from adafruit_bno08x import (
    BNO_REPORT_ACCELEROMETER
  #  BNO_REPORT_GYROSCOPE,
   # BNO_REPORT_MAGNETOMETER,
   # BNO_REPORT_ROTATION_VECTOR,
   # BNO_REPORT_LINEAR_ACCELERATION,
   # BNO_REPORT_GRAVITY
)

RECOVERY_WAIT = 0.05

# todo: Class getters should probably have exceptions and bad data handled

class BNO085(abstract_BNO):
    """
    Wrapper class for interating with the BNO055 IMU Sensor
    """

    def __init__(self):
        pass
    

    def initialize(self, alpha, address: int = 0x4a):
        """
        Input: I2C Address\n
        Output: None\n
        Note: Attempts to initialize and configure the BNO sensor. 
        Will throw an exception if there is an error initializing. 
        """
        self.address = address
        self.alpha = alpha
        self.accel = 0

        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                print(f"Error initializing I2C for BNO: {e}")
        
        for i in range(10):
            try:
                self.sensor = BNO08X_I2C(self.i2c, debug=False, address=address)

                # Initalize sensor
                self.sensor.initialize()
                
                # Calibrate
                self.sensor.begin_calibration()

                self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_ACCELEROMETER)#, 2000)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_MAGNETOMETER)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_ROTATION_VECTOR)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_LINEAR_ACCELERATION)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_GRAVITY)
                break
            except Exception as e:
                print(f"Error initializing BNO: {e}")



if __name__ == '__main__':
    bno = BNO085()
    try:
        bno.initialize(alpha=0.8)
    except Exception as e:
        print(e)

    bno.recover()

    while True:
        raw_accel, accel = bno.get_acceleration()
        print(f"Accel: {raw_accel}")
        # print(f"Accel w/o gravity: {bno.get_linear_acceleration()}")
        # print(f"Velocity: {bno.get_velocity()}")
        print(f"Gravity: {bno.convert_gforce(raw_accel)}")
        # print(f"Calibrated: {bno.is_calibrated()}")
        time.sleep(0.5)

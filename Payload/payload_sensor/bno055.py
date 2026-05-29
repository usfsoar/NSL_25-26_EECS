# Wrapper class for the BNO055 IMU Sensor
# API Reference: https://docs.circuitpython.org/projects/bno055/en/latest/api.html#

import adafruit_bno055
from abstract_bno import abstract_BNO
import board
import busio
import time
import math

# todo: Class getters should probably have exceptions and bad data handled

class BNO055(abstract_BNO):
    """
    Wrapper class for interating with the BNO055 IMU Sensor
    """

    def __init__(self):
        pass
    

    def initialize(self, address: int = 0x28):
        """
        Input: I2C Address\n
        Output: None\n
        Note: Attempts to initialize and configure the BNO sensor. 
        Will throw an exception if there is an error initializing. 
        """
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                print(f"Error initializing I2C for BNO: {e}")
        
        for i in range(10):
            try:
                self.sensor = adafruit_bno055.BNO055_I2C(self.i2c, address=address)

                # Set mode to configuration
                self.sensor.mode = adafruit_bno055.CONFIG_MODE
                time.sleep(0.05) # How much time does switching modes take?

                # Set mode to absolute orientation fusion
                self.sensor.mode = adafruit_bno055.NDOF_MODE
                time.sleep(0.05)
                break
            except Exception as e:
                print(f"Error initializing BNO: {e}")



if __name__ == '__main__':
    bno = BNO055()
    try:
        bno.initialize()
    except Exception as e:
        print(e)

    while True:
        print(f"Accel: {bno.get_acceleration()}")
        print(f"Accel w/o gravity: {bno.get_linear_acceleration()}")
        print(f"Velocity: {bno.get_velocity()}")
        print(f"Gravity: {bno.get_gravity()}")
        print(f"Orientation: {bno.get_orientation()}")
        print(f"Temperature: {bno.get_temperature()}")
        print(f"Calibrated: {bno.is_calibrated()}")

        time.sleep(1)



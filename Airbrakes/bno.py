# Class to make calls to the BNO easier

import board
import busio
import adafruit_bno08x
from adafruit_bno08x.i2c import BNO08X_I2C
from adafruit_bno08x import (
    BNO_REPORT_ACCELEROMETER,
    BNO_REPORT_GYROSCOPE,
    BNO_REPORT_MAGNETOMETER,
    BNO_REPORT_ROTATION_VECTOR,
    BNO_REPORT_LINEAR_ACCELERATION,
    BNO_REPORT_GRAVITY
)


class BNO:
    def __init__(self):
        self.i2c = busio.I2C(board.SCL, board.SDA)
        self.sensor = BNO08X_I2C(self.i2c)

        self.sensor.enable_feature(BNO_REPORT_ACCELEROMETER)
        self.sensor.enable_feature(BNO_REPORT_MAGNETOMETER)
        self.sensor.enable_feature(BNO_REPORT_ROTATION_VECTOR)
        self.sensor.enable_feature(BNO_REPORT_LINEAR_ACCELERATION)
        self.sensor.enable_feature(BNO_REPORT_GRAVITY)


    def magnetic(self):
        return self.sensor.magnetic
    
    # def gyro(self):
    #     return self.sensor.gyro
    
    def quaternion(self):
        return self.sensor.quaternion

    def linear_acceleration(self):
        return self.sensor.linear_acceleration
    
    def acceleration(self):
        return self.sensor.acceleration
    
    def gravity(self):
        return self.sensor.gravity

if __name__ == '__main__':
    import time
    bno = BNO()
    print('Created bno object')
    while(1):
        print(f'{bno.magnetic()=}')
        # print(f'{bno.gyro()=}')
        print(f'{bno.quaternion()=}')
        print(f'{bno.linear_acceleration()=}')
        print(f'{bno.acceleration()=}')
        print(f'{bno.gravity()=}')
        time.sleep(2)

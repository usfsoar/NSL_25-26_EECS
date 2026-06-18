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
import time

ALPHA = 0.3
RECOVERY_WAIT = 0.1
class BNO:
    def __init__(self):
        pass
        

    def initialize(self, accel=None):
        self.i2c = busio.I2C(board.SCL, board.SDA)
        self.sensor = BNO08X_I2C(self.i2c)

        self.sensor.enable_feature(BNO_REPORT_ACCELEROMETER)
        self.sensor.enable_feature(BNO_REPORT_GYROSCOPE)
        self.sensor.enable_feature(BNO_REPORT_MAGNETOMETER)
        self.sensor.enable_feature(BNO_REPORT_ROTATION_VECTOR)
        self.sensor.enable_feature(BNO_REPORT_LINEAR_ACCELERATION)
        self.sensor.enable_feature(BNO_REPORT_GRAVITY)
        time.sleep(0.2)
        if accel:
            self.accel = accel
        else:
            self.accel = self.sensor.linear_acceleration[2]

    def recover(self):
        print(f"Recovering BNO")
        try:
            del self.sensor
            del self.i2c
        except Exception as e:
            print(f"Error deleting old i2c sensor object: {e}")
        time.sleep(RECOVERY_WAIT)
        self.initialize(alpha=self.alpha, address=self.address)


    def magnetic(self):
        return self.sensor.magnetic
    
    def gyro(self):
         return self.sensor.gyro
    
    def quaternion(self):
        return self.sensor.quaternion

    def linear_acceleration(self):
    
        curr_accel = 0
        for i in range (8):
            try:
                curr_accel = self.sensor.linear_acceleration
                EMA_Accel = ALPHA * curr_accel + (1-ALPHA) * self.accel
                self.accel = EMA_Accel
                return EMA_Accel
            except Exception as e:
                print(f"BNO Linear Acceleration Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()
        return self.accel
        
    
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
        print(f'{bno.gyro()=}')
        print(f'{bno.quaternion()=}')
        print(f'{bno.linear_acceleration()=}')
        print(f'{bno.acceleration()=}')
        print(f'{bno.gravity()=}')
        time.sleep(2)

# Class to make calls to the BNO easier

import board
import busio
import adafruit_bno08x
from adafruit_bno08x.i2c import BNO08X_I2C


class BNO():
    def __init__(self):
        self.i2c = busio.I2C(board.SCL, board.SDA)
        self.sensor = BNO08X_I2C(self)

    def temperature(self):
        return self.sensor.temperature
    
    def magnetic(self):
        return self.sensor.magnetic
    
    def gyro(self):
        return self.sensor.gyro
    
    def euler(self):
        return self.sensor.euler
    
    def quaternion(self):
        return self.sensor.quaternion

    def linear_acceleration(self):
        return self.sensor.linear_acceleration
    
    def acceleration(self):
        return self.sensor.acceleration
    
    def gravity(self):
        return self.sensor.gravity

if __name__ == '__main__':
    pass
# Wrapper class for the PCA9865 PWM Controller
# API Reference: https://docs.circuitpython.org/projects/pca9685/en/latest/api.html

import adafruit_pca9685
import board
import busio
import time


class PCA():
    def __init__(self):
        pass


    def initialize(self, address: int = 0x40):
        """
        Input: I2C Address
        Output: None\n
        Note: Will throw an exception if the PCA sensor cannot be initialized
        """
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing I2C for PCA: {e}")
                continue
        
        for i in range(10):
            try:
                self.sensor = adafruit_pca9685.PCA9685(self.i2c, address=address)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing PCA: {e}")
                continue


    def set_duty_cycle(self, duty_cycle: int):
        """
        Input: Indicated how much of a cycle is high versus low: 0xffff high to 0 low\n
        Output: None
        """
        self.sensor.duty_cycle = duty_cycle


if __name__ == '__main__':
    pca = PCA()
    try:
        pca.initialize()
    except Exception as e:
        print(e)

    duty_cycle = 0
    while True:
        duty_cycle = (duty_cycle + 0x1) % 0xffff
        pca.set_duty_cycle(duty_cycle)
        time.sleep(1)
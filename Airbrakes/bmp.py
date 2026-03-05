# Class to make calls to the BMP easier
import board
from adafruit_bmp5xx import BMP5XX

class BMP:
    SEALEVELPRESSURE_HPA = 1013.25
    def __init__(self):
        # I2C setup
        self.i2c = board.I2C()  # uses board.SCL and board.SDA
        # i2c = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller
        self.bmp = BMP5XX.over_i2c(self.i2c)
    def temperature(self):
        return self.bmp.temperature
    
    def pressure(self):
        return self.bmp.pressure
    
    def altitude(self):
        return self.bmp.altitude
    
    def set_sea_level(self):
        self.bmp.sea_level_pressure = self.bmp.pressure

if __name__ == '__main__':
    import time
    bmp = BMP()
    print(f'Created bmp object')
    time.sleep(2)

    bmp.set_sea_level()
    while True:
        print(f'{bmp.temperature()=}')
        print(f'{bmp.pressure()=}')
        print(f'{bmp.altitude()=}')
        time.sleep(2)

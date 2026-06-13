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
        self.temp = None
        self.pres = None
        self.alt = None
    def temperature(self):
        temp = self.bmp.temperature
        print(f'{temp=}')
        return 0.8* temp + 0.2*self.temp
    
    def pressure(self):
        pres = self.bmp.pressure
        return 0.8*pres + 0.2*self.pres
    
    def altitude(self):
        curr_alt = self.bmp.altitude
        print(f'{curr_alt=}')
        EMA_alt = 0.8*curr_alt + 0.2*self.alt
        print(f'{EMA_alt=}')
        self.alt = EMA_alt
        return EMA_alt
    
    def initialize(self):
        self.temp = self.bmp.temperature
        self.pres = self.bmp.pressure
        self.alt = self.bmp.altitude
    def set_sea_level(self):
        self.bmp.sea_level_pressure = self.bmp.pressure

if __name__ == '__main__':
    import numpy as np
    import time
    bmp = BMP()
    print(f'Created bmp object')
    time.sleep(2)

    bmp.set_sea_level()
    bmp.initialize()
    arr = np.array([])
    for i in range(1000):
        
        arr = np.append(arr, bmp.altitude())
        time.sleep(0.01)
    varience = np.var(arr)
    print(f'{varience=}')




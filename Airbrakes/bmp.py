# Class to make calls to the BMP easier
import time
import board
from adafruit_bmp5xx import BMP5XX

class BMP():
    SEALEVELPRESSURE_HPA = 1013.25
    def __init__(self):
        # I2C setup
        self.i2c = board.I2C()  # uses board.SCL and board.SDA
        # i2c = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller
        self.bmp = BMP5XX.over_i2c(self.i2c)
    def temperature():
        return bmp.temperature
    
    def pressure():
        return bmp.pressure
    
    def altitude():
        return bmp.altitude

if __name__ == '__main__':
    SEALEVELPRESSURE_HPA = 1013.25
    bmp = BMP()
    while True:
        if bmp.data_ready:
            print(f"temp F: {bmp.temperature * (9 / 5) + 32}", f"pressure: {bmp.pressure} hPa", f"Approx altitude: {bmp.altitude} m")
            time.sleep(1)
    pass

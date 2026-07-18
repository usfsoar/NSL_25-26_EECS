# Wrapper class for the STEMMA Soil Sensor
# API Reference: https://docs.circuitpython.org/projects/seesaw/en/latest/api.html#

from adafruit_seesaw.seesaw import Seesaw
import board
import busio
import time



class STEMMA():
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
                print(f"Error initializing I2C for PCA: {e}")
        
        for i in range(10):
            try:
                self.sensor = Seesaw(self.i2c, addr=address)
                break
            except Exception as e:
                print(f"Error initializing PCA: {e}")
    

    def get_moisture(self):
        """
        Input: None\n
        Output: Returns the measured moisture
        """
        return self.sensor.moisture_read()
    

    def get_temperature(self):
        """
        Input: None\n
        Output: Returns the measured temperature in 
        """
        return self.sensor.get_temp()


if __name__ == '__main__':
    stemma = STEMMA()
    try:
        stemma.initialize()
    except Exception as e:
        print(e)

    while True:
        print(f"Moisture: {stemma.get_moisture()}")
        print(f"Temperature: {stemma.get_temperature()}")
        time.sleep(1)
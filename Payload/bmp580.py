# Wrapper class for the BMP580 Barometer
# API Reference: https://docs.circuitpython.org/projects/bmp5xx/en/latest/api.html#

import adafruit_bmp5xx
import board
import busio
import time

# todo: Class getters should probably have exceptions and bad data handled

class BMP():
    """
    Wrapper class for interating with the BMP580 Barometer
    """

    def __init__(self):
        pass
    
    
    def initialize(self, address: int = 0x47, sea_level: float = 1013.25):
        """
        Input: I2C Address; Pressure at sea levelin hPa\n
        Output: None\n
        Note: Will throw an exception if the BMP sensor cannot be initialized
        """
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing I2C for BMP: {e}")
                continue
        
        for i in range(10):
            try:
                self.sensor = adafruit_bmp5xx.BMP5XX_I2C(self.i2c, address=address)
                self.set_sea_level_pressure(sea_level)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing BMP: {e}")
                continue

    
    def set_sea_level_pressure(self, sea_level: float):
        """
        Input: Pressure at sea levelin hPa\n
        Output: None
        """
        self.bmp.sea_level_pressure = sea_level


    def get_altitude(self):
        """
        Input: None
        Output: Returns altitude in meters
        """
        return self.sensor.altitude
    

    def get_vertical_velocity(self):
        """
        Input: None
        Output: Returns vertical velocity in m/s
        """
        # This velocity will be very noise dependent. Should implement filtering here or on the user's end
        start_t = time.time() # Possibly switch this to time.perf_counter()
        start_alt  = self.get_altitude() # Should these just get straight from sensor and avoid function overhead?
        time.sleep(0.2) # Max Sampling Rate of the BMP is 200 Hz (ie 200 ms)
        end_alt = self.get_altitude()
        delta_t = time.time() - start_t

        return (end_alt - start_alt) / delta_t


    def get_pressure(self):
        """
        Input: None
        Output: Returns pressure in hPa
        """
        return self.sensor.pressure


    def get_temperature(self):
        """
        Input: None
        Output: Returns temperature in Celsius
        """
        return self.sensor.temperature


if __name__ == '__main__':
    bmp = BMP()
    try:
        bmp.initialize()
    except Exception as e:
        print(e)

    while True:
        print(f"Altitude: {bmp.get_altitude()}")
        print(f"Vertical Velocity: {bmp.get_vertical_velocity()}")
        print(f"Pressure: {bmp.get_pressure()}")
        print(f"Temperature: {bmp.get_temperature()}")
        time.sleep(1)
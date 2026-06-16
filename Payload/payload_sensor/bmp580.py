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
    
    
    def initialize(self, alpha, address: int = 0x47, sea_level: float = 1013.25):
        """
        Input: I2C Address; Pressure at sea levelin hPa\n
        Output: None\n
        Note: Will throw an exception if the BMP sensor cannot be initialized
        """
        self.alpha = alpha
        self.address = address
        self.prev = (0,0)
        self.alt = (0,0)

        self.pressure = 0
        self.temperature = 0
        
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                print(f"Error initializing I2C for BMP: {e}")
        
        for i in range(10):
            try:
                self.sensor = adafruit_bmp5xx.BMP5XX_I2C(self.i2c, address=address)
                self.sensor.pressure_oversample = 8
                self.set_sea_level_pressure(sea_level)
                self.sensor.output_data_rate = adafruit_bmp5xx.BMP5XX_ODR_240_HZ
                break
            except Exception as e:
                print(f"Error initializing BMP: {e}")

    
    def recover(self):
        print(f"Recovering BMP")
        del self.sensor
        del self.i2c
        self.initialize(alpha=self.alpha, address=self.address)


    def set_sea_level_pressure(self, sea_level: float):
        """
        Input: Pressure at sea levelin hPa\n
        Output: None
        """
        self.sensor.sea_level_pressure = sea_level


    def get_altitude(self):
        """
        Input: None
        Output: Returns altitude in meters
        """
        raw_altitude = 0
        self.prev = self.alt
        for i in range (8):
            try:
                raw_altitude = self.sensor.altitude
            except Exception as e:
                print(f"BMP Altitude Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()

        self.alt = ((self.alpha * raw_altitude) + (1 - self.alpha)*self.prev[0], time.perf_counter(), raw_altitude)

        return raw_altitude, self.alt[1]    

    def get_vertical_velocity(self):
        """
        Input: None
        Output: Returns vertical velocity in m/s
        """
        for _ in range(2):
            if time.perf_counter() - self.prev[1] > 1:
                raw_alt, _ = self.get_altitude()
        
        delta_t = self.alt[1] - self.prev[1]
        return (self.alt[0] - self.prev[0]) / delta_t, self.alt[2], self.alt[0]


    def get_pressure(self):
        """
        Input: None
        Output: Returns pressure in hPa
        """
        pressure = 0
        for i in range (8):
            try:
                pressure = self.sensor.pressure
            except Exception as e:
                print(f"BMP Pressure Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()

        self.pressure = (self.alpha * pressure) + (1 - self.alpha)*self.pressure

        return pressure, self.pressure


    def get_temperature(self):
        """
        Input: None
        Output: Returns temperature in Celsius
        """
        temperature = 0
        for i in range (8):
            try:
                temperature = self.sensor.temperature
            except Exception as e:
                print(f"BMP Temperature Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()

        self.temperature = (self.alpha * temperature) + (1 - self.alpha)*self.temperature
        return temperature, self.temperature


if __name__ == '__main__':
    bmp = BMP()
    try:
        bmp.initialize(alpha=0.5)
    except Exception as e:
        print(e)

    while True:
        print(f"Altitude: {bmp.get_altitude()}")
        print(f"Vertical Velocity: {bmp.get_vertical_velocity()}")
        print(f"Pressure: {bmp.get_pressure()}")
        print(f"Temperature: {bmp.get_temperature()}")
        time.sleep(1)
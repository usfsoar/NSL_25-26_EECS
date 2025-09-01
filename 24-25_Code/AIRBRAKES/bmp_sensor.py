import adafruit_bmp3xx #type:ignore
import busio #type:ignore
import board #type:ignore

class BMP:
    
    def __init__(self):
        pass

    def initialize_bmp(self):
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except:
                if (i == 9):
                    print("Error initializing i2c for bmp")
                    return False
                continue
        for i in range(10):
            try:
                self.bmp = adafruit_bmp3xx.BMP3XX_I2C(self.i2c, address=0x76)
                break
            except:
                if (i == 9):
                    print("Error initializing bmp")
                    return False
                continue
        return True
    def set_sea_level(self, sea_level):
        self.bmp.sea_level_pressure = sea_level

    def get_altitude(self):
        try:
            altitude = self.bmp.altitude
            if altitude == None:
                print("Altitude is None")
        except:
            altitude = None
            print("Error getting altitude")
        return altitude
    
    def get_velocity(self, altitude, prev_altitude, dt, error_count):
        if altitude == None:
            return None
        velocity = (altitude-prev_altitude)/(dt*(error_count+1))
        return velocity
    def get_pressure(self):
        for i in range(10):
            try:
                self.pressure = self.bmp.pressure
                return self.pressure
            except:
                if i == 9:
                    self.pressure = None
                    print("Error getting pressure")
                    return None
                continue
    def get_temperature(self):
        for i in range(10):
            try:
                self.temperature = self.bmp.temperature
                return self.temperature
            except:
                if i == 9:
                    self.temperature = None
                    print("Error getting temperature")
                    return None
                continue
    def get_air_density(self):
        if self.pressure is not None and self.temperature is not None:
            pres = self.pressure * 100
            temp = self.temperature + 273.15
            return pres / (8.3145 * temp)
        else:
            print("Failed to calculate air density")
            return None
        



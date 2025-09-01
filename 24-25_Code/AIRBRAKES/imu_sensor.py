import adafruit_bno055 #type:ignore
import board #type: ignore
import busio # type: ignore
import math
import time

class IMU:
    
    def __init__(self):
        pass        

    def initialize_imu(self):
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except:
                if (i == 10):
                    print("Error initializing i2c for bno")
                    return False
                continue
        for i in range(10):
            try:
                self.imu = adafruit_bno055.BNO055_I2C(self.i2c)
                time.sleep(0.5)

                # Set to CONFIG mode first
                self.imu.mode = adafruit_bno055.CONFIG_MODE
                time.sleep(0.1)

                # Then set to NDOF mode (sensor fusion, required for gravity)
                self.imu.mode = adafruit_bno055.NDOF_MODE
                time.sleep(0.5)

                break
            except:
                if (i == 10):
                    print("Error initializing imu")
                    return False
                continue
        return True
    
    #acceleration x,y,z
    def acceleration(self):
        
        try:
            acceleration = self.imu.acceleration
            
            if acceleration == None or None in acceleration:
                print("Acceleration is None")
            self.acceleration_x, self.acceleration_y, self.acceleration_z = acceleration
            return acceleration
        except Exception as e:
            self.acceleration_x = self.acceleration_y = self.acceleration_z = None
            print(f"Error getting acceleration {e}")
            return None

    def get_acceleration_x(self):        
        self.acceleration()
        if (self.acceleration_x == None):
            print("acceleration x is None")
        return self.acceleration_x
    
    def get_acceleration_y(self):
        self.acceleration()
        if (self.acceleration_y == None):
            print("acceleration y is None")
        return self.acceleration_y
    
    def get_acceleration_z(self):
        self.acceleration()
        if (self.acceleration_z == None):
            print("acceleration z is None")
        return self.acceleration_z
    

    #gravity x,y,z
    def gravity(self):
        try:
            self.gravity_x, self.gravity_y, self.gravity_z = self.imu.gravity
            if (self.gravity_x == None) or (self.gravity_y == None) or (self.gravity_z == None):
                print("gravity is None")
        except:
            self.gravity_x = self.gravity_y = self.gravity_z = None
            print("Error getting gravity")

    def get_gravity_x(self):        
        self.gravity()
        if (self.gravity_x == None):
            print("gravity z is None")
        
        return self.gravity_x
    
    def get_gravity_y(self):
        self.gravity()
        if (self.gravity_y == None):
            print("gravity y is None")
        
        return self.gravity_y
    
    def get_gravity_z(self):
        self.gravity()
        if (self.gravity_z == None):
            print("gravity z is None")
        
        return self.gravity_z
    

    #gforce
    def get_gforce(self):
        try:
            accel_x = self.get_acceleration_x()
            accel_y = self.get_acceleration_y()
            accel_z = self.get_acceleration_z()
            g_force = math.sqrt((accel_x * accel_x) + (accel_y * accel_y)+ (accel_z * accel_z))/9.81
            if g_force is None:
                print("G-force is None")
                g_force = None
            return g_force
        except Exception as e:
            print(f"Error calculating g_force {e}")
            return None
        
    def gyro(self):
        try:
            gyro_x, gyro_y, gyro_z = self.imu.gyro
            if (gyro_x == None) or (gyro_y == None) or (gyro_z == None):
                print("Gyro is None")
                return None, None, None
            return gyro_x, gyro_y, gyro_z
        except:
            print("Error getting Gyro")
            return None, None, None
        
    def magnetometer(self):
        try:
            mag_x, mag_y, mag_z = self.imu.magnetic
            if (mag_x == None) or (mag_y == None) or (mag_z == None):
                print("Mangetometer is None")
                return None, None, None
            return mag_x, mag_y, mag_z
        except:
            print("Error getting Magnetometer")
            return None, None, None


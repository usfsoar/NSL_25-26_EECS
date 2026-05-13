# Wrapper class for the BNO085 IMU Sensor
# API Reference: https://docs.circuitpython.org/projects/bno08x/en/latest/

from adafruit_bno08x.i2c import BNO08X_I2C
import adafruit_bno08x
import board
import busio
import time
import math

from adafruit_bno08x import (
    BNO_REPORT_ACCELEROMETER
  #  BNO_REPORT_GYROSCOPE,
   # BNO_REPORT_MAGNETOMETER,
   # BNO_REPORT_ROTATION_VECTOR,
   # BNO_REPORT_LINEAR_ACCELERATION,
   # BNO_REPORT_GRAVITY
)

RECOVERY_WAIT = 0.05

# todo: Class getters should probably have exceptions and bad data handled

class BNO():
    """
    Wrapper class for interating with the BNO055 IMU Sensor
    """

    def __init__(self):
        pass
    

    def initialize(self, alpha, address: int = 0x4a):
        """
        Input: I2C Address\n
        Output: None\n
        Note: Attempts to initialize and configure the BNO sensor. 
        Will throw an exception if there is an error initializing. 
        """
        self.address = address
        self.alpha = alpha
        self.accel = 0

        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing I2C for BNO: {e}")
                continue
        
        for i in range(10):
            try:
                self.sensor = BNO08X_I2C(self.i2c, debug=False, address=address)

                # Initalize sensor
                self.sensor.initialize()
                
                # Calibrate
                self.sensor.begin_calibration()

                self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_ACCELEROMETER)#, 2000)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_MAGNETOMETER)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_ROTATION_VECTOR)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_LINEAR_ACCELERATION)
                # self.sensor.enable_feature(adafruit_bno08x.BNO_REPORT_GRAVITY)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing BNO: {e}")
                continue

    def recover(self):
        print(f"Recovering BNO")
        del self.sensor
        del self.i2c
        time.sleep(RECOVERY_WAIT)
        self.initialize(alpha=self.alpha, address=self.address)


    def get_acceleration(self):
        """
        Input: None\n
        Output: Returns raw acceleration (accel_x, accel_y, accel_z) m/s^2\n
        Note: If acceleration has been disabled, returns empty 3-tuple
        """
        raw_accel = 0
        for i in range (8):
            try:
                raw_accel = self.sensor.acceleration
                raw_accel = math.sqrt(raw_accel[0]*raw_accel[0] + raw_accel[1]*raw_accel[1] + raw_accel[2]*raw_accel[2])
            except Exception as e:
                print(f"BNO Acceleration Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()
        self.accel = (self.alpha * raw_accel) + (1 - self.accel) * self.accel

        return raw_accel, self.accel
    

    def get_linear_acceleration(self):
        """
        Input: None\n
        Output: Returns acceleration without gravity (accel_x, accel_y, accel_z) m/s^2\n
        Note: If linear acceleration has been disabled, retuns empty 3-tuple
        """
        linear_accel = 0
        for i in range (8):
            try:
                linear_accel = self.sensor.linear_acceleration
            except Exception as e:
                print(f"BNO Linear Acceleration Exception: {e}")
                # If we fail 6 times, recover before last times:
                if i >= 5: 
                    self.recover()
        return linear_accel

    # def get_g_force(self):
    #     """
    #     Returns magnitude of the g-force (using Euclidian distance)
    #     """
    #     raw_accel, accel = self.get_acceleration()
    #     return math.sqrt(accel[0]*accel[0] + accel[1]*accel[1] + accel[2]*accel[2]) / 9.81


    def convert_gforce(self, accel):
        return accel / 9.81


    # def get_velocity(self):
    #     # todo
    #     prev_time = time.time()
    #     velocity = [0, 0, 0] # Will this even work or do we need starting velocity?
    #     for i in range(0, 10): # Max value is number of readings we want to sum over
    #         measurement = self.get_acceleration()
    #         cur_time = time.time()
    #         delta_t = cur_time - prev_time
    #         prev_time = cur_time
    #         velocity[0] += measurement[0] * delta_t
    #         velocity[1] += measurement[1] * delta_t
    #         velocity[2] += measurement[2] * delta_t
    #         time.sleep(0.1) # 100 Hz delay
    #     return (velocity[0], velocity[1], velocity[2])


    def get_gravity(self):
        """
        Input: None\n
        Output: Returns the gravity vector (gravity_x, gravity_y, gravity_z)\n
        Note: If gravity has been disabled, returns empty 3-tuple
        """
        gravity = 0
        for i in range (8):
            try:
                gravity = self.sensor.gravity
            except Exception as e:
                print(e)
        return gravity


    def is_calibrated(self):
        """
        Input: None\n
        Output: Boolean indicating if sensor is calibrated
        """
        status = 0
        for i in range (8):
            try:
                status = self.sensor.calibration_status
            except Exception as e:
                print(e)
        return status


if __name__ == '__main__':
    bno = BNO()
    try:
        bno.initialize(alpha=0.8)
    except Exception as e:
        print(e)

    bno.recover()

    while True:
        raw_accel, accel = bno.get_acceleration()
        print(f"Accel: {raw_accel}")
        # print(f"Accel w/o gravity: {bno.get_linear_acceleration()}")
        # print(f"Velocity: {bno.get_velocity()}")
        print(f"Gravity: {bno.convert_gforce(raw_accel)}")
        # print(f"Calibrated: {bno.is_calibrated()}")
        time.sleep(0.5)

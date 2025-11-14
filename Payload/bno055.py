# Wrapper class for the BNO055 IMU Sensor
# API Reference: https://docs.circuitpython.org/projects/bno055/en/latest/api.html#

import adafruit_bno055
import board
import busio
import time

# todo: Class getters should probably have exceptions and bad data handled

class BNO():
    """
    Wrapper class for interating with the BNO055 IMU Sensor
    """

    def __init__(self):
        pass
    

    def initialize(self, address: int = 28):
        """
        Input: I2C Address\n
        Output: None\n
        Note: Attempts to initialize and configure the BNO sensor. 
        Will throw an exception if there is an error initializing. 
        """
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
                self.sensor = adafruit_bno055.BNO055_I2C(self.i2c, address=address)

                # Set mode to configuration
                self.sensor.mode = adafruit_bno055.CONFIG_MODE
                time.sleep(0.05) # How much time does switching modes take?

                # Set mode to absolute orientation fusion
                self.sensor.mode = adafruit_bno055.NDOF_MODE
                time.sleep(0.05)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing BNO: {e}")
                continue


    def get_acceleration(self):
        """
        Input: None\n
        Output: Returns raw acceleration (accel_x, accel_y, accel_z) m/s^2\n
        Note: If acceleration has been disabled, returns empty 3-tuple
        """
        return self.sensor.acceleration
    

    def get_linear_acceleration(self):
        """
        Input: None\n
        Output: Returns acceleration without gravity (accel_x, accel_y, accel_z) m/s^2\n
        Note: If linear acceleration has been disabled, retuns empty 3-tuple
        """
        return self.sensor.linear_acceleration


    def get_velocity(self):
        # todo
        prev_time = time.time()
        velocity = [0, 0, 0] # Will this even work or do we need starting velocity?
        for i in range(0, 10): # Max value is number of readings we want to sum over
            measurement = self.get_acceleration()
            cur_time = time.time()
            delta_t = cur_time - prev_time
            prev_time = cur_time
            velocity[0] += measurement[0] * delta_t
            velocity[1] += measurement[1] * delta_t
            velocity[2] += measurement[2] * delta_t
            time.sleep(0.1) # 100 Hz delay
        return (velocity[0], velocity[1], velocity[2])


    def get_gravity(self):
        """
        Input: None\n
        Output: Returns the gravity vector (gravity_x, gravity_y, gravity_z)\n
        Note: If gravity has been disabled, returns empty 3-tuple
        """
        return self.sensor.gravity


    def get_orientation(self):
        """
        Input: None\n
        Output: Returns the orientation in degrees ( , , )\n
        Note: If orientation has been disabled, returns empty 3-tuple
        """
        return self.sensor.euler


    def get_temperature(self):
        """
        Input: None\n
        Output: Returns the temperature in Celcius
        """
        return self.sensor.temperature


    def is_calibrated(self):
        """
        Input: None\n
        Output: Boolean indicating if sensor is calibrated
        """
        return self.sensor.calibrated


if __name__ == '__main__':
    bno = BNO()
    try:
        bno.initialize()
    except Exception as e:
        print(e)

    while True:
        print(f"Accel: {bno.get_acceleration()}")
        print(f"Accel w/o gravity: {bno.get_linear_acceleration()}")
        print(f"Velocity: {bno.get_velocity()}")
        print(f"Gravity: {bno.get_gravity()}")
        print(f"Orientation: {bno.get_orientation()}")
        print(f"Temperature: {bno.get_temperature()}")
        print(f"Calibrated: {bno.is_calibrated()}")

        time.sleep(1)



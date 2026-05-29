# Wrapper class for the INA260 Current Sensor
# API Reference: https://docs.circuitpython.org/projects/ina260/en/latest/api.html

#### STILL NEEDS TO BE TESTED!!! ####

import adafruit_ina260
import board
import busio
import time


class INA():
    """
    Simple wrapper for the Adafruit INA260.
    Only exposes current measurement functionality.
    """

    def __init__(self, address=0x40):
        """
        Initialize the INA260 sensor.
        Args:
            address (int): I2C address of the INA260.
                           Default is 0x40.
        """
        for i in range(10):
            try:
                self.i2c = busio.I2C(board.SCL, board.SDA)
                break
            except Exception as e:
                if i == 9:
                    raise print(f"Error initializing I2C for INA260: {e}")
                continue
        
        for i in range(10):
            try:
                self._sensor = adafruit_ina260.INA260(self.i2c, address=address)
                break
            except Exception as e:
                if i == 9:
                    raise print(f"Error initializing INA260: {e}")
                continue

    def get_current_ma(self):
        """
        Get the current in milliamps.
        Returns:
            float: Current in mA
        """
        for _ in range(8):
            current = 0
            try:
                current = self._sensor.current
            except Exception as e:
                print(f"[INA260] Error retrieving current: {e}")
        return current

    def get_current_a(self):
        """
        Get the current in amps.

        Returns:
            float: Current in A
        """
        current = 0
        try:
            current = self._sensor.current / 1000.0
        except Exception as e:
            print(f"[INA260] Error retrieving current: {e}")
        return current
    
    def is_stall(self):
        return self.get_current_a() > 5
    
    def stall_triple_check(self):
        for i in range(5):
            if not self.is_stall():
                return False
            time.sleep(0.5)
        return True

if __name__ == '__main__':
    ina = INA()
    while True:
        print(f"Current: {ina.get_current_ma()} mA")
        print(f"Current: {ina.get_current_a()} A")
        print(f"Is Stall: {ina.is_stall()}")
        time.sleep(0.5)
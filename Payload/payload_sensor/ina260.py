# Wrapper class for the INA260 Current Sensor
# API Reference: https://docs.circuitpython.org/projects/ina260/en/latest/api.html

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
        i2c = busio.I2C(board.SCL, board.SDA)
        self._sensor = INA260(i2c, address=address)

    def get_current_ma(self):
        """
        Get the current in milliamps.
        Returns:
            float: Current in mA
        """
        return self._sensor.current

    def get_current_a(self):
        """
        Get the current in amps.

        Returns:
            float: Current in A
        """
        return self._sensor.current / 1000.0
    
if __name__ == '__main__':
    ina = INA()
    while True:
        print(f"Current: {ina.get_current_ma()} mA")
        print(f"Current: {ina.get_current_a()} A")
        time.sleep(0.5)
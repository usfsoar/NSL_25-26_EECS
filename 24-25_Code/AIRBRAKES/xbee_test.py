import time
import serial
from digi.xbee.devices import XBeeDevice

PORT = "/dev/ttyS0"
BAUD = 9600

# Step 1: Manually enter bypass mode
ser = serial.Serial(PORT, BAUD)
time.sleep(0.1)
ser.write(b'\r')     # Wake it up
time.sleep(0.1)
ser.write(b'B')      # Enter bypass mode
time.sleep(0.5)
ser.close()

# Step 2: Now open it with Digi API
device = XBeeDevice(PORT, BAUD)
try:
    device.open()
    print("Connected to XBee!")
    print("64-bit address:", device.get_64bit_addr())
except Exception as e:
    print(f"[ERROR] {e}")
finally:
    if device.is_open():
        device.close()
import serial
import time

ser = serial.Serial('COM10', 115200)  # or COM3 on Windows
time.sleep(2)  # give it time to connect

ser.write(b'Hello Pi!\n')
while True:
    line = ser.readline().decode().strip()
    print(f"From Pi: {line}")

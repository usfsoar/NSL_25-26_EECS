import serial
import struct
import time
import math
import random

ser = serial.Serial('/dev/ttyGS0', 115200, timeout=0)
time.sleep(2)

def convert_Flap_Length_to_Angle(flaps_length):
    a = -0.0002
    b = 0.0309
    c = -(0.0877 + flaps_length)

    discriminant = b**2 - 4*a*c
    try:
        angle1 = round((-b + math.sqrt(discriminant)) / (2 * a), 1)
        angle2 = round((-b - math.sqrt(discriminant)) / (2 * a), 1)
    except:
        return 2.9

    if 2.9 <= angle1 <= 39.63:
        return angle1
    else:
        return angle2

def getSimFloat(req_byte1, req_byte2, expected_response_byte):
    ser.write(bytes([req_byte1, req_byte2]))
    timeout_start = time.time()
    while time.time() - timeout_start < 1.0:
        if ser.in_waiting:
            response_code = ser.read(1)
            if response_code and response_code[0] == expected_response_byte:
                float_bytes = ser.read(4)
                if len(float_bytes) == 4:
                    value = struct.unpack('<f', float_bytes)[0]
                    return value
                else:
                    return 44330.00
    return 44330.00

# === Static flap length ===

max_lenght = 0.7
min_length = 0
last_sent = 0
while True:
    try:
        # Get altitude (blocking like microcontroller)
        altitude = getSimFloat(0x03, 0x01, 0x03)
        print(f"Received altitude: {altitude:.2f}")

        # Send angle response
        if time.time() - last_sent > 2:
            last_sent = time.time()
            flaps_length = random.uniform(min_length, max_lenght)
            angle_to_send = int(convert_Flap_Length_to_Angle(flaps_length))
            payload = bytes([0x07, 0x01]) + struct.pack('<i', angle_to_send)
            ser.write(payload)
            print(f"Sent length: {flaps_length} -> angle: {angle_to_send}")

            # Read confirmation
            if ser.in_waiting:
                confirmation = ser.read(1)
                if confirmation and confirmation[0] == 0x07:
                    print("✅ Confirmed")

        time.sleep(0.2)

    except Exception as e:
        print("❌ Error:", e)
        time.sleep(1)

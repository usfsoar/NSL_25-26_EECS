import bmp580
import bno055
import time

bmp = bmp580.BMP()
bno = bno055.BNO()
try:
    bmp.initialize()
except Exception as e:
    print(e)

try:
    bno.initialize()
except Exception as e:
    print(e)


while True:
    print("\n---BMP580 SENSOR DATA---")
    print(f"Altitude: {bmp.get_altitude()}")
    print(f"Vertical Velocity: {bmp.get_vertical_velocity()}")
    print(f"Pressure: {bmp.get_pressure()}")
    print(f"Temperature: {bmp.get_temperature()}")

    print("\n---BNO055 SENSOR DATA---")
    print(f"Accel: {bno.get_acceleration()}")
    print(f"Accel w/o gravity: {bno.get_linear_acceleration()}")
    print(f"Velocity: {bno.get_velocity()}")
    print(f"Gravity: {bno.get_gravity()}")
    print(f"Orientation: {bno.get_orientation()}")
    print(f"Temperature: {bno.get_temperature()}")
    print(f"Calibrated: {bno.is_calibrated()}")
    time.sleep(1)


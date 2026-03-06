import bmp
import bno
import time

try:
    alt = bmp.BMP()
    accel = bno.BNO()
except:
    print('Failed')
    exit()
start_time = time.time()
elapsed_time = time.time()-start_time
while elapsed_time < 15:
    elapsed_time = time.time()-start_time
    try:
        print(f"Time: {elapsed_time}, Altitute: {alt.altitude()}, Pressure: {alt.pressure()}, Temperature: {alt.temperature()},\nAcceleration: {accel.acceleration()}, Linear Acceleration: {accel.linear_acceleration()}, Gravity: {accel.gravity()},\nMagnetic: {accel.magnetic()}, Gyro: {accel.gyro()}, Quaternion: {accel.quaternion()}")
    except:
        print("oops")
    time.sleep(0.5)

import time
import imu_sensor

start_time = time.time()
elapsed_time = time.time() - start_time
imu = imu_sensor.IMU()
imu.initialize_imu()
while True:
    dt = time.time() - start_time - elapsed_time
    elapsed_time = time.time()-start_time
    imu.acceleration()
    print(f"X: {imu.acceleration_x} Y: {imu.acceleration_y} Z: {imu.acceleration_z}")

    time.sleep(0.5)

import bmp_sensor
import imu_sensor
import state_machine
import csv_maker
import time
import pid
import servo_control
import config
import digital_twin as twin
import kalman_no_drag

initialized = True
if config.USE_SIM:
    twin.setup_sim()
else:
    bmp = bmp_sensor.BMP()
    if not bmp.initialize_bmp():
        initialized = False
    if initialized:
        bmp.set_sea_level(1014.5)

    bno = imu_sensor.IMU()
    if not bno.initialize_imu():
        initialized = False
if config.HARDWARE:
    servo = servo_control.SERVO(16, 3, 39)
    if not servo.intialize(12):
        initialized = False


target_height = 1280.16
prev_altitude = 0.0
start_time = time.time()
elapsed_time = time.time()-start_time
started_time = False
extra_dt = 0.05
control_loop = pid.PID(target_height, 0.0, 0.0, 0.0, 0.0)
flap_percent = state_machine.StateMachine(target_height, 0.0, 0.0, 0.0, 0.0, control_loop)
if not config.USE_SIM:
    bmp.set_sea_level(1014.5)
apogee = 0.0

path = f'/home/soar/airbrakes/CSVs/launch_data'
data = {
    'Time' : [],
    'Altitude' : [],
    'Velocity' : [],
    'Acceleration' : [],
    'State' : [],
    'Servo Angle' : []
}
if config.TESTING:
    csv = csv_maker.CSV(path,data)
else:
    with open('file_index.txt', 'r') as f:
        index = int(f.readline())
    csv = csv_maker.CSV(path,data, index)
    f.close()
    with open('file_index.txt', 'w') as f:
        f.write(str(index+1))
csv.make_csv()

error_count = 0

milestone = 200

first = True
prev_alt = 0
prev_vel = 0
prev_accel = 0

while True:
    dt = time.time() - start_time - elapsed_time
    elapsed_time = time.time() - start_time
    
    sensor_alt = 0
    sensor_vel = 0
    sensor_accel = 0

    if(not initialized or error_count>10):
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [3]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(3)
        if config.HARDWARE:
            servo.set_angle(3)
        time.sleep(extra_dt)
        continue
    if config.USE_SIM:
        altitude = twin.getSimulatedAltitude()
    else:
        altitude = bmp.get_altitude()
    sensor_alt = altitude
        


    if altitude is None:
        error_count += 1
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [3]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(3)
        else:
            servo.set_angle(3)
        time.sleep(extra_dt)
        continue
    if altitude > apogee:
        apogee = altitude
    
    if config.USE_SIM:
        _,_,acceleration = twin.getSimulatedAcceleration()
    else:
        acceleration = bno.get_acceleration_z()
    sensor_accel = acceleration
    if acceleration is None:
        error_count += 1
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [3]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(3)
        if config.HARDWARE:
            servo.set_angle(3)
        
        time.sleep(extra_dt)
        continue
    try:
        if config.USE_SIM:
            velocity = (altitude-prev_altitude)/dt
        else:
            velocity = bmp.get_velocity(altitude, prev_altitude, dt, error_count)
        sensor_vel = velocity
        


        filtered_alt, filtered_vel, filtered_accel = kalman_no_drag.process_sensor_data(sensor_alt, sensor_vel, sensor_accel, prev_alt, prev_vel, prev_accel, dt, first)
        prev_alt = filtered_alt
        prev_vel = filtered_vel
        prev_accel = filtered_accel
        first = False

        #flap_percent.update(elapsed_time, acceleration, altitude, velocity)
        flap_percent.update(elapsed_time, filtered_accel, filtered_alt, filtered_vel)

        flap_percent.states()
        current_state = flap_percent.current_state
        angle = flap_percent.servo_angle
        '''
        if current_state == 2 and not started_time:
            print("STARTED TIMER")
            time_from_two = elapsed_time
            started_time=True
        if started_time and (elapsed_time - time_from_two>300):
            break
        '''
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [altitude],
            'Velocity' : [velocity],
            'Acceleration' : [acceleration],
            'State' : [current_state],
            'Servo Angle' : [angle]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(angle)
        if config.HARDWARE:
            servo.set_angle(angle)
        print(f"Time: {elapsed_time} Altitude: {altitude} Velocity: {velocity} Acceleration: {acceleration} State: {current_state} Angle: {angle}")
        prev_altitude = altitude
        error_count=0
    except Exception as e:
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [3]
        }
        csv.update_csv(data)
        
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE:
            servo.set_angle(2.9)
        error_count += 1
        print(f"Error with state machine!: {e}")
        time.sleep(extra_dt)



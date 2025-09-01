import bmp_sensor
import imu_sensor
import mini_state_machine
import csv_maker
import time
import pid
import servo_control
import config
import digital_twin as twin
import XBeeTransmitter

initialized = True
telem = config.TELEM
if config.TELEM:
    xbee = XBeeTransmitter.XBeeTransmitter()
    for i in range(2):
        try:
            xbee.open_connection()
            break
        except:
            if i == 10:
                telem = False
            continue
    
if config.USE_SIM:
    twin.setup_sim()
else:
    bmp = bmp_sensor.BMP()
    if not bmp.initialize_bmp():
        initialized = False
    if initialized:
        bmp.set_sea_level(bmp.get_pressure())

    bno = imu_sensor.IMU()
    if not bno.initialize_imu():
        initialized = False
if config.HARDWARE: 
    servo = servo_control.SERVO(16, 3, 39)
    if not servo.initialize(12):
        initialized = False


target_height = 1.0
prev_altitude = 0.0
start_time = time.time()
elapsed_time = time.time()-start_time
started_time = False
extra_dt = 0.1
control_loop = pid.PID(target_height, 0.0, 0.0, 0.0, 0.0)
flap_percent = mini_state_machine.StateMachine(target_height, 0.0, 0.0, 0.0, 0.0, control_loop)
apogee = 0.0
if config.TELEM and telem:
    flap_percent.add_xbee(xbee)


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

while True:
    dt = time.time() - start_time - elapsed_time
    elapsed_time = time.time() - start_time

    if(not initialized or error_count>10):
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [2.9]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE:
            servo.set_angle(2.9)
        time.sleep(0.05)
        continue
    if config.USE_SIM:
        altitude = twin.getSimulatedAltitude()
    else:
        altitude = bmp.get_altitude()
    if altitude is None:
        error_count += 1
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [2.9]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE:
            servo.set_angle(2.9)
        time.sleep(0.05)
        continue
    if altitude > apogee:
        apogee = altitude
    
    if config.USE_SIM:
        _,_,acceleration = twin.getSimulatedAcceleration()
    else:
        acceleration = bno.get_acceleration_z()
    if acceleration is None:
        error_count += 1
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [2.9]
        }
        csv.update_csv(data)
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE:
            servo.set_angle(2.9)
        time.sleep(0.05)
        continue
    try:
        if config.USE_SIM:
            velocity = (altitude-prev_altitude)/dt
        else:
            velocity = bmp.get_velocity(altitude, prev_altitude, dt, error_count)
        flap_percent.update(elapsed_time, acceleration, altitude, velocity, apogee)
        flap_percent.states()

        current_state = flap_percent.current_state
        if config.TELEM and telem:
            if altitude > milestone:
                xbee.send_milestone(f"3,4,state,{current_state},time,{elapsed_time},altitude,{altitude}")
                milestone += 200
        if config.TELEM and telem and current_state == 4 and altitude < 30:
            xbee.close_connection()
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
        print(f"Time: {elapsed_time} Altitude: {altitude} Velocity: {velocity} Acceleration: {acceleration} State: {current_state} Angle: {angle} Apogee: {apogee}")
        prev_altitude = altitude
        error_count=0
        time.sleep(0.05)
    except Exception as e:
        
        data = {
            'Time' : [elapsed_time],
            'Altitude' : [0],
            'Velocity' : [0],
            'Acceleration' : [0],
            'State' : [0],
            'Servo Angle' : [2.9]
        }
        csv.update_csv(data)
        
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE:
            servo.set_angle(2.9)
        error_count += 1
        print(f"Error with state machine!: {e}")
        time.sleep(0.05)
        

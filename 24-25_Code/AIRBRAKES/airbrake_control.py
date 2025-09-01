import bmp_sensor
import imu_sensor
import state_machine
import csv_maker
import time
import pid
import servo_control
import config
import digital_twin as twin
import XBeeTransmitter

#Initialize telemetry/xbee
initialized_telem = config.TELEM
if config.TELEM:
    xbee = XBeeTransmitter.XBeeTransmitter()
    for i in range(2):
        try:
            xbee.open_connection()
            break
        except:
            if i == 10:
                initialized_telem = False
            continue

#Setup digital twin connection
if config.USE_SIM:
    twin.setup_sim()
#Initialize sensors
else:
    bmp = bmp_sensor.BMP()
    initialized_bmp = bmp.initialize_bmp()
    if initialized_bmp:
        bmp.set_sea_level(bmp.get_pressure())
    
    bno = imu_sensor.IMU()
    initialized_bno = bno.initialize_imu()
#Initialize Servo
if config.HARDWARE:
    servo = servo_control.SERVO(16,3,39)
    initialized_servo = servo.initialize()
    print(initialized_servo)


target_height = 1127.76
prev_altitude = 0.0
start_time = time.time()
elapsed_time = time.time()-start_time
started_time = False
dt = 0
extra_dt = 0.05
control_loop = pid.PID(target_height, 0.0, 0.0, 0.0, 0.0)
flap_percent = state_machine.StateMachine(target_height, 0.0, 0.0, 0.0, 0.0, control_loop)
apogee = 0.0

#Initialize xbee with state machine
if config.TELEM and initialized_telem:
    flap_percent.add_xbee(xbee)

#Create csv file
path = f'/home/soar/airbrakes/CSVs/launch_data'
data = {
    'Time' : [],
    'Altitude' : [],
    'Velocity' : [],
    'Acceleration X': [],
    'Acceleration Y': [],
    'Acceleration Z': [],
    'Gravity X': [],
    'Gravity Y': [],
    'Gravity Z': [],
    'Gyro X': [],
    'Gyro Y': [],
    'Gyro Z': [],
    'Magnet X': [],
    'Magnet Y': [],
    'Magnet Z': [],
    'Pressure': [],
    'Temperature': [],
    'Air Density': [],
    'State' : [],
    'Servo Angle' : [],
    'Predicted Apogeee' : []
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
failed = False
launch_time = 0
launched = False

#Main loop
while True:
    
    #Calculate time for 1 iteration
    
    if not failed:
        dt = time.time() - start_time - elapsed_time
    else:
        dt += time.time() - start_time - elapsed_time
    elapsed_time = time.time() - start_time

    failed = False

    #Get altituide and acceleration
    if config.USE_SIM:
        altitude = twin.getSimulatedAltitude()
        acceleration_x,acceleration_y,acceleration_z = twin.getSimulatedAcceleration()
    else:
        if initialized_bmp:
            altitude = bmp.get_altitude()
        else:
            altitude = None
        if initialized_bno:
            acceleration_x,acceleration_y,acceleration_z = bno.acceleration()
        else:
            acceleration_x, acceleration_y, acceleration_z = None

    #Check if altitude is valid
    if altitude is None:
        error_count += 1
        failed = True
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE and initialized_servo:
            servo.set_angle(2.9)
        altitude = "ERROR"
    else:
        #Update apogee
        if altitude > apogee:
            apogee = altitude

    #Calculate velocity
    if not failed:
        if config.USE_SIM:
            velocity = (altitude-prev_altitude)/dt
        else:
            velocity = bmp.get_velocity(altitude, prev_altitude, dt, error_count)
    else:
        velocity = "ERROR"

    #Check if acceleration is valid
    if acceleration_z is None or acceleration_x is None or acceleration_y is None:
        error_count += 1
        failed = True
        if config.USE_SIM:
            twin.setSimulatedAngle(2.9)
        if config.HARDWARE and initialized_servo:
            servo.set_angle(2.9)
        acceleration_x = "ERROR"
        acceleration_y = "ERROR"
        acceleration_z = "ERROR"
    
    #Start timeout after launch
    if launched and (elapsed_time - launch_time) > 300:
        break 
    
    #Begin control
    if not failed:
        try:
            #Update state machine
            flap_percent.update(elapsed_time, acceleration_z, altitude, velocity, apogee)
            flap_percent.states()

            #Get current state
            current_state = flap_percent.current_state

            #Check if launched
            if current_state == 1 and launch_time == 0:
                launch_time = elapsed_time
                launched = True

            #Transmit at milestones
            if initialized_telem:
                if altitude > milestone:
                    xbee.send_milestone(f"3,4,state,{current_state},time,{elapsed_time},altitude,{altitude}")
                    milestone += 200
            #Close xbee connection after landing
            if initialized_telem and current_state == 4 and altitude < 30:
                xbee.close_connection()

            if config.USE_SIM:
                grav_x, grav_y, grav_z = 0,0,0
                gyro_x, gyro_y, gyro_z = 0,0,0
                mag_x, mag_y, mag_z = 0,0,0
                pressure = 0
                temperature = 0
                air_density = 0
            else:
                grav_x, grav_y, grav_z = bno.get_gravity_x(), bno.get_gravity_y(), bno.get_gravity_z()
                if grav_x is None:
                    grav_x, grav_y, grav_z = 0,0,0

                gyro_x, gyro_y, gyro_z = bno.gyro()
                if gyro_x is None:
                    gyro_x, gyro_y, gyro_z = 0,0,0

                mag_x, mag_y, mag_z = bno.magnetometer()
                if mag_x is None:
                    mag_x, mag_y, mag_z = 0,0,0

                pressure = bmp.get_pressure()
                if pressure is None:
                    pressure = 0
                temperature = bmp.get_temperature()
                if temperature is None:
                    temperature = 0
                air_density = bmp.get_air_density()
                if air_density is None:
                    air_density = 0

            #Get servo angle
            angle = flap_percent.servo_angle

            #Get projected height from rk4
            if current_state == 2:
                proj_height = flap_percent.getProjectedHeight()
                if proj_height is None:
                    proj_height = 0
            else:
                proj_height = 0
            
            #Add data to csv
            data = {
            'Time' : [elapsed_time],
            'Altitude' : [altitude],
            'Velocity' : [velocity],
            'Acceleration X': [acceleration_x],
            'Acceleration Y': [acceleration_y],
            'Acceleration Z': [acceleration_z],
            'Gravity X': [grav_x],
            'Gravity Y': [grav_y],
            'Gravity Z': [grav_z],
            'Gyro X': [gyro_x],
            'Gyro Y': [gyro_y],
            'Gyro Z': [gyro_z],
            'Magnet X': [mag_x],
            'Magnet Y': [mag_y],
            'Magnet Z': [mag_z],
            'Pressure': [pressure],
            'Temperature': [temperature],
            'Air Density': [air_density],
            'State' : [current_state],
            'Servo Angle' : [angle],
            'Predicted Apogee' : [proj_height]
            }
            print("Updating csv")
            csv.update_csv(data)

            #Actuate servo
            if config.USE_SIM:
                twin.setSimulatedAngle(angle)
            if config.HARDWARE and initialized_servo:
                servo.set_angle(angle)

            prev_altitude = altitude
            error_count=0
            success = True

        except Exception as e:
            #Set servo angle to min
            if config.USE_SIM:
                twin.setSimulatedAngle(2.9)

            if config.HARDWARE and initialized_servo:
                servo.set_angle(2.9)

            error_count += 1
            print(f"Error with state machine!: {e}")
            angle = "ERROR"
            proj_height = "ERROR"
            current_state = "ERROR"
            success = False
            
        time.sleep(extra_dt)
    try:
        #Print data from current iteration
        print(f"""Time: {elapsed_time} Altitude: {altitude} Velocity: {velocity} Acceleration_x: {acceleration_x} Acceleration_y: 
            {acceleration_y} Acceleration_z: {acceleration_z} Gravity_x: {grav_x} Gravity_y: {grav_y} Gravity_z: {grav_z} 
            Gyro_x: {gyro_x} Gyro_y: {gyro_y} Gyro_z: {gyro_z} Magnet_x: {mag_x} Magnet_y: {mag_y} Magnet_z: {mag_z} 
            Pressure: {pressure} Temperature: {temperature} Air_Density: {air_density} State: {current_state} Angle: {angle} Projected Height: {proj_height}""")
    except Exception as e:
        print(e)
    

    

    

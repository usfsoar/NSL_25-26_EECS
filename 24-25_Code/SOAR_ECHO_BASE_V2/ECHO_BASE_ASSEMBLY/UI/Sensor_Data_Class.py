class SensorData:
    def __init__(self, time_stamp, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z, gps_nmea):
        self.time_stamp = time_stamp
        self.accel_x = accel_x
        self.accel_y = accel_y
        self.accel_z = accel_z
        self.linear_x = linear_x
        self.linear_y = linear_y
        self.linear_z = linear_z
        self.gravity_x = gravity_x
        self.gravity_y = gravity_y
        self.gravity_z = gravity_z
        self.quat_w = quat_w
        self.quat_x = quat_x
        self.quat_y = quat_y
        self.quat_z = quat_z
        self.gyro_x = gyro_x
        self.gyro_y = gyro_y
        self.gyro_z = gyro_z
        self.gps_nmea = gps_nmea    

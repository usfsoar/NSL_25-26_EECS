class SensorData:
    def __init__(
        self,
        sender_id: int = 0,
        rx_id: int = 0,
        hours: int = 0,
        minutes: int = 0,
        seconds: int = 0,
        microseconds: int = 0,
        accel_x: float = 0.0,
        accel_y: float = 0.0,
        accel_z: float = 0.0,
        linear_x: float = 0.0,
        linear_y: float = 0.0,
        linear_z: float = 0.0,
        gravity_x: float = 0.0,
        gravity_y: float = 0.0,
        gravity_z: float = 0.0,
        quat_w: float = 0.0,
        quat_x: float = 0.0,
        quat_y: float = 0.0,
        quat_z: float = 0.0,
        gyro_x: float = 0.0,
        gyro_y: float = 0.0,
        gyro_z: float = 0.0,
        altitude: float = 0.0,
        temp: float = 0.0,
        pressure: float = 0.0,
        status: str = "",
        lat: float = 0.0,
        n_s: str = "",
        long: float = 0.0,
        e_w: str = "",
    ):
        self.sender_id = sender_id
        self.rx_id = rx_id
        self.hours = hours
        self.minutes = minutes
        self.seconds = seconds
        self.microseconds = microseconds
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
        self.altitude = altitude
        self.temp = temp
        self.pressure = pressure
        self.status = status
        self.lat = lat
        self.n_s = n_s  # Updated attribute name
        self.long = long
        self.e_w = e_w  # Updated attribute name

    def __repr__(self):
        return (
            f"SensorDataPackage("
            f"SenderID: {self.sender_id}, RxID: {self.rx_id}, Time: {self.hours}:{self.minutes}:{self.seconds}.{self.microseconds}, "
            f"Acceleration: ({self.accel_x}, {self.accel_y}, {self.accel_z}), "
            f"Linear Acceleration: ({self.linear_x}, {self.linear_y}, {self.linear_z}), "
            f"Gravity: ({self.gravity_x}, {self.gravity_y}, {self.gravity_z}), "
            f"Quaternion: ({self.quat_w}, {self.quat_x}, {self.quat_y}, {self.quat_z}), "
            f"Gyro: ({self.gyro_x}, {self.gyro_y}, {self.gyro_z}), "
            f"Altitude: {self.altitude}, Temp: {self.temp}, Pressure: {self.pressure}, "
            f"Status: {self.status}, Lat: {self.lat} {self.n_s}, Long: {self.long} {self.e_w})"
        )
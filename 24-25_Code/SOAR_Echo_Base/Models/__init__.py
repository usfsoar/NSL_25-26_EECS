

class Barometer:
    def __init__(self, pressure=None, temperature=None, altitude=None):
        self.pressure = pressure
        self.temperature = temperature
        self.altitude = altitude

# Example usage:
# barometer_data = Barometer(pressure=1013.25, temperature=25.5, altitude=100.0)
# barometer_data.pressure will give you the pressure value
# barometer_data.temperature will give you the temperature value
# barometer_data.altitude will give you the altitude value
class IMU:
    def __init__(self, acceleration_x=None, acceleration_y=None, acceleration_z=None,
                 gyro_x=None, gyro_y=None, gyro_z=None, magnetic_x=None, magnetic_y=None, magnetic_z=None):
        self.acceleration_x = acceleration_x
        self.acceleration_y = acceleration_y
        self.acceleration_z = acceleration_z
        self.gyro_x = gyro_x
        self.gyro_y = gyro_y
        self.gyro_z = gyro_z
        self.magnetic_x = magnetic_x
        self.magnetic_y = magnetic_y
        self.magnetic_z = magnetic_z

# Example usage:
# imu_data = IMU(acceleration_x=0.1, acceleration_y=0.2, acceleration_z=9.8, gyro_x=1.5, gyro_y=2.0, gyro_z=0.5)
# imu_data.acceleration_x will give you the acceleration_x value
# imu_data.gyro_y will give you the gyro_y value
# Similarly for other properties
class LogMessage:
    def __init__(self, message: str, msg_type:str):
        self.message = message
        self.type = msg_type
    def to_json(self):
        return {"message": self.message, "type": self.type}

class SerialMessage:
    def __init__(self, device:str, message:str):
        self.device = device
        self.message = message
    def to_json(self):
        return {"device": self.device, "message": self.message}


class LoraPacket:
    def __init__(self, valid_data, address, length, data_bytes, checksum, rssi, snr):
        self.valid_data = valid_data
        self.address = address
        self.length = length
        self.data_bytes = data_bytes
        self.checksum = checksum
        self.rssi = rssi
        self.snr = snr
    def to_json(self):
        return {
            "valid_data": self.valid_data,
            "address": self.address,
            "length": self.length,
            "data_bytes": list(self.data_bytes),
            "checksum": self.checksum,
            "rssi": self.rssi,
            "snr": self.snr
        }
        
        
    def __str__(self):
        return f"LORA: Valid data:{self.valid_data}, Address Bytes: {self.address}, Length Bytes: {self.length}, Data Bytes: {self.data_bytes}, Checksum: {self.checksum}, RSSI: {self.rssi}, SNR: {self.snr}"

class LoraMessage:
    def __init__(self, device:str, packet:LoraPacket):
        self.device = device
        self.packet = packet
    def to_json(self):
        return {"device": self.device, "packet": self.packet.to_json()}

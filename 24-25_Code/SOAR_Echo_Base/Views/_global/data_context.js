//BAROMETER Frontend Class
export class Barometer {
	constructor(pressure=NULL, temperature=NULL, altitude=NULL){
		this.pressure=pressure;
		this.temperature=temperature;
		this.altitude=altitude;
	}
    static fromJSON(json){
        return new Barometer(
            json.pressure,
            json.temperature,
            json.altitude
        );
    }
}

//IMU Frontend Class
export class IMU {
	constructor(acceleration_x=NULL, acceleration_y=NULL, acceleration_z=NULL,
              gyro_x=NULL, gyro_y=NULL, gyro_z=NULL, magnetic_x=NULL, magnetic_y=NULL, magnetic_z=NULL){
		this.acceleration_x = acceleration_x
		this.acceleration_y = acceleration_y
		this.acceleration_z = acceleration_z
		this.gyro_x = gyro_x
		this.gyro_y = gyro_y
		this.gyro_z = gyro_z
		this.magnetic_x = magnetic_x
		this.magnetic_y = magnetic_y
		this.magnetic_z = magnetic_z
	}
    static fromJSON(json){
        return new IMU(
            json.acceleration_x,
            json.acceleration_y,
            json.acceleration_z,
            json.gyro_x,
            json.gyro_y,
            json.gyro_z,
            json.magnetic_x,
            json.magnetic_y,
            json.magnetic_z
        );
    }
}
export class LogMessage {
    constructor(message="", type=""){
        this.message = message;
        this.type = type;
    }
    static fromJSON(json){
        return new LogMessage(json.message, json.type);
    }
}
export class SerialMessage {
    constructor(message="", device=""){
        this.device = device;
        this.message = message;
    }
    static fromJSON(json){
        return new SerialMessage(json.message, json.device);
    }
}

/**
 * class LoraPacket:
    def __init__(self, valid_data, address, length, data_bytes, checksum, rssi, snr):
        self.valid_data = valid_data
        self.address = address
        self.length = length
        self.data_bytes = data_bytes
        self.checksum = checksum
        self.rssi = rssi
        self.snr = snr
    def to_json(self):
        return {"packet":{
            "valid_data": self.valid_data,
            "address": self.address,
            "length": self.length,
            "data_bytes": self.data_bytes,
            "checksum": self.checksum,
            "rssi": self.rssi,
            "snr": self.snr
        }}
 */

export class LoraPacket{
    constructor(valid_data=false, address="", length=0, data_bytes="", checksum="", rssi=0, snr=0){
        this.valid_data = valid_data;
        this.address = address;
        this.length = length;
        this.data_bytes = data_bytes;
        this.checksum = checksum;
        this.rssi = rssi;
        this.snr = snr;
    }
    static fromJSON(json){
        return new LoraPacket(
            json.valid_data,
            json.address,
            json.length,
            json.data_bytes,
            json.checksum,
            json.rssi,
            json.snr
        );
    }
    toStr(){
        return `LoraPacket: ${this.valid_data} ${this.address} ${this.length} ${this.data_bytes} ${this.checksum} ${this.rssi} ${this.snr}`;
    }
}
export class LoraMessage{
    constructor(device="", packet=NULL){
        this.device = device;
        this.packet = packet;
    }
    static fromJSON(json){
        return new LoraMessage(json.device, LoraPacket.fromJSON(json.packet));
    }
}
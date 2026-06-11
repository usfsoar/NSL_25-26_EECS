enum {
    MAX_DATA = 10 * 1024 /* KB */
};
#pragma pack(push, 1)
struct imu_packet
{
	float accel[3];
	float linear[3];
	float gravity[3];
	float quat[4];
	float gyro[3];
};

struct altimeter_packet
{
	float altitude;
	float temp;
	float pressure;
};

struct gps_packet
{
	char nmea[100];
};

struct kalman_packet
{
	int kalman_state;
	double kalman_altitude;
	double kalman_velocity;
	double kalman_acceleration;
};

enum SensorType : uint8_t
{
	IMU = 0,
	ALTIMETER,
	GPS,
	KALMAN
};

// unified sensor data structure
struct SensorData
{
	SensorType type;
	uint32_t bay_seq;
	union
	{
		imu_packet imu;
		altimeter_packet alt;
		gps_packet gps;
		kalman_packet kalman;
	} data;

	char timestamp[16];
};
#pragma pack(pop)
void dataToString(SensorData sdata, char* msg) {
	switch (sdata.type) {
		case IMU:
			snprintf(msg, MAX_DATA, "%d %s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", sdata.type, sdata.timestamp, 
                    sdata.data.imu.accel[0], sdata.data.imu.accel[1], sdata.data.imu.accel[2], 
                    sdata.data.imu.linear[0], sdata.data.imu.linear[1], sdata.data.imu.linear[2], 
                    sdata.data.imu.gravity[0], sdata.data.imu.gravity[1], sdata.data.imu.gravity[2], 
                    sdata.data.imu.quat[0], sdata.data.imu.quat[1], sdata.data.imu.quat[2], sdata.data.imu.quat[3],
                    sdata.data.imu.gyro[0], sdata.data.imu.gyro[1], sdata.data.imu.gyro[2]);
			break;
		case ALTIMETER:
			snprintf(msg, MAX_DATA, "%d %s,%f,%f,%f", sdata.type, sdata.timestamp,
                    sdata.data.alt.altitude, sdata.data.alt.temp, sdata.data.alt.pressure);
			break;
		case GPS:
			snprintf(msg, MAX_DATA, "%d %s,%s", sdata.type, sdata.timestamp, sdata.data.gps.nmea);
			break;
		case KALMAN:
			snprintf(msg, MAX_DATA, "%d %s, %d, %f,%f,%f", sdata.type, sdata.timestamp, sdata.data.kalman.kalman_state,
                    sdata.data.kalman.kalman_altitude, sdata.data.kalman.kalman_velocity, sdata.data.kalman.kalman_acceleration);
			break;
        default:
            snprintf(msg, MAX_DATA, "");
            break;
	}
}

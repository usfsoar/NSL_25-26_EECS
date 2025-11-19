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
	char* nmea;
};

enum SensorType
{
	IMU = 0,
	ALTIMETER,
	GPS
};

// unified sensor data structure
struct SensorData
{
	SensorType type;
	union
	{
		imu_packet imu;
		altimeter_packet alt;
		gps_packet gps;
	} data;
	String timestamp;
};

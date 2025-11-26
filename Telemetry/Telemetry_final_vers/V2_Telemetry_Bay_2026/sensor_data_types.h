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

struct kalman_packet
{
	double kalman_altitude;
	double kalman_velocity;
	double kalman_acceleration;
};

enum SensorType
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
	union
	{
		imu_packet imu;
		altimeter_packet alt;
		gps_packet gps;
		kalman_packet kalman;
	} data;
	String timestamp;
};

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
	char timestamp[16];
	union
	{
		imu_packet imu;
		altimeter_packet alt;
		gps_packet gps;
		kalman_packet kalman;
	} data;
};
#pragma pack(pop)
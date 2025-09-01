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

// or use char array for lora_command
struct lora_packet
{
	char lora_command[5];
};

enum SensorType
{
	IMU = 0,
	ALTIMETER,
	LORA
};

// unified sensor data structure
struct SensorData
{
	SensorType type;
	union
	{
		imu_packet imu;
		altimeter_packet alt;
		lora_packet lora;
	} data;
	uint8_t timestamp;
};

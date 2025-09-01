struct imu_packet
{
	int hours;
  int minutes;
  int seconds;
  int microseconds;
	float accel[3];
	float linear[3];
	float gravity[3];
	float quat[4];
	float gyro[3];
};

struct altimeter_packet
{
	int hours;
  int minutes;
  int seconds;
  int microseconds;
	float altitude;
	float temp;
	float pressure;
};

// struct battery_gauge
// {
//   int hours;
//   int minutes;
//   int seconds;
//   int microseconds;

// };

struct gps_packet 
{
	int hours;
  int minutes;
  int seconds;
  int microseconds;
  char* status;
  char* lat; 
  char* N_S;
  char* longitude; 
  char* E_W;
};

enum SensorType
{
	IMU = 0,
	ALTIMETER,
	GPS,
	// BAT
};

// unified sensor data structure
struct SensorData
{
	SensorType type;
	union
	{
		// battery_gauge bat;
		imu_packet imu;
		altimeter_packet alt;
		gps_packet gps;
	} data;
	// uint8_t timestamp;
};

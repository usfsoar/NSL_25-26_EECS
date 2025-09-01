struct imu_packet
{
	float accel[3];
	float linear[3];
	float gravity[3];
	float quat[4];
	float gyro[3];
};

// struct altimeter_packet
// {
// 	float altitude;
// 	float temp;
// 	float pressure;
// };

struct timer_packet 
{
  char* date;
  char* time;
};

struct gps_packet 
{
  char* status;
  char* lat; 
  char* N_S;
  char* longitude; 
  char* E_W;
};

// unified sensor data structure
struct SensorData
{
	union
	{
    timer_packet rtc;
		imu_packet imu;
		// altimeter_packet alt;
		gps_packet gps;
	} data;
	// uint8_t timestamp;
};

struct gps_packet
{
	int hours;
	int minutes;
	int seconds;
	int microseconds;
	char* status;
	float lat;
	char* N_S;
	float longitude;
	char* E_W;
};

enum SensorType
{
	GPS
	// BAT
};

// unified sensor data structure
struct SensorData
{
	SensorType type;
	union
	{
		gps_packet gps;
	} data;
	// uint8_t timestamp;
};


struct TelemetryStruct {
    int SenderID;
    int RxID;
    float hours;
    float minutes;
    float seconds;
    float microseconds;
    float altitude;
    char* Status;
    float lat;
    char* N_S;
    float longitude;
    char* E_W;
};
#ifndef CONFIG_H
#define CONFIG_H

#define DIGITAL_TWIN 0
#define NO_BUZZ 0
#define DEBUG_ALT 0
#define DEBUG_CSV 0

// Altimeter ------------------------------------
#define SEALEVELPRESSURE_HPA (1013.25)

// Queue Lengths -------------------------------
#define SD_CARD_QUEUE_LEN 10
#define LORAQU_LEN 5
#define RECEIVEQU_LEN 5
#define DATA_HNDL_QU_LEN 5

// Delays --------------------------------------
#define LORA_DELAY 500
#define LORA_READ_DELAY 2000
#define BMP_DELAY 500
#define BNO_DELAY 500
#define DATA_HANDLING_DELAY 100



// SD Card --------------------------------------
// extern const char* IMU_FILEPATH = "/imu.csv";
// extern const char* ALTIMETER_FILEPATH = "/altimeter.csv";
// extern const char* LORA_FILEPATH = "/lora.csv";

#endif // CONFIG_H


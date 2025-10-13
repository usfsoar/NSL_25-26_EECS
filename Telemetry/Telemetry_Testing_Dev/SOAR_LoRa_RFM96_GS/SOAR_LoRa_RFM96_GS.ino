#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "_config.h"
#include "sensor_data_types.h"
#include "SOAR_LoRa_RFM96.h"
#include "V1_SOAR_RTOS_SD_CARD.h"

int byteMax = 8192 * 2;
static QueueHandle_t SensQu;
SOAR_LoRa_RFM96 lora;
SOAR_SD_CARD sd(D0);

void ReceiveSensorTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = byteMax * 0.98;
    SensorData all_sensors;
    // Store in the SD Card -------------------------------------------------------------------
    char buffer[RH_RF95_MAX_MESSAGE_LEN];
    DataType dtype;
    if (lora.receiveTyped(buffer, sizeof(buffer), &dtype, 0)) {
     switch (dtype) {
      case IMU:
        sd.appendFile(IMU_FILEPATH, buffer);
        break;
      case ALTIMETER:
        sd.appendFile(ALTIMETER_FILEPATH, buffer);
        break;
      case GPS:
        sd.appendFile(GPS_FILEPATH, buffer);
        break;
      default:
      // Handle unexpected type
        break;
      }
    } else {
      Serial.println("Failed to receive data");
      }  
      }
    }

void setup() {
  Serial.begin(115200);

  lora.LoRa_begin();
  // Create the sensor data queue
  SensQu = xQueueCreate(10, sizeof(SensorData));

  sd.begin();
  sd.writeFile(IMU_FILEPATH, "hours, minutes, seconds, microseconds, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "hours, minutes, seconds, microseconds, altitude, temperature, pressure\n");
  sd.writeFile(GPS_FILEPATH, "hours, minutes, seconds, microseconds, status, latitude, North/South, longitude, East/West\n");

  // New itation string
  String initialIMU = "Initializing IMU";
  String initialALTIMETER = "Intializing Altimeter";
  String initialGPS = "Initializing GPS";
  String ifileIMU = IMU_FILEPATH;
  String ifileALTIMETER = ALTIMETER_FILEPATH;
  String ifileGPS = GPS_FILEPATH;
  
    // Starting new iteration
  initialIMU += "\n";
  initialALTIMETER += "\n";
  initialGPS += "\n";
  sd.appendFile(ifileIMU.c_str(), initialIMU.c_str());
  sd.appendFile(ifileALTIMETER.c_str(), initialALTIMETER.c_str());
  sd.appendFile(ifileGPS.c_str(), initialGPS.c_str());

  // Create the Read Sensor Task
  xTaskCreatePinnedToCore(
    ReceiveSensorTask,
    "Read Sensor Task",
    byteMax,
    NULL,
    3,
    NULL,
    app_cpu
  );
}

void loop() {

}
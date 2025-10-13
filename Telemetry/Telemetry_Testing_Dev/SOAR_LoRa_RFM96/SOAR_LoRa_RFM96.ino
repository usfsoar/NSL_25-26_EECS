#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "_config.h"
#include "sensor_data_types.h"
#include "SOAR_LoRa_RFM96.h"

int byteMax = 8192 * 2;
static QueueHandle_t SensQu;
SOAR_LoRa_RFM96 lora;

void GetIMUTask(void* Parameters) {
  while(1) {    
    // IMU Data -------------------------------------------------------------------------------
        SensorData imu_data;
        imu_data.type = IMU;
        int current_hours = 5;
        int current_minutes = 5;
        int current_seconds = 5;
        int current_microseconds = 5;
        imu_data.data.imu.hours = current_hours;
        imu_data.data.imu.minutes = current_minutes;
        imu_data.data.imu.seconds = current_seconds;
        imu_data.data.imu.microseconds = current_microseconds;
        // imu_sensor.loop_iterations++;

        // Initialize IMU Variables
        float accel[3];
        float lin_accel[3];
        float gravity[3];
        float quat[4];
        float gyro[3];

        // Update the struct
        imu_data.data.imu.accel[0] = 5;
        imu_data.data.imu.accel[1] = 5; // accel y
        imu_data.data.imu.accel[2] = 5;
        imu_data.data.imu.linear[0] = 5;
        imu_data.data.imu.linear[1] = 5;
        imu_data.data.imu.linear[2] = 5;
        imu_data.data.imu.gravity[0] = 5;
        imu_data.data.imu.gravity[1] = 5;
        imu_data.data.imu.gravity[2] = 5;
        imu_data.data.imu.quat[0] = 5;
        imu_data.data.imu.quat[1] = 5;
        imu_data.data.imu.quat[2] = 5;
        imu_data.data.imu.quat[3] = 5;
        imu_data.data.imu.gyro[0] = 5;
        imu_data.data.imu.gyro[1] = 5;
        imu_data.data.imu.gyro[2] = 5;
        xQueueSend(SensQu, &imu_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
} 
void GetALTTask(void* Parameters) {
  while(1) {
  // Altimeter Data -------------------------------------------------------------------------
      SensorData altimeter_data;
      altimeter_data.type = ALTIMETER;

      int current_hours = 5;
      int current_minutes = 5;
      int current_seconds = 5;
      int current_microseconds = 5;
      altimeter_data.data.alt.hours = current_hours;
      altimeter_data.data.alt.minutes = current_minutes;
      altimeter_data.data.alt.seconds = current_seconds;
      altimeter_data.data.alt.microseconds = current_microseconds;

      // Initialize Altimeter Variables
      float altimeter = 5;
      float temp = 5;
      float pressure = 5;

      // Update the struct
      altimeter_data.data.alt.altitude = altimeter;
      altimeter_data.data.alt.temp = temp;
      altimeter_data.data.alt.pressure = pressure;

      xQueueSend(SensQu, &altimeter_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void GetGPSTask(void* Parameters) {
  while(1) {
    // GPS Data -------------------------------------------------------------------------------
    // Get the GPS NMEA Sentence
        SensorData gps_data;
        gps_data.type = GPS;

        int current_hours = 5;
        int current_minutes = 5;
        int current_seconds = 5;
        int current_microseconds = 5;
        gps_data.data.gps.hours = current_hours;
        gps_data.data.gps.minutes = current_minutes;
        gps_data.data.gps.seconds = current_seconds;
        gps_data.data.gps.microseconds = current_microseconds;

        char stat[6];
        char lat[6];
        char N_S[1];
        char longitude[6];
        char E_W[1];
        char* i_status = "hi";
        char* i_lat = "hello";
        char* i_N_S = "hi";
        char* i_longitude = "hello";
        char* i_E_W = "hi";


        // Update the struct
        gps_data.data.gps.status = i_status;
        gps_data.data.gps.lat = i_lat;
        gps_data.data.gps.N_S = i_N_S;
        gps_data.data.gps.longitude = i_longitude;
        gps_data.data.gps.E_W = i_E_W;

        xQueueSend(SensQu, &gps_data, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
} 

void ReadSensorTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = byteMax * 0.98;
    SensorData all_sensors;
    // Store in the SD Card -------------------------------------------------------------------
    if (xQueueReceive(SensQu, &all_sensors, portMAX_DELAY) == pdTRUE) {
      String data_str_IMU;
      String data_str_ALTIMETER;
      String data_str_GPS;
      switch (all_sensors.type) {
        case IMU:
        data_str_IMU = 
        String(all_sensors.data.imu.hours) + "," + String(all_sensors.data.imu.minutes) + "," + String(all_sensors.data.imu.seconds) + "," + String(all_sensors.data.imu.microseconds) + "," +
        String(all_sensors.data.imu.accel[0]) + "," + String(all_sensors.data.imu.accel[1]) + "," + String(all_sensors.data.imu.accel[2]) + "," +
        String(all_sensors.data.imu.linear[0]) + "," + String(all_sensors.data.imu.linear[1]) + "," + String(all_sensors.data.imu.linear[2]) + "," +
        String(all_sensors.data.imu.gravity[0]) + "," + String(all_sensors.data.imu.gravity[1]) + "," + String(all_sensors.data.imu.gravity[2]) + "," +
        String(all_sensors.data.imu.quat[0]) + "," + String(all_sensors.data.imu.quat[1]) + "," + String(all_sensors.data.imu.quat[2]) + "," + String(all_sensors.data.imu.quat[3]) + "," +
        String(all_sensors.data.imu.gyro[0]) + "," + String(all_sensors.data.imu.gyro[1]) + "," + String(all_sensors.data.imu.gyro[2]);
        
        if (!lora.sendTypedString(TYPE_IMU, data_str_IMU.c_str())) {
          Serial.println("Failed to send IMU data");
        }
        break;
        
        case ALTIMETER: 
        data_str_ALTIMETER = 
        String(all_sensors.data.alt.hours) + "," + String(all_sensors.data.alt.minutes) + "," + String(all_sensors.data.alt.seconds) + "," + String(all_sensors.data.alt.microseconds) + "," +
        String(all_sensors.data.alt.altitude) + "," + String(all_sensors.data.alt.temp) + "," + String(all_sensors.data.alt.pressure);
        
        if (!lora.sendTypedString(TYPE_ALTIMETER, data_str_ALTIMETER.c_str())) {
          Serial.println("Failed to send Altimeter data");
        }
        break;

        case GPS:
        data_str_GPS = 
        String(all_sensors.data.gps.hours) + "," + String(all_sensors.data.gps.minutes) + "," + String(all_sensors.data.gps.seconds) + "," + String(all_sensors.data.gps.microseconds) + "," +
        String(all_sensors.data.gps.status) + "," + String(all_sensors.data.gps.lat) + "," + String(all_sensors.data.gps.N_S) + "," + String(all_sensors.data.gps.longitude) + "," + String(all_sensors.data.gps.E_W);
        
        if (!lora.sendTypedString(TYPE_GPS, data_str_GPS.c_str())) {
          Serial.println("Failed to send GPS data");
        }
        break;
      }

      data_str_IMU += "\n";
      data_str_ALTIMETER += "\n";
      data_str_GPS += "\n";
      }
  }
  }

void setup() {
  Serial.begin(115200);

  lora.LoRa_begin();
  // Create the sensor data queue
  SensQu = xQueueCreate(10, sizeof(SensorData));

  xTaskCreatePinnedToCore(
    GetIMUTask,
    "Get IMU Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );
  xTaskCreatePinnedToCore(
    GetALTTask,
    "Get Altimeter Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    GetGPSTask,
    "Get GPS Task",
    byteMax,
    NULL,
    2,
    NULL,
    app_cpu
  );

  // Create the Read Sensor Task
  xTaskCreatePinnedToCore(
    ReadSensorTask,
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
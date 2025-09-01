// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "sensor_data_types.h"

// Globals --------------------------------------
static const uint8_t Data_HndlQu_len = 5;
static QueueHandle_t Data_HndlQu;
static float init_altitude = 0;

// Tasks ----------------------------------------
void data_processing(void *parameters) {
  while (1) {
    SensorData sensorData;
    if (xQueueReceive(Data_HndlQu, (void *)&sensorData, portMAX_DELAY) == pdTRUE) {
      // INSERT LOGIC TO SEND TO SD CARD
      Serial.println("Data Processed to SD");

      // Check if the altimeter data is valid
      if (sensorData.type == ALTIMETER) {
        if (sensorData.data.alt.altitude == 0 &&  sensorData.data.alt.temp == 0 && sensorData.data.alt.pressure == 0) {
          Serial.println("Invalid data");
          // char response[32] = "Invalid altimeter data";
          // xQueueSend(txCritQu, &response, portMAX_DELAY);
        }
      }

      // Check if the IMU data is valid
      if (sensorData.type == IMU) {
        if (
          sensorData.data.imu.accel[0] == 0 && sensorData.data.imu.accel[1] == 0 && sensorData.data.imu.accel[2] == 0
          && sensorData.data.imu.linear[0] == 0 && sensorData.data.imu.linear[1] == 0 && sensorData.data.imu.linear[2] == 0
          && sensorData.data.imu.gravity[0] == 0 && sensorData.data.imu.gravity[1] == 0 && sensorData.data.imu.gravity[2] == 0
          && sensorData.data.imu.quat[0] == 0 && sensorData.data.imu.quat[1] == 0 && sensorData.data.imu.quat[2] == 0 && sensorData.data.imu.quat[3] == 0
          && sensorData.data.imu.gyro[0] == 0 && sensorData.data.imu.gyro[1] == 0 && sensorData.data.imu.gyro[2] == 0
          ) {
          Serial.println("Invalid data");
          // char response[32] = "Invalid IMU data";
          // xQueueSend(txCritQu, &response, portMAX_DELAY);
        }
      }

      // Check if the altitude has reached a critical threshold
      // Critical thresholds are defined in the config file

      if (sensorData.type == ALTIMETER) {
        // First check if the altitude is decreasing
        // Send a crit message once altitude is detected to be decreasing
        if (sensorData.data.alt.altitude < 0) {
          Serial.println("Altitude is decreasing");
          // char response[32] = "Altitude is decreasing";
          // xQueueSend(txCritQu, &response, portMAX_DELAY);
        }

        // Check if the altitude is above 5000m
          if (sensorData.data.alt.altitude > 5000) {
            Serial.println("Altitude is above 5000m");
            // char response[32] = "Altitude is above 5000m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 4000m
          if (sensorData.data.alt.altitude > 4000) {
            Serial.println("Altitude is above 4000m");
            // char response[32] = "Altitude is above 4000m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 3000m
          if (sensorData.data.alt.altitude > 3000) {
            Serial.println("Altitude is above 3000m");
            // char response[32] = "Altitude is above 3000m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 2000m
          if (sensorData.data.alt.altitude > 2000) {
            Serial.println("Altitude is above 2000m");
            // char response[32] = "Altitude is above 2000m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 1000m
          if (sensorData.data.alt.altitude > 1000) {
            Serial.println("Altitude is above 1000m");
            // char response[32] = "Altitude is above 1000m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 500m
          if (sensorData.data.alt.altitude > 500) {
            Serial.println("Altitude is above 500m");
            // char response[32] = "Altitude is above 500m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 400m
          if (sensorData.data.alt.altitude > 400) {
            Serial.println("Altitude is above 400m");
            // char response[32] = "Altitude is above 400m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 300m
          if (sensorData.data.alt.altitude > 300) {
            Serial.println("Altitude is above 300m");
            // char response[32] = "Altitude is above 300m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 200m
          if (sensorData.data.alt.altitude > 200) {
            Serial.println("Altitude is above 200m");
            // char response[32] = "Altitude is above 200m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }

        // Check if the altitude is above 100m
          if (sensorData.data.alt.altitude > 100) {
            Serial.println("Altitude is above 100m");
            // char response[32] = "Altitude is above 100m";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }
        
        // Check if the altitude is above initial altitude
          if (sensorData.data.alt.altitude > init_altitude) {
            Serial.println("Altitude is above initial altitude");
            // char response[64] = "Altitude is above initial altitude";
            // xQueueSend(txCritQu, &response, portMAX_DELAY);
          }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // Handles to tasks
  TaskHandle_t DtaTskHndl = NULL;

  // Arbitrary delay to make sure the serial monitor is open
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("Data handling task");

  // Queues ------------------------------
  Data_HndlQu = xQueueCreate(Data_HndlQu_len, sizeof(SensorData));
  if (Data_HndlQu == NULL)
  {
    Serial.println("Error creating queues");
    while (1);
  }

  // Tasks ----------------------------------------
  xTaskCreatePinnedToCore(
      data_processing,
      "Data Handling Task",
      2048,
      NULL,
      2,
      &DtaTskHndl,
      app_cpu);

  // vTaskDelete(NULL);
}

void loop()
{
  
  SensorData sample_val_data;

  sample_val_data.type = ALTIMETER;
  sample_val_data.data.alt.altitude = 110.0;
  sample_val_data.data.alt.temp = 25.0;
  sample_val_data.data.alt.pressure = 1000.0;
  sample_val_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);
  xQueueSend(Data_HndlQu, &sample_val_data, portMAX_DELAY);
  // NOTE -----------------------------
  // This is an arbitrary delay of 5 seconds to simulate the crit task running less frequently
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  // ----------------------------------
}

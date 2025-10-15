#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "sensor_data_types.h"
#include "_config.h"

SOAR_SD_CARD sd_card(0);
static QueueHandle_t sdStoreQu; // Global queue handle

void write_sd_file_headers() {
  Serial.println("Writing SD file headers");
  sd_card.deleteFile(IMU_FILEPATH);
  sd_card.deleteFile(ALTIMETER_FILEPATH);
  sd_card.deleteFile(LORA_FILEPATH);

  sd_card.writeFile(IMU_FILEPATH, "time_stamp, accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z\n");

  sd_card.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd_card.writeFile(LORA_FILEPATH, "time_stamp,lora_command\n");
}

// SD card writing task
void sd_card_task(void* parameter) {
  Serial.println("SD card task started");
  SensorData sensor_data;
  while (1) {
    if (xQueueReceive(sdStoreQu, &sensor_data, portMAX_DELAY) == pdTRUE) {
      // Create appropriate filename based on sensor type
      Serial.println("Beginning sd storing process, received from qu");
      String filename;
      String data_str;
      switch (sensor_data.type) {
      case IMU:
        filename = IMU_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," +
          String(sensor_data.data.imu.accel[0]) + "," + String(sensor_data.data.imu.accel[1]) + "," + String(sensor_data.data.imu.accel[2]) + "," +
          String(sensor_data.data.imu.linear[0]) + "," + String(sensor_data.data.imu.linear[1]) + "," + String(sensor_data.data.imu.linear[2]) + "," +
          String(sensor_data.data.imu.gravity[0]) + "," + String(sensor_data.data.imu.gravity[1]) + "," + String(sensor_data.data.imu.gravity[2]) + "," +
          String(sensor_data.data.imu.quat[0]) + "," + String(sensor_data.data.imu.quat[1]) + "," + String(sensor_data.data.imu.quat[2]) + "," + String(sensor_data.data.imu.quat[3]) + "," +
          String(sensor_data.data.imu.gyro[0]) + "," + String(sensor_data.data.imu.gyro[1]) + "," + String(sensor_data.data.imu.gyro[2]);
        break;
      case ALTIMETER:
        filename = IMU_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.alt.altitude) + "," + String(sensor_data.data.alt.temp) + "," + String(sensor_data.data.alt.pressure);
        break;
      case LORA:
        filename = IMU_FILEPATH;
        data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.lora.lora_command);
        break;
      }
      

      // Write data to SD card
      data_str += "\n";
      sd_card.appendFile(filename.c_str(), data_str.c_str());
    }
  }
}

void setup()
{
  Serial.begin(115200);

  // Create sensor queue
  sdStoreQu = xQueueCreate(QUEUE_LEN, sizeof(SensorData));
  
  // Initialize SD card
  sd_card.begin();

  // Delete csv files and write csv header rows
  write_sd_file_headers();

  // Create SD card writing task
  xTaskCreate(
    sd_card_task,
    "SD Card Task",
    4096,
    NULL,
    2,
    NULL);
}

void loop()
{
  // Example: Create sensor data and send to queue
  SensorData altimeter_data;

  // Fill with IMU data
  altimeter_data.type = ALTIMETER;
  altimeter_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);
  altimeter_data.data.alt.altitude = 100.0;
  altimeter_data.data.alt.temp = 25.0;
  altimeter_data.data.alt.pressure = 1013.25;

  // Send to queue
  xQueueSend(sdStoreQu, &altimeter_data, portMAX_DELAY);

  vTaskDelay(pdMS_TO_TICKS(QUEUE_RECEIVE_DELAY));

  SensorData imu_data;
  // BNO: Initializing variables
  float accel[3];
  float lin_accel[3];
  float gravity[3];
  float quat[4];
  float gyro[3];

  // BNO: Getting data values
  accel[0] = 1.0;
  accel[1] = 2.0;
  accel[2] = 3.0;
  lin_accel[0] = 4.0;
  lin_accel[1] = 5.0;
  lin_accel[2] = 6.0;
  gravity[0] = 7.0;
  gravity[1] = 8.0;
  gravity[2] = 9.0;
  quat[0] = 0.1;
  quat[1] = 0.2;
  quat[2] = 0.3;
  quat[3] = 0.4;
  gyro[0] = 10.0;
  gyro[1] = 11.0;
  gyro[2] = 12.0;

  // Filling out struct to send to data handling task
  imu_data.type = IMU;
  imu_data.data.imu.accel[0] = accel[0];
  imu_data.data.imu.accel[1] = accel[1];
  imu_data.data.imu.accel[2] = accel[2];
  imu_data.data.imu.linear[0] = lin_accel[0];
  imu_data.data.imu.linear[1] = lin_accel[1];
  imu_data.data.imu.linear[2] = lin_accel[2];
  imu_data.data.imu.gravity[0] = gravity[0];
  imu_data.data.imu.gravity[1] = gravity[1];
  imu_data.data.imu.gravity[2] = gravity[2];
  imu_data.data.imu.quat[0] = quat[0];
  imu_data.data.imu.quat[1] = quat[1];
  imu_data.data.imu.quat[2] = quat[2];
  imu_data.data.imu.quat[3] = quat[3];
  imu_data.data.imu.gyro[0] = gyro[0];
  imu_data.data.imu.gyro[1] = gyro[1];
  imu_data.data.imu.gyro[2] = gyro[2];
  imu_data.timestamp = (xTaskGetTickCount() / portTICK_PERIOD_MS);

  // Send to the data handling queue directly
  xQueueSend(sdStoreQu, &imu_data, portMAX_DELAY);


}
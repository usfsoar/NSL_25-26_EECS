#include "V1_SOAR_RTOS_SD_CARD.h"
#include "sensor_data_types.h"
#include "_config.h"

SOAR_SD_CARD sd_card(254, true);  // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);            // External, use SPI

void write_sd_file_headers(SOAR_SD_CARD& sd) {
  Serial.println("Writing SD file headers");
  sd.deleteFile(IMU_FILEPATH);
  sd.deleteFile(ALTIMETER_FILEPATH);
  sd.deleteFile(LORA_FILEPATH);

  sd.writeFile(IMU_FILEPATH, "time_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd.writeFile(LORA_FILEPATH, "time_stamp,lora_command\n");
}

void writeSensorDataToSD(SOAR_SD_CARD& sd, SensorData& sensor_data, const char* card_name) {
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
      filename = ALTIMETER_FILEPATH;
      data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.alt.altitude) + "," + String(sensor_data.data.alt.temp) + "," + String(sensor_data.data.alt.pressure);
      break;
      
    case LORA:
      filename = LORA_FILEPATH;
      data_str = String(sensor_data.timestamp) + "," + String(sensor_data.data.lora.lora_command);
      break;
  }
  
  data_str += "\n";
  sd.appendFile(filename.c_str(), data_str.c_str());
  Serial.print("Data written to ");
  Serial.print(card_name);
  Serial.print(": ");
  Serial.println(filename);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  delay(1000);
  
  // Initialize both SD cards
  Serial.println("Initializing SD card 1 (pin 254)...");
  sd_card.begin();
  
  Serial.println("Initializing SD card 2 (pin 10)...");
  sd_card2.begin();

  // Write headers to both cards
  Serial.println("Writing headers to SD card 1...");
  write_sd_file_headers(sd_card);
  
  Serial.println("Writing headers to SD card 2...");
  write_sd_file_headers(sd_card2);
  
  Serial.println("Setup complete!");
}

void loop() {
  // Create altimeter data
  SensorData altimeter_data;
  altimeter_data.type = ALTIMETER;
  altimeter_data.timestamp = millis();
  altimeter_data.data.alt.altitude = 100.0;
  altimeter_data.data.alt.temp = 25.0;
  altimeter_data.data.alt.pressure = 1013.25;
  
  // Write to both SD cards with delay between
  Serial.println("Writing altimeter data...");
  writeSensorDataToSD(sd_card, altimeter_data, "Card1");
  delay(50);  // Give first card time to finish
  writeSensorDataToSD(sd_card2, altimeter_data, "Card2");

  delay(100);

  // Create IMU data
  SensorData imu_data;
  imu_data.type = IMU;
  imu_data.timestamp = millis();
  
  imu_data.data.imu.accel[0] = 1.0;
  imu_data.data.imu.accel[1] = 2.0;
  imu_data.data.imu.accel[2] = 3.0;
  imu_data.data.imu.linear[0] = 4.0;
  imu_data.data.imu.linear[1] = 5.0;
  imu_data.data.imu.linear[2] = 6.0;
  imu_data.data.imu.gravity[0] = 7.0;
  imu_data.data.imu.gravity[1] = 8.0;
  imu_data.data.imu.gravity[2] = 9.0;
  imu_data.data.imu.quat[0] = 0.1;
  imu_data.data.imu.quat[1] = 0.2;
  imu_data.data.imu.quat[2] = 0.3;
  imu_data.data.imu.quat[3] = 0.4;
  imu_data.data.imu.gyro[0] = 10.0;
  imu_data.data.imu.gyro[1] = 11.0;
  imu_data.data.imu.gyro[2] = 12.0;

  // Write to both SD cards with delay between
  Serial.println("Writing IMU data...");
  writeSensorDataToSD(sd_card, imu_data, "Card1");
  delay(50);  // Give first card time to finish
  writeSensorDataToSD(sd_card2, imu_data, "Card2");
  
  delay(1000);
}
/*
 * V1_Telemetry_Bay_2026.ino
 * 
 * Integrated Telemetry System for Teensy 4.1
 * Reads data from IMU (BNO085), Barometer (BMP581), and GPS sensors
 * Logs all data to SD card in CSV format
 * 
 * No RTOS - uses simple sequential polling in loop()
 */

#include "V1_SOAR_RTOS_SD_CARD.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "sensor_data_types.h"
#include "_config.h"
#include "RTC_Test.h"
#include "SOAR_BMP581.h"
#include "SOAR_BNO085.h"
#include <math.h>

SOAR_SD_CARD sd_card(254, true);  // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);            // External, use SPI
SOAR_RTC rtc;
SOAR_RTOS_GPS gps2;
Adafruit_GPS gps_hw(&Wire1);
BMP581Sensor barometer;
SOAR_BNO085 imu;

float altitude;
float pressure;
float temperature;
float i_altitude;

char dataBuffer[512];

void write_sd_file_headers(SOAR_SD_CARD& sd) {
  Serial.println("Writing SD file headers");
  sd.deleteFile(IMU_FILEPATH);
  sd.deleteFile(ALTIMETER_FILEPATH);
  sd.deleteFile(GPS_FILEPATH);

  sd.writeFile(IMU_FILEPATH, "time_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd.writeFile(GPS_FILEPATH, "time_stamp,gps_data\n");
}

void writeToBothCards(const char* filename, const char* data) {
  sd_card.appendFile(filename, data);
  delay(50);
  sd_card2.appendFile(filename, data);
}

void writeSensorDataToSD(SensorData& sensor_data) {
  const char* filename;
  int len = 0;
  
  switch (sensor_data.type) {
    case IMU:
      filename = IMU_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer),
        "%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.imu.accel[0], sensor_data.data.imu.accel[1], sensor_data.data.imu.accel[2],
        sensor_data.data.imu.linear[0], sensor_data.data.imu.linear[1], sensor_data.data.imu.linear[2],
        sensor_data.data.imu.gravity[0], sensor_data.data.imu.gravity[1], sensor_data.data.imu.gravity[2],
        sensor_data.data.imu.quat[0], sensor_data.data.imu.quat[1], sensor_data.data.imu.quat[2], sensor_data.data.imu.quat[3],
        sensor_data.data.imu.gyro[0], sensor_data.data.imu.gyro[1], sensor_data.data.imu.gyro[2]);
      break;
      
    case ALTIMETER:
      filename = ALTIMETER_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%lu,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.alt.altitude,
        sensor_data.data.alt.temp,
        sensor_data.data.alt.pressure);
      break;
      
    case GPS:
      filename = GPS_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%lu,%s\n",
        sensor_data.timestamp,
        sensor_data.data.gps.nmea);
      break;
  }
  
  if (len > 0 && len < sizeof(dataBuffer)) {
    writeToBothCards(filename, dataBuffer);
    Serial.print("Data written: ");
    Serial.println(filename);
  }
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

  Wire1.begin();
  Wire1.setClock(100000);
  gps2.setup();

  Wire2.begin();
  Wire2.setClock(400000);
  barometer.begin();

  delay(1000);
  i_altitude = barometer.get_altitude();
  
  Serial.println("Setup complete!");
}

void loop() {
  // Get timestamp
  String timestamp = rtc.getTimestamp(true);

  // Read GPS data
  char nmea[100];
  gps2.GET_NMEA(nmea);
  SensorData gps_data;
  gps_data.type = GPS;
  gps_data.timestamp = timestamp;
  gps_data.data.gps.nmea = nmea;

  Serial.println("Writing gps data...");
  writeSensorDataToSD(gps_data);
  delay(100);

  // Read altimeter data
  altitude = barometer.get_altitude() - i_altitude;
  pressure = barometer.get_pressure();
  temperature = barometer.get_temperature();
  if (altitude == 0) {
      Serial.println("Altitude is 0, retrying...");
      barometer.begin();
  }
  
  SensorData altimeter_data;
  altimeter_data.type = ALTIMETER;
  altimeter_data.timestamp = timestamp;
  altimeter_data.data.alt.altitude = altitude;
  altimeter_data.data.alt.temp = temperature;
  altimeter_data.data.alt.pressure = pressure;
  
  Serial.println("Writing altimeter data...");
  writeSensorDataToSD(altimeter_data);
  delay(100);

  // Read IMU data
  imu.update();
  SensorData imu_data;
  imu_data.type = IMU;
  imu_data.timestamp = timestamp;
  imu_data.data.imu.accel[0] = imu.sensorData.acceleration.x;
  imu_data.data.imu.accel[1] = imu.sensorData.acceleration.y;
  imu_data.data.imu.accel[2] = imu.sensorData.acceleration.z;
  imu_data.data.imu.linear[0] = imu.sensorData.linearAcceleration.x;
  imu_data.data.imu.linear[1] = imu.sensorData.linearAcceleration.y;
  imu_data.data.imu.linear[2] = imu.sensorData.linearAcceleration.z;
  imu_data.data.imu.gravity[0] = imu.sensorData.gravity.x;
  imu_data.data.imu.gravity[1] = imu.sensorData.gravity.y;
  imu_data.data.imu.gravity[2] = imu.sensorData.gravity.z;
  imu_data.data.imu.quat[0] = imu.sensorData.orientation.w;
  imu_data.data.imu.quat[1] = imu.sensorData.orientation.x;
  imu_data.data.imu.quat[2] = imu.sensorData.orientation.y;
  imu_data.data.imu.quat[3] = imu.sensorData.orientation.z;
  imu_data.data.imu.gyro[0] = imu.sensorData.gyroscope.x;
  imu_data.data.imu.gyro[1] = imu.sensorData.gyroscope.y;
  imu_data.data.imu.gyro[2] = imu.sensorData.gyroscope.z;

  Serial.println("Writing IMU data...");
  writeSensorDataToSD(imu_data);
  delay(50);

  Serial.println("GPS: " + String(nmea));
  Serial.println("Altitude: " + String(altitude));
  Serial.println("Pressure: " + String(pressure));
  Serial.println("Temperature: " + String(temperature));
  snprintf(dataBuffer, sizeof(dataBuffer),
      "IMU: %.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
      imu_data.data.imu.accel[0], imu_data.data.imu.accel[1], imu_data.data.imu.accel[2],
      imu_data.data.imu.linear[0], imu_data.data.imu.linear[1], imu_data.data.imu.linear[2],
      imu_data.data.imu.gravity[0], imu_data.data.imu.gravity[1], imu_data.data.imu.gravity[2],
      imu_data.data.imu.quat[0], imu_data.data.imu.quat[1], imu_data.data.imu.quat[2], imu_data.data.imu.quat[3],
      imu_data.data.imu.gyro[0], imu_data.data.imu.gyro[1], imu_data.data.imu.gyro[2]);
  Serial.println(dataBuffer);
  
  delay(1000);
}
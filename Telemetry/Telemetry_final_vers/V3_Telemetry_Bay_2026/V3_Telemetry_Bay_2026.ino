/*
 * V2_Telemetry_Bay_2026.ino
 * 
 * Integrated Telemetry System for Teensy 4.1
 * Reads data from IMU (BNO085), Barometer (BMP581), and GPS sensors
 * Integrated Kalman filter
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
#include "kalman.h"
#include "matrix.h"
#include "extras.h"

SOAR_SD_CARD sd_card(254, true);  // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);            // External, use SPI
SOAR_RTC rtc;
SOAR_RTOS_GPS gps2;               // Wire 1
Adafruit_GPS gps_hw(&Wire1);
BMP581Sensor barometer;           // Wire 2
SOAR_BNO085 imu;                  // Wire

matrix * quat;
matrix * dir;
matrix * acc;
double magnitude;
double kalman_altitude;
double kalman_velocity;
double kalman_acceleration;
float altitude;
float pressure;
float temperature;
float i_altitude;
const float max_altitude = 1;
const float max_alt_tol = 0.5;
int stage = 0;
float alt_diff;

const double dt = 0.05; /* must be accurate to data rate */
const double sigma_j = 0.2; /* process StdDev: TUNED */
const double sigma_s = 0.1666667; /* altitude reading StdDev */
const double sigma_a = 0.179; /* acceleration reading StdDev */
const int states = 3;
const int observations = 2;
const double MIN_ALT = 1; /* trust sensors below this altitude [m]: allows filter to adapt quickly */
const double APOGEE = 3048; // [m] 10k ft
kalmanFilter *filter = NULL;

char dataBuffer[512];

void write_sd_file_headers(SOAR_SD_CARD& sd) {
  Serial.println("Writing SD file headers");
  sd.deleteFile(IMU_FILEPATH);
  sd.deleteFile(ALTIMETER_FILEPATH);
  sd.deleteFile(GPS_FILEPATH);
  sd.deleteFile(KALMAN_FILEPATH);

  sd.writeFile(IMU_FILEPATH, "time_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd.writeFile(GPS_FILEPATH, "time_stamp,gps_data\n");
  sd.writeFile(KALMAN_FILEPATH, "time_stamp,altitude,velocity,acceleration\n");
}

void writeToBothCards(const char* filename, const char* data) {
  sd_card.appendFile(filename, data);
  delay(50);
  sd_card2.appendFile(filename, data);
}

void writeSensorDataToSD(SensorData& sensor_data) {
  const char* filename = nullptr;
  int len = 0;
  
  switch (sensor_data.type) {
    case IMU:
      filename = IMU_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer),
        "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.imu.accel[0], sensor_data.data.imu.accel[1], sensor_data.data.imu.accel[2],
        sensor_data.data.imu.linear[0], sensor_data.data.imu.linear[1], sensor_data.data.imu.linear[2],
        sensor_data.data.imu.gravity[0], sensor_data.data.imu.gravity[1], sensor_data.data.imu.gravity[2],
        sensor_data.data.imu.quat[0], sensor_data.data.imu.quat[1], sensor_data.data.imu.quat[2], sensor_data.data.imu.quat[3],
        sensor_data.data.imu.gyro[0], sensor_data.data.imu.gyro[1], sensor_data.data.imu.gyro[2]);
      break;
      
    case ALTIMETER:
      filename = ALTIMETER_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.alt.altitude,
        sensor_data.data.alt.temp,
        sensor_data.data.alt.pressure);
      break;
      
    case GPS:
      filename = GPS_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%s\n",
        sensor_data.timestamp,
        sensor_data.data.gps.nmea);
      break;
    
    case KALMAN:
      filename = KALMAN_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.kalman.kalman_altitude,
        sensor_data.data.kalman.kalman_velocity,
        sensor_data.data.kalman.kalman_acceleration);
      break;
  }
  
  if (filename && len > 0 && (size_t)len < sizeof(dataBuffer)) {
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

  quat = matrixCreate(4, 1);
  dir = matrixCreate(3, 1);
  acc = matrixCreate(3, 1);
  setElement(dir, 3, 1, 1);

  filter = kalmanFilterCreate(states, observations);

  /* set filter matrices and vectors matrices
   * matrices are initially all 0s so no need to set them
   * initialize x_k_prev and P_k_prev not current time */
  
  /* F_k for constant acceleration assumption */
  setElement(filter->F_k, 1, 1, 1);
  setElement(filter->F_k, 1, 2, dt);
  setElement(filter->F_k, 1, 3, 0.5*dt*dt);
  setElement(filter->F_k, 2, 2, 1);
  setElement(filter->F_k, 2, 3, dt);
  setElement(filter->F_k, 3, 3, 1);
  
  setElement(filter->H_k, 1, 1, 1);
  setElement(filter->H_k, 2, 3, 1);
  
  /* process covar */
  setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
  setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));
  
  /* reading covar */
  setElement(filter->R_k, 1, 1, sigma_s * sigma_s);
  setElement(filter->R_k, 2, 2, sigma_a * sigma_a);
  
  /* uncertainty of initial velocity and acceleration */
  setElement(filter->P_k_prev, 2, 2, 100);
  setElement(filter->P_k_prev, 3, 3, 100);
  
  Serial.println("Setup complete!");
}

void loop() {
  // Get timestamp
  String ts = rtc.getTimestamp(true);

  // Read GPS data
  char nmea_tmp[100];
  gps2.GET_NMEA(nmea_tmp, sizeof(nmea_tmp));
  SensorData gps_data;
  gps_data.type = GPS;
  strncpy(gps_data.timestamp, ts.c_str(), sizeof(gps_data.timestamp) - 1);
  gps_data.timestamp[sizeof(gps_data.timestamp) - 1] = '\0';
  strncpy(gps_data.data.gps.nmea, nmea_tmp, sizeof(gps_data.data.gps.nmea) - 1);
  gps_data.data.gps.nmea[sizeof(gps_data.data.gps.nmea) - 1] = '\0';

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
  strncpy(altimeter_data.timestamp, ts.c_str(), sizeof(altimeter_data.timestamp) - 1);
  altimeter_data.timestamp[sizeof(altimeter_data.timestamp) - 1] = '\0';
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
  strncpy(imu_data.timestamp, ts.c_str(), sizeof(imu_data.timestamp) - 1);
  imu_data.timestamp[sizeof(imu_data.timestamp) - 1] = '\0';
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
  delay(100);

  // Kalman filter
  if (altitude < MIN_ALT) { // fully trust sensors
        setElement(filter->P_k_prev, 2, 2, 100);
        setElement(filter->P_k_prev, 3, 3, 100);
  }

  setElement(quat, 1, 1, imu_data.data.imu.quat[0]);
  setElement(quat, 2, 1, imu_data.data.imu.quat[1]);
  setElement(quat, 3, 1, imu_data.data.imu.quat[2]);
  setElement(quat, 4, 1, imu_data.data.imu.quat[3]);

  setElement(acc, 1, 1, imu_data.data.imu.linear[0]);
  setElement(acc, 2, 1, imu_data.data.imu.linear[1]);
  setElement(acc, 3, 1, imu_data.data.imu.linear[2]);

  vectorComponent(acc, quat, dir, &magnitude);

  setElement(filter->z_k, 1, 1, altitude);
  setElement(filter->z_k, 2, 1, magnitude);
  kalmanFilterPredict(filter);
  kalmanFilterUpdate(filter);

  getElement(filter->x_k, 1, 1, &kalman_altitude);
  getElement(filter->x_k, 2, 1, &kalman_velocity);
  getElement(filter->x_k, 3, 1, &kalman_acceleration);

  SensorData kalman_data;
  kalman_data.type = KALMAN;
  strncpy(kalman_data.timestamp, ts.c_str(), sizeof(kalman_data.timestamp) - 1);
  kalman_data.timestamp[sizeof(kalman_data.timestamp) - 1] = '\0';
  kalman_data.data.kalman.kalman_altitude = kalman_altitude;
  kalman_data.data.kalman.kalman_velocity = kalman_velocity;
  kalman_data.data.kalman.kalman_acceleration = kalman_acceleration;
  
  Serial.println("Writing kalman data...");
  writeSensorDataToSD(kalman_data);
  delay(100);

  // Stage update
  alt_diff = abs(kalman_altitude - max_altitude);
  switch (stage) {
    case 0: // On Ground
        if((kalman_acceleration > 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) {
            stage = 1;
        }
        break;
    case 1: // Thrust Phase
        if((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) {
            stage = 2;
        }
        break;
    case 2: // Falling Upwards
        if((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > APOGEE)) {
            stage = 3;
        }
        break;
    case 3: // Passing Apogee
        if((kalman_velocity < 0) && (kalman_altitude < APOGEE)) {
            stage = 4;
        }
        break;
    case 4: // Falling Downwards
        if(kalman_altitude < MIN_ALT) {
            stage = 0;
        }
        break;
  }

  Serial.println("GPS: " + String(nmea_tmp));
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
  snprintf(dataBuffer, sizeof(dataBuffer),
      "KALMAN: %.2f,%.2f,%.2f",
      kalman_data.data.kalman.kalman_altitude, kalman_data.data.kalman.kalman_velocity, kalman_data.data.kalman.kalman_acceleration);
  Serial.println("Stage: " + String(stage));
  Serial.println(dataBuffer);
  
  delay(1000);
}

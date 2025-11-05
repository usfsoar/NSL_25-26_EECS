/*
 * V1_Telemetry_Bay_2026.ino
 * 
 * Integrated Telemetry System for Teensy 4.1
 * Reads data from IMU (BNO085), Barometer (BMP581), and GPS sensors
 * Logs all data to SD card in CSV format
 * 
 * No RTOS - uses simple sequential polling in loop()
 */

#ifndef BUILTIN_SDCARD
#define BUILTIN_SDCARD 254
#endif

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <TimeLib.h>
#include "RTC_Test.h"
#include "SOAR_BNO085.h"
#include "SOAR_BMP581.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "_config.h"
#include "sensor_data_types.h"

// Create sensor objects
SOAR_RTC timer;
SOAR_BNO085 imu_sensor;
BMP581Sensor barometer;
SOAR_RTOS_GPS gps;
SOAR_SD_CARD sd(BUILTIN_SDCARD);

// Timing variables
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 100; // Read sensors every 100ms (10Hz)

// Function to read and log IMU data
void readAndLogIMU() {
  // Get current time
  int current_hours = timer.getTimeHours();
  int current_minutes = timer.getTimeMinutes();
  int current_seconds = timer.getTimeSeconds();
  int current_microseconds = timer.getTimeMicroseconds();

  // Update and get IMU data
  imu_sensor.update();
  SOAR_BNO085::AllSensorData_t imu_data = imu_sensor.getAllData();

  // Create CSV string with timestamp and IMU data
  String data_str_IMU = String(current_hours) + "," + String(current_minutes) + "," + 
                        String(current_seconds) + "," + String(current_microseconds) + "," +
                        String(imu_data.acceleration.x) + "," + String(imu_data.acceleration.y) + "," + String(imu_data.acceleration.z) + "," +
                        String(imu_data.linearAcceleration.x) + "," + String(imu_data.linearAcceleration.y) + "," + String(imu_data.linearAcceleration.z) + "," +
                        String(imu_data.gravity.x) + "," + String(imu_data.gravity.y) + "," + String(imu_data.gravity.z) + "," +
                        String(imu_data.orientation.x) + "," + String(imu_data.orientation.y) + "," + String(imu_data.orientation.z) + "," +
                        String(imu_data.gyroscope.x) + "," + String(imu_data.gyroscope.y) + "," + String(imu_data.gyroscope.z) + "\n";

  // Write to SD card
  sd.appendFile(IMU_FILEPATH, data_str_IMU.c_str());
}

// Function to read and log Barometer data
void readAndLogAltimeter() {
  // Get current time
  int current_hours = timer.getTimeHours();
  int current_minutes = timer.getTimeMinutes();
  int current_seconds = timer.getTimeSeconds();
  int current_microseconds = timer.getTimeMicroseconds();

  // Get barometer data
  float altitude = barometer.get_altitude();
  float temp = barometer.get_temperature();
  float pressure = barometer.get_pressure();

  // Create CSV string
  String data_str_ALTIMETER = String(current_hours) + "," + String(current_minutes) + "," + 
                              String(current_seconds) + "," + String(current_microseconds) + "," +
                              String(altitude) + "," + String(temp) + "," + String(pressure) + "\n";

  // Write to SD card
  sd.appendFile(ALTIMETER_FILEPATH, data_str_ALTIMETER.c_str());
}

// Function to read and log GPS data (stub for now - GPS implementation may vary)
void readAndLogGPS() {
  // Get current time
  int current_hours = timer.getTimeHours();
  int current_minutes = timer.getTimeMinutes();
  int current_seconds = timer.getTimeSeconds();
  int current_microseconds = timer.getTimeMicroseconds();

  // Get GPS NMEA sentence
  char nmea_sentence[100];
  char* gps_data = gps.GET_NMEA(nmea_sentence);

  // Create CSV string
  String data_str_GPS = String(current_hours) + "," + String(current_minutes) + "," + 
                        String(current_seconds) + "," + String(current_microseconds) + "," +
                        String(gps_data) + "\n";

  // Write to SD card
  sd.appendFile(LORA_FILEPATH, data_str_GPS.c_str());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== V1 Telemetry Bay 2026 Initializing ===");
  
  // Initialize I2C
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C
  
  // Initialize time
  setTime(0, 0, 0, 1, 1, 2026); // Set initial time: HH:MM:SS DD MM YYYY
  Serial.println("Time initialized");
  
  // Initialize SD card
  Serial.print("Initializing SD card...");
  sd.begin();
  Serial.println(" done!");
  
  // Create CSV header files
  sd.writeFile(IMU_FILEPATH, "Hours,Minutes,Seconds,Microseconds,AccelX,AccelY,AccelZ,LinearX,LinearY,LinearZ,GravX,GravY,GravZ,Roll,Pitch,Yaw,GyroX,GyroY,GyroZ\n");
  sd.writeFile(ALTIMETER_FILEPATH, "Hours,Minutes,Seconds,Microseconds,Altitude,Temperature,Pressure\n");
  sd.writeFile(LORA_FILEPATH, "Hours,Minutes,Seconds,Microseconds,GPS_Data\n");
  Serial.println("CSV files created with headers");
  
  // Initialize IMU
  Serial.print("Initializing BNO085 IMU...");
  if (imu_sensor.begin()) {
    Serial.println(" success!");
  } else {
    Serial.println(" FAILED!");
  }
  
  // Initialize Barometer
  Serial.print("Initializing BMP581 Barometer...");
  if (barometer.begin()) {
    Serial.println(" success!");
  } else {
    Serial.println(" FAILED!");
  }
  
  // Initialize GPS
  Serial.print("Initializing GPS...");
  gps.setup();
  Serial.println(" done!");
  
  Serial.println("=== Initialization Complete ===");
  Serial.println("Starting data logging...");
  Serial.println();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if it's time to read sensors
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;
    
    // Read and log all sensors
    readAndLogIMU();
    readAndLogAltimeter();
    readAndLogGPS();
    
    // Optional: Print status to serial every second
    static unsigned long lastPrint = 0;
    if (currentMillis - lastPrint >= 1000) {
      lastPrint = currentMillis;
      Serial.print("Logging... Time: ");
      Serial.print(timer.getTimeHours());
      Serial.print(":");
      Serial.print(timer.getTimeMinutes());
      Serial.print(":");
      Serial.print(timer.getTimeSeconds());
      Serial.print(" Altitude: ");
      Serial.print(barometer.get_altitude());
      Serial.println(" m");
    }
  }
}

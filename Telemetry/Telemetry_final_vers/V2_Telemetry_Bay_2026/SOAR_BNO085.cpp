/*
 * SOAR_BNO085.cpp
 *
 * Implementation file for the SOAR_BNO085 class. This file contains the logic
 * for initializing, calibrating, and reading data from the BNO085 sensor.
 */

#include "SOAR_BNO085.h"

// Constructor
SOAR_BNO085::SOAR_BNO085() {
  // Initialize last update time for velocity calculation
  lastVelocityUpdate = 0;
}

// Destructor
SOAR_BNO085::~SOAR_BNO085() {
  // Clean up the mutex
  if (dataMutex != NULL) {
    vSemaphoreDelete(dataMutex);
  }
}

// Initialize the BNO085 sensor
bool SOAR_BNO085::begin() {
  // Create the mutex for thread-safe data access
  dataMutex = xSemaphoreCreateMutex();
  if (dataMutex == NULL) {
    Serial.println("Error: Failed to create mutex.");
    return false;
  }
  
  // Initialize I2C communication
  Wire.begin();
  Wire.setClock(400000L); // Use 400kHz I2C speed

  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x. Check wiring.");
    return false;
  }
  Serial.println("BNO08x Found!");

  // Enable all necessary sensor reports
  Serial.println("Enabling sensor reports...");
  bool success = true;
  success &= bno08x.enableReport(SH2_ACCELEROMETER, 50000);
  success &= bno08x.enableReport(SH2_GRAVITY, 50000);
  success &= bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED, 50000);
  success &= bno08x.enableReport(SH2_LINEAR_ACCELERATION, 50000);
  success &= bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, 50000);
  success &= bno08x.enableReport(SH2_GAME_ROTATION_VECTOR, 50000);

  if (!success) {
    Serial.println("Failed to enable all sensor reports.");
  } else {
    Serial.println("All reports enabled successfully.");
  }
  return success;
}

// Poll the sensor and update internal data structures
void SOAR_BNO085::update() {
  if (bno08x.wasReset()) {
    Serial.println("Sensor reset detected, re-enabling reports.");
    begin(); // Re-initialize everything
  }

  if (bno08x.getSensorEvent(&sensorValue)) {
    // Lock the mutex for a thread-safe update
    if (xSemaphoreTake(dataMutex, (TickType_t)10) == pdTRUE) {
      processSensorEvent();
      xSemaphoreGive(dataMutex); // Release the mutex
    }
  }
}

// Private helper to process a sensor event
void SOAR_BNO085::processSensorEvent() {
  switch (sensorValue.sensorId) {
    case SH2_ACCELEROMETER:
      sensorData.acceleration.x = sensorValue.un.accelerometer.x;
      sensorData.acceleration.y = sensorValue.un.accelerometer.y;
      sensorData.acceleration.z = sensorValue.un.accelerometer.z;
      break;
    case SH2_GRAVITY:
      sensorData.gravity.x = sensorValue.un.gravity.x;
      sensorData.gravity.y = sensorValue.un.gravity.y;
      sensorData.gravity.z = sensorValue.un.gravity.z;
      break;
    case SH2_GYROSCOPE_CALIBRATED:
      sensorData.gyroscope.x = sensorValue.un.gyroscope.x;
      sensorData.gyroscope.y = sensorValue.un.gyroscope.y;
      sensorData.gyroscope.z = sensorValue.un.gyroscope.z;
      break;
    case SH2_LINEAR_ACCELERATION:
      sensorData.linearAcceleration.x = sensorValue.un.linearAcceleration.x;
      sensorData.linearAcceleration.y = sensorValue.un.linearAcceleration.y;
      sensorData.linearAcceleration.z = sensorValue.un.linearAcceleration.z;
      updateVelocity();
      break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
      sensorData.magneticField.x = sensorValue.un.magneticField.x;
      sensorData.magneticField.y = sensorValue.un.magneticField.y;
      sensorData.magneticField.z = sensorValue.un.magneticField.z;
      break;
    case SH2_GAME_ROTATION_VECTOR:
      updateOrientation();
      break;
  }
}

// Private helper to calculate velocity
void SOAR_BNO085::updateVelocity() {
  const float VELOCITY_DECAY = 0.98;
  const float ACCEL_THRESHOLD = 0.1;

  float accelX = (abs(sensorData.linearAcceleration.x) > ACCEL_THRESHOLD) ? sensorData.linearAcceleration.x : 0.0;
  float accelY = (abs(sensorData.linearAcceleration.y) > ACCEL_THRESHOLD) ? sensorData.linearAcceleration.y : 0.0;
  float accelZ = (abs(sensorData.linearAcceleration.z) > ACCEL_THRESHOLD) ? sensorData.linearAcceleration.z : 0.0;
  
  unsigned long currentTime = millis();
  if (lastVelocityUpdate != 0) {
    float dt = (currentTime - lastVelocityUpdate) / 1000.0;
    sensorData.velocity.x += accelX * dt;
    sensorData.velocity.y += accelY * dt;
    sensorData.velocity.z += accelZ * dt;

    sensorData.velocity.x *= VELOCITY_DECAY;
    sensorData.velocity.y *= VELOCITY_DECAY;
    sensorData.velocity.z *= VELOCITY_DECAY;
  }
  lastVelocityUpdate = currentTime;
}

// Private helper to calculate orientation
void SOAR_BNO085::updateOrientation() {
  float real = sensorValue.un.gameRotationVector.real;
  float i = sensorValue.un.gameRotationVector.i;
  float j = sensorValue.un.gameRotationVector.j;
  float k = sensorValue.un.gameRotationVector.k;

  sensorData.orientation.x = atan2(2 * (real * i + j * k), 1 - 2 * (i * i + j * j)) * RAD_TO_DEG;
  sensorData.orientation.y = asin(2 * (real * j - k * i)) * RAD_TO_DEG;
  sensorData.orientation.z = atan2(2 * (real * k + i * j), 1 - 2 * (j * j + k * k)) * RAD_TO_DEG;
  sensorData.orientation.accuracy = sensorValue.status;
}

// Getter function to safely retrieve all sensor data
SOAR_BNO085::AllSensorData_t SOAR_BNO085::getAllData() {
  AllSensorData_t dataCopy;
  // Lock the mutex for a thread-safe copy
  if (xSemaphoreTake(dataMutex, (TickType_t)10) == pdTRUE) {
    memcpy(&dataCopy, (void *)&sensorData, sizeof(AllSensorData_t));
    xSemaphoreGive(dataMutex);
  }
  return dataCopy;
}

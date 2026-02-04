#include "SOAR_BNO085.h"

// Constructor
SOAR_BNO085::SOAR_BNO085() : bno08x(BNO08X_RESET) {
  // Initialize I2C communication
  Wire.begin();
  Wire.setClock(400000L); // Use 400kHz I2C speed
}

void SOAR_BNO085::begin(void) {
  if (!bno08x.begin_I2C()) {
    Serial.println("Failed to find BNO08x. Check wiring.");
    while (1) { delay(10); }
  }
  Serial.println("BNO08x Found!");
  setReports();
}

// choose sensor outputs
void SOAR_BNO085::setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  }
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
    Serial.println("Could not enable gyroscope");
  }
  if (!bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED)) {
    Serial.println("Could not enable magnetic field calibrated");
  }
  if (!bno08x.enableReport(SH2_LINEAR_ACCELERATION)) {
    Serial.println("Could not enable linear acceleration");
  }
  if (!bno08x.enableReport(SH2_GRAVITY)) {
    Serial.println("Could not enable gravity vector");
  }
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
}

// Poll the sensor and update internal data structures
void SOAR_BNO085::update() {
  if (bno08x.wasReset()) {
    Serial.println("Sensor reset detected, re-enabling reports.");
    setReports();
  :}

  if (bno08x.getSensorEvent(&sensorValue)) {
    processSensorEvent();
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
        break;
    case SH2_MAGNETIC_FIELD_CALIBRATED:
        sensorData.magneticField.x = sensorValue.un.magneticField.x;
        sensorData.magneticField.y = sensorValue.un.magneticField.y;
        sensorData.magneticField.z = sensorValue.un.magneticField.z;
        break;
    case SH2_ROTATION_VECTOR:
        sensorData.orientation.w = sensorValue.un.rotationVector.real;
        sensorData.orientation.x = sensorValue.un.rotationVector.i;
        sensorData.orientation.y = sensorValue.un.rotationVector.j;
        sensorData.orientation.z = sensorValue.un.rotationVector.k;
        sensorData.orientation.accuracy = sensorValue.status;
        break;
  }
}

void SOAR_BNO085::showState() {
    printf("Orientation: (%.2f, %.2f, %.2f, %.2f)\n", sensorData.orientation.w, 
                                                      sensorData.orientation.x, 
                                                      sensorData.orientation.y, 
                                                      sensorData.orientation.z);

    printf("Accelerometer: (%.2f, %.2f, %.2f)\n", sensorData.acceleration.x,
                                                  sensorData.acceleration.y,
                                                  sensorData.acceleration.z);

    printf("Linear: (%.2f, %.2f, %.2f)\n", sensorData.linearAcceleration.x,
                                           sensorData.linearAcceleration.y,
                                           sensorData.linearAcceleration.z);                           

    printf("Gravity: (%.2f, %.2f, %.2f)\n", sensorData.gravity.x,
                                            sensorData.gravity.y,
                                            sensorData.gravity.z);

    printf("Magnetic Field: (%.2f, %.2f, %.2f)\n", sensorData.magneticField.x,
                                                   sensorData.magneticField.y,
                                                   sensorData.magneticField.z);

    printf("Gyro: (%.2f, %.2f, %.2f)\n", sensorData.gyroscope.x,
                                         sensorData.gyroscope.y,
                                         sensorData.gyroscope.z); 
}

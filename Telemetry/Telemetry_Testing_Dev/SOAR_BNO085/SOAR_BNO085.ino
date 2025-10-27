/*
 * SOAR_BNO085.ino
 *
 * Main application file. This sketch demonstrates how to use the SOAR_BNO085
 * class within a FreeRTOS environment on an ESP32.
 *
 * It creates two tasks:
 * 1. sensorUpdateTask: Continuously calls the update() method of the sensor object
 * to poll for new data.
 * 2. printDataTask: Periodically fetches the latest data from the sensor object
 * and prints it to the Serial Monitor.
 */

#include "SOAR_BNO085.h"

SOAR_BNO085 imu;

void setup() {
  delay(2000);  // Give Serial and sensor time to power up
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to open
  Serial.println("\nSOAR BNO085 Test (Teensy 4.1 - No FreeRTOS)");

  if (!imu.begin()) {
    Serial.println("Failed to initialize BNO085. Check wiring and I2C.");
    while (1);
  }
}

void loop() {
  imu.update(); // Poll sensor for new data

  SOAR_BNO085::AllSensorData_t data = imu.getAllData();

  Serial.println("\n--- IMU DATA ---");
  Serial.print("Orientation (deg) -> Roll: "); Serial.print(data.orientation.x, 1);
  Serial.print(" Pitch: "); Serial.print(data.orientation.y, 1);
  Serial.print(" Yaw: "); Serial.println(data.orientation.z, 1);

  Serial.print("Acceleration (m/s^2) -> X: "); Serial.print(data.acceleration.x, 2);
  Serial.print(" Y: "); Serial.print(data.acceleration.y, 2);
  Serial.print(" Z: "); Serial.println(data.acceleration.z, 2);

  Serial.print("Linear Accel (m/s^2) -> X: "); Serial.print(data.linearAcceleration.x, 2);
  Serial.print(" Y: "); Serial.print(data.linearAcceleration.y, 2);
  Serial.print(" Z: "); Serial.println(data.linearAcceleration.z, 2);

  Serial.print("Velocity (m/s) -> X: "); Serial.print(data.velocity.x, 2);
  Serial.print(" Y: "); Serial.print(data.velocity.y, 2);
  Serial.print(" Z: "); Serial.println(data.velocity.z, 2);

  Serial.print("Gyroscope (rad/s) -> X: "); Serial.print(data.gyroscope.x, 2);
  Serial.print(" Y: "); Serial.print(data.gyroscope.y, 2);
  Serial.print(" Z: "); Serial.println(data.gyroscope.z, 2);

  Serial.print("Gravity (m/s^2) -> X: "); Serial.print(data.gravity.x, 2);
  Serial.print(" Y: "); Serial.print(data.gravity.y, 2);
  Serial.print(" Z: "); Serial.println(data.gravity.z, 2);

  Serial.print("Magnetic Field (uT) -> X: "); Serial.print(data.magneticField.x, 2);
  Serial.print(" Y: "); Serial.print(data.magneticField.y, 2);
  Serial.print(" Z: "); Serial.println(data.magneticField.z, 2);

  delay(500); // Adjust print rate
}

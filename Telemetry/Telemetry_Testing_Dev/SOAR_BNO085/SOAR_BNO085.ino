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
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "SOAR_BNO085.h"

// Create an instance of our BNO085 sensor class
SOAR_BNO085 imu;

// Task to continuously update sensor data
void sensorUpdateTask(void *parameter) {
  // The main loop for this task
  while (true) {
    // This single function call handles polling and updating all sensor values
    imu.update();
    // A small delay is crucial to allow other tasks to run.
    delay(5);
  }
}

// Task to print the sensor data to the Serial Monitor
void printDataTask(void *parameter) {
  // The main loop for this task
  while (true) {
    // Retrieve all data using the getter functions
    SOAR_BNO085::AllSensorData_t data = imu.getAllData();

    // Print the retrieved data
    Serial.println("\n--- IMU DATA ---");
    Serial.print("Orientation (deg) -> X(Roll): "); Serial.print(data.orientation.x, 1);
    Serial.print(" Y(Pitch): "); Serial.print(data.orientation.y, 1);
    Serial.print(" Z(Yaw): "); Serial.print(data.orientation.z, 1);
    Serial.print(" | Accuracy/Status: "); Serial.println(data.orientation.accuracy, 0);

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

    // This delay controls the print rate
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  // Wait up to 3 seconds for Serial, then continue
  unsigned long startMillis = millis();
  while (!Serial && (millis() - startMillis < 3000)) {
    delay(10);
  }
  Serial.println("\nSOAR BNO085 Class Test with FreeRTOS");

  // Initialize the IMU. This handles I2C setup and enabling reports.
  if (!imu.begin()) {
    Serial.println("Failed to initialize BNO085. Halting.");
    while (1) delay(100);
  }

  // Create and pin the tasks to a specific core
  xTaskCreate(sensorUpdateTask, "Update IMU", 4096, NULL, 2, NULL);
  xTaskCreate(printDataTask, "Print Data", 4096, NULL, 1, NULL);
}

void loop() {
  // The main loop is empty. FreeRTOS is now in control.
  delay(1000);
}

#include "V2_1_SOAR_RTOS_IMU.h"

// Define the built-in LED pin for the ESP32S3
// #define BUILTIN_LED_PIN 2
// #define D0 D1

SOAR_IMU::SOAR_IMU() {
  this->bno = Adafruit_BNO055(55);
  // Constructor implementation
  // Initialize variables if needed
  prev_accel_x = 0;
  prev_accel_y = 0;
  prev_accel_z = 0;
  loop_iterations = 0;

}
// Implement other methods here

void SOAR_IMU::BNO_SETUP() {

  Serial.println("Orientation Sensor Test"); Serial.println("");

  /* Initialise the sensor */
  if (!this->bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  this->bno.setExtCrystalUse(true);
  Serial.print("Successful\n");
}

// bool SOAR_IMU::resetBNO() {
//   Serial.println("BNO not Initialized. Attempting hardware reset...");

//   // Turn on the built-in LED to indicate a reset attempt
//   pinMode(BUILTIN_LED_PIN, OUTPUT);
//   digitalWrite(BUILTIN_LED_PIN, HIGH);

//   // Perform hardware reset
//   digitalWrite(D0, LOW); // Pull the reset pin low
//   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
//   digitalWrite(D0, HIGH); // Release the reset pin
//   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

//   if (!this->bno.begin()) {
//     Serial.println("BNO reset failed. Halting execution.");
//     while (true) {
//       // Halt execution indefinitely
//       Serial.println("BNO not Initialized");
//       vTaskDelay(1000 / portTICK_PERIOD_MS); // Add a delay to avoid flooding the serial output
//     }
//     return false;
//   }

//   // Turn off the built-in LED if reset is successful
//   digitalWrite(BUILTIN_LED_PIN, LOW);
//   return true;
// }

// V2 Class Implementation
void SOAR_IMU::GET_ACCELERATION(float acceleration[3]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");
  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  imu::Vector<3> a = this->bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  acceleration[0] = a.x();
  acceleration[1] = a.y();
  acceleration[2] = a.z();
}


void SOAR_IMU::GET_LINEARACCEL(float lin_accel[3]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");

  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  imu::Vector<3> a = this->bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  lin_accel[0] = a.x();
  lin_accel[1] = a.y();
  lin_accel[2] = a.z();
}

void SOAR_IMU::GET_GRAVITY(float gravity[3]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");

  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  imu::Vector<3> a = this->bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  gravity[0] = a.x();
  gravity[1] = a.y();
  gravity[2] = a.z();
}

void SOAR_IMU::GET_GYROSCOPE(float gyro[3]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");

  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  imu::Vector<3> a = this->bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  gyro[0] = a.x();
  gyro[1] = a.y();
  gyro[2] = a.z();
}

void SOAR_IMU::GET_QUAT(float quat[4]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");

  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  imu::Quaternion q = this->bno.getQuat();
  quat[0] = q.x();
  quat[1] = q.y();
  quat[2] = q.z();
  quat[3] = q.w();
}

void SOAR_IMU::GET_VELOCITY(float velocity[3]) {
  // if (!this->bno.begin()) {
  //   Serial.println("BNO not Initialized. Attempting hardware reset...");

  //   // Turn on the built-in LED to indicate a reset attempt
  //   pinMode(BUILTIN_LED_PIN, OUTPUT);
  //   digitalWrite(BUILTIN_LED_PIN, HIGH);

  //   // Perform hardware reset
  //   digitalWrite(D0, LOW); // Pull the reset pin low
  //   vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 10 milliseconds
  //   digitalWrite(D0, HIGH); // Release the reset pin
  //   vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for the sensor to reset

  //   if (!this->bno.begin()) {
  //     Serial.println("BNO reset failed. BNO IS ASLEEP");

  //   }

  //   // Turn off the built-in LED if reset is successful
  //   digitalWrite(BUILTIN_LED_PIN, LOW);
  // }
  float lin_accel[3];
  GET_LINEARACCEL(lin_accel);
  // float *accel = GET_LINEARACCEL();
  if (loop_iterations < 1) {
    prev_accel_x = lin_accel[0];
    prev_accel_y = lin_accel[1];
    prev_accel_z = lin_accel[2];
    prev_time = (xTaskGetTickCount() / portTICK_PERIOD_MS);
  }

  if (loop_iterations >= 1) {
    //calculations using right hand sum rects and triangles
    uint32_t delta_time = (xTaskGetTickCount() / portTICK_PERIOD_MS) - prev_time;
    float delta_time_sec = delta_time / 1000.0;
    velocity[0] += (delta_time_sec * prev_accel_x) + (0.5 * delta_time_sec * lin_accel[0]);
    velocity[1] += (delta_time_sec * prev_accel_y) + (0.5 * delta_time_sec * lin_accel[1]);
    velocity[2] += (delta_time_sec * prev_accel_z) + (0.5 * delta_time_sec * lin_accel[2]);
  }

  prev_accel_x = lin_accel[0];
  prev_accel_y = lin_accel[1];
  prev_accel_z = lin_accel[2];
  prev_time = (xTaskGetTickCount() / portTICK_PERIOD_MS);
}

#include "SOAR_IMU.h"

SOAR_IMU::SOAR_IMU()
  : bno(55), barometer() {
  // Constructor implementation
  prev_accel_x = 0;
  prev_accel_y = 0;
  prev_accel_z = 0;
  loop_iterations = 0;
}

void SOAR_IMU::GET_EULER(float* euler) {
imu::Vector<3> eulerVec = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
euler[0] = eulerVec.x(); // Pitch
euler[1] = eulerVec.y(); // Roll
euler[2] = eulerVec.z(); // Yaw (may not be needed)
}

void SOAR_IMU::BNO_SETUP() {
  Serial.println("Orientation Sensor Test");
  if (!this->bno.begin()) {
    Serial.println("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
  }
  delay(1000);
  this->bno.setExtCrystalUse(true);
  Serial.print("Successful\n");
}

void SOAR_IMU::GET_ACCELERATION(float* acceleration) {
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  acceleration[0] = a.x();
  acceleration[1] = a.y();
  acceleration[2] = a.z();
}

void SOAR_IMU::GET_LINEARACCEL(float* linearAccel) {
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  linearAccel[0] = a.x();
  linearAccel[1] = a.y();
  linearAccel[2] = a.z();
}

void SOAR_IMU::GET_GRAVITY(float* gravity) {
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  gravity[0] = g.x();
  gravity[1] = g.y();
  gravity[2] = g.z();
}

void SOAR_IMU::GET_VELOCITY(float* velocity) {
  float current_altitude = barometer.get_last_altitude_reading();
  static float prev_altitude = barometer.get_last_altitude_reading();
  static uint32_t prev_time = 0;

  if (loop_iterations < 2) {
    prev_time = millis();
    prev_altitude = current_altitude;
  } else {
    uint32_t current_time = millis();
    uint32_t delta_time = current_time - prev_time;
    float delta_time_sec = delta_time / 1000.0;

    if (delta_time_sec > 0) {
      velocity[0] = (current_altitude - prev_altitude) / delta_time_sec;
    } else {
      velocity[0] = 0.0;
    }

    prev_altitude = current_altitude;
    prev_time = current_time;
  }

  loop_iterations++;
}

float SOAR_IMU::GET_RPM(float* rpm) {
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  float x = (float)a.x();
  float y = (float)a.y();
  float z = (float)a.z();

  float angularVelocity = sqrt(x * x + y * y + z * z);

  *rpm = angularVelocity * (60.0 / 314.0);

  return *rpm;
}
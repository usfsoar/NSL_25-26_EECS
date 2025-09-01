#include "SOAR_IMU.h"
#include "_config.h"
#include "DIGITAL_TWIN.h"

SOAR_IMU::SOAR_IMU() : bno(55), barometer() {
  // Constructor implementation
  prev_accel_x = 0;
  prev_accel_y = 0;
  prev_accel_z = 0;
  loop_iterations = 0;
}

void SOAR_IMU::BNO_SETUP() {
#if !SIMULATION
  Serial.println("Orientation Sensor Test");
  if (!this->bno.begin()) {
    Serial.println("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
  }
  delay(1000);
  this->bno.setExtCrystalUse(true);
  Serial.print("Successful\n");
#else
  Serial.println("Simulated Sensor Initilized");
#endif
}

void SOAR_IMU::GET_ACCELERATION(float* acceleration) {
#if !SIMULATION
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  acceleration[0] = a.x();
  acceleration[1] = a.y();
  acceleration[2] = a.z();
#else
  float x, y, z;
  getSimulatedAcceleration(x, y, z);
  acceleration[0] = x;
  acceleration[1] = y;
  acceleration[2] = z;
#endif
}

void SOAR_IMU::GET_LINEARACCEL(float* linearAccel) {
#if !SIMULATION
  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  linearAccel[0] = a.x();
  linearAccel[1] = a.y();
  linearAccel[2] = a.z();
#else
  float x, y, z;
  getSimulatedLinearAcceleration(x, y, z);
  linearAccel[0] = x;
  linearAccel[1] = y;
  linearAccel[2] = z;
#endif
}

void SOAR_IMU::GET_GRAVITY(float* gravity) {
#if !SIMULATION
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  gravity[0] = g.x();
  gravity[1] = g.y();
  gravity[2] = g.z();
#else
  float x, y, z;
  getSimulatedGravity(x, y, z);
  gravity[0] = x;
  gravity[1] = y;
  gravity[2] = z;
#endif
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
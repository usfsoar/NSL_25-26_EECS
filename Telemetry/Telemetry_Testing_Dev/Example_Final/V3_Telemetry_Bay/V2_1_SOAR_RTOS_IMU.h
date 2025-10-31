#ifndef SOAR_IMU_h
#define SOAR_IMU_h
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>


class SOAR_IMU {
public:
  SOAR_IMU(); // Constructor
  // Declare methods and variables here
  void BNO_SETUP();
  // bool resetBNO();
  void GET_ACCELERATION(float acceleration[3]);
  void GET_LINEARACCEL(float lin_accel[3]);
  void GET_GRAVITY(float gravity[3]);
  void GET_GYROSCOPE(float gyro[3]);
  void GET_QUAT(float quat[4]);
  void GET_VELOCITY(float velocity[3]);

private:
  // Declare private variables and methods here
  Adafruit_BNO055 bno;
  int fail_count;
  // previous time value
  uint32_t prev_time;
  //previous iteration acceleration values for calculating velocity
  float prev_accel_x; // Previous iteration's x-axis acceleration
  float prev_accel_y; // Previous iteration's y-axis acceleration
  float prev_accel_z; // Previous iteration's z-axis acceleration
  
  //count iterations of main loop:

public:
  //declare public variables here:
  uint32_t loop_iterations;
};

#endif

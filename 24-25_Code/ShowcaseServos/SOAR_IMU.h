#ifndef SOAR_IMU_h
#define SOAR_IMU_h
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

class SOAR_IMU {
public:
  SOAR_IMU();  // Constructor
  // Declare methods and variables here
  void BNO_SETUP();
  void GET_ACCELERATION(float* acceleration); 
  void GET_LINEARACCEL(float* linearAccel);   
  void GET_GRAVITY(float* gravity);           
  void GET_GYROSCOPE(float* gyro);            
  void GET_QUAT(float* quat);                         
  float GET_RPM(float* rpm);
private:
  // Declare private variables and methods here
  Adafruit_BNO055 bno;
  int fail_count;
  // previous time value
  uint32_t prev_time;
  //previous iteration acceleration values for calculating velocity
  float prev_accel_x; 
  float prev_accel_y; 
  float prev_accel_z; 

public:
  //declare public variables here:
  uint32_t loop_iterations;
};

#endif
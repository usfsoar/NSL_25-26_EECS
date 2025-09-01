#ifndef SOAR_IMU_h
#define SOAR_IMU_h
#include "_config.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "SOAR_BMP.h"

class SOAR_IMU {
  public:
    SOAR_IMU(); // Constructor
    // Declare methods and variables here
    void BNO_SETUP();
    void GET_ACCELERATION(float* acceleration); // Pass array as argument
    void GET_LINEARACCEL(float* linearAccel);   // Pass array as argument
    void GET_GRAVITY(float* gravity);           // Pass array as argument
    void GET_GYROSCOPE(float* gyro);           // Pass array as argument
    void GET_QUAT(float* quat);                // Pass array as argument
    void GET_VELOCITY(float* velocity);        // Pass array as argument


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
    SOAR_BAROMETER barometer;
};

#endif
#ifndef SOAR_IMU_h
#define SOAR_IMU_h
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

class SOAR_IMU {
public:
  SOAR_IMU();
  void BNO_SETUP();
  float* GET_GRAVITY();

private:
  Adafruit_BNO055 bno;
public:
};

#endif
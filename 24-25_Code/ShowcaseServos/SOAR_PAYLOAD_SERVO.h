#ifndef SOAR_PAYLOAD_SERVO_H
#define SOAR_PAYLOAD_SERVO_H

#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_PWMServoDriver.h>

class SOAR_PAYLOAD_SERVO {
public:
  SOAR_PAYLOAD_SERVO();  // Constructor
  void initialize(Adafruit_PWMServoDriver& pwm);
  void setServoAngle(Adafruit_PWMServoDriver& pwm, int channel, int angle);
  bool servoLogic(Adafruit_PWMServoDriver& pwm, float* gravity);

private:
  static const int SERVOMIN = 150;  // Minimum pulse length count (0 degrees)
  static const int SERVOMAX = 600;  // Maximum pulse length count (180 degrees)
};

#endif
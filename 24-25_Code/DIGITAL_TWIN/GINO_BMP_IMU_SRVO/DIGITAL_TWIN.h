#ifndef DIGITAL_TWIN_H
#define DIGITAL_TWIN_H

#include "_config.h"
#include <Arduino.h>

float getSimFloat(byte req_byte1, byte req_byte2, byte res_byte);
void getSimVector3(byte req_byte1, byte req_byte2, byte res_byte, float &x, float &y, float &z);

float getSimulatedAltitude();
float getSimulatedPressure();
float getSimulatedTemperature();
void getSimulatedAcceleration(float &x, float &y, float &z);
void getSimulatedLinearAcceleration(float &x, float &y, float &z);
void getSimulatedGravity(float &x, float &y, float &z);


void setSimIntInt(byte req_byte1, byte req_byte2, byte res_byte, int value1, int value2);
void setSimulatedServo(int servoNumber, int angle);


#endif

#ifndef SOAR_BME_H
#define SOAR_BME_H

#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Arduino.h>
#include <Wire.h>
#define SEALEVELPRESSURE_HPA (1020.32)  // Standard pressure at sea level

class SOAR_BME {
public:
  SOAR_BME(int sdaPin, int sclPin);
  void initialize();
  float getAltitude();
  float getTemperature();
  float getPressure();
  float getHumidity();
  bool isTemperatureInRange(float minTemp, float maxTemp);
  bool isPressureInRange(float minPressure, float maxPressure);

  bool isReady();
  bool ready;

private:
  Adafruit_BME680 bme;
  bool performSensorReading();
  float temperature;
  float pressure;
  float humidity;
  int sdaPin;
  int sclPin;
};

#endif
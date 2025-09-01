#include "SOAR_BME.h"

SOAR_BME::SOAR_BME(int sdaPin, int sclPin)
  : sdaPin(sdaPin), sclPin(sclPin) {}

void SOAR_BME::initialize() {
  Wire.begin(sdaPin, sclPin);
  if (!bme.begin(0x77)) {
    Serial.println("Could not find BME680 sensor!");
    while (1)
      ;
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  Serial.println("BME680 initialized successfully");
}

bool SOAR_BME::performSensorReading() {
  if (!bme.performReading()) {
    Serial.println("Failed to perform BME680 reading!");
    return false;
  }
  temperature = bme.temperature;
  pressure = bme.pressure / 100.0;  // Convert Pa to hPa
  humidity = bme.humidity;
  return true;
}

float SOAR_BME::getAltitude() {
  if (!performSensorReading()) return NAN;

  // Simplified barometric formula
  float pressure_hPa = bme.pressure / 100.0;
  float altitude = 44330.0 * (1.0 - pow(pressure_hPa / SEALEVELPRESSURE_HPA, 0.1903));

  return altitude;
}

float SOAR_BME::getTemperature() {
  if (!performSensorReading()) {
    return NAN;  // Return NaN if reading fails
  }
  return temperature;
}

float SOAR_BME::getPressure() {
  if (!performSensorReading()) {
    return NAN;  // Return NaN if reading fails
  }
  return pressure;
}

float SOAR_BME::getHumidity() {
  if (!performSensorReading()) {
    return NAN;  // Return NaN if reading fails
  }
  return humidity;
}

bool SOAR_BME::isTemperatureInRange(float minTemp, float maxTemp) {
  if (!performSensorReading()) {
    return false;  // Return false if reading fails
  }
  return (temperature >= minTemp && temperature <= maxTemp);
}

bool SOAR_BME::isPressureInRange(float minPressure, float maxPressure) {
  if (!performSensorReading()) {
    return false;  // Return false if reading fails
  }
  return (pressure >= minPressure && pressure <= maxPressure);
}
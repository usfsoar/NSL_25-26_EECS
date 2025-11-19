#include "SOAR_BMP581.h"

// Constructor: sets starting values for variables when the sensor object is created
BMP581Sensor::BMP581Sensor() : bmp_fail(0) {}
    //   fail_checkpoint(0)      // no failure timestamp yet


bool BMP581Sensor::begin() { // Communicate with sensor, used bool to check if successfull.  
    Serial.println("Initializing BMP581 sensor..."); //combine bool with print to have both debug messages and return value.
    if (!this->bmp.begin(BMP5XX_ALTERNATIVE_ADDRESS, &Wire2)) {
        bmp_fail++; //keep track of how many failures happened since boot.(Check Serial Monitor)
        fail_checkpoint = millis(); // records the time (ms) when the last failure happened. (Check Serial Monitor)
        Serial.println("ERROR: Could not find a valid BMP581 sensor, check wiring!");
        return false;
    }
    Serial.println("BMP581 initialized successfully.");

      // Set up oversampling and filter initialization
    this->bmp.setTemperatureOversampling(BMP5XX_OVERSAMPLING_8X);
    this->bmp.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
    this->bmp.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
    this->bmp.setOutputDataRate(BMP5XX_ODR_50_HZ);
    
    return true;
}

float BMP581Sensor::get_last_altitude_reading(){ //retries initialization if the barometer is unresponsive. Need for launch?
  if (!this->bmp.performReading() && (millis() - fail_checkpoint > 10000)) 
  {
    bmp_fail++;
    Serial.println("Failed to perform reading :(");
    if (bmp_fail > 10)
    {
      bmp_fail = 0;
      this->begin();  
      fail_checkpoint = millis();
      delay(100); // Small wait before retry (change to vTaskDelay if using FreeRTOS boards)
    }
    // Attempt to reconnect to the sensor
    return 0;
  }
  bmp_fail = 0;
  return this->bmp.readAltitude(SEALEVELPRESSURE_HPA);
}

float BMP581Sensor::get_altitude() {
    if (!this->bmp.performReading()) {
        bmp_fail++;
        fail_checkpoint = millis();
        Serial.println("ERROR: Failed to read altitude.");
        return 0; 
    }
    return bmp.readAltitude(SEALEVELPRESSURE_HPA);   // instead of _seaLevelPressure
}

float BMP581Sensor::get_pressure() {
    if (!this->bmp.performReading()) {
        bmp_fail++;
        fail_checkpoint = millis();
        Serial.println("ERROR: Failed to read pressure.");
        return 0;
    }
    return bmp.pressure; // direct property after performReading()
}

float BMP581Sensor::get_temperature() {
    if (!this->bmp.performReading()) {
        bmp_fail++;
        fail_checkpoint = millis();
        Serial.println("ERROR: Failed to read temperature.");
        return 0;
    }
    return bmp.temperature; // direct property after performReading()
}

bool BMP581Sensor::descent_check() { return false; } // Always returns false --Placeholder: Add logic later if needed

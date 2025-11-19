#ifndef BMP581_SENSOR_H
#define BMP581_SENSOR_H
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP5xx.h>   
#define SEALEVELPRESSURE_HPA (1013.25) // last year was 1009.5, ask Alan if we do different measurement. 
#include <Wire.h> // may not be necessary but have it for the hardware SDA SCL lines, and I2C              


class BMP581Sensor {
  public:
    BMP581Sensor();             // constructor
    bool begin();               // sets up sensor, returns true if success

    float get_altitude();       // meters (m)
    float get_pressure();       // Pascals (Pa)
    float get_temperature();    // Celsius (°C)
    float get_last_altitude_reading();  
    bool descent_check(); //Check if rocket still flying or coming down

    // Ask Alan: Do we need these?
    // float get_speed_reading();
    // float * get_dataframe();
    // bool descent_check();

 private:
    // Declare private variables and methods here
    Adafruit_BMP5xx bmp; // For physical sensor
    int bmp_fail; //keep track of failed readings
    uint32_t fail_checkpoint=-5000; // stores last time a reading failed
};

#endif

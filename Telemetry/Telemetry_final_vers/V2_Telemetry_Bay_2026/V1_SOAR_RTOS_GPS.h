#ifndef SOAR_RTOS_GPS_h
#define SOAR_RTOS_GPS_h
#include <Wire.h>
#include <Adafruit_GPS.h>

class SOAR_RTOS_GPS {
  public:
    SOAR_RTOS_GPS(); // Constructor
    void setup();
    bool GET_NMEA(char* out, size_t outSize);

    private:
      Adafruit_GPS GPS;  // GPS
      uint32_t GPS_FOCUS_MAX = 5000;
    // Declare private variables and methods here
};
  


#endif

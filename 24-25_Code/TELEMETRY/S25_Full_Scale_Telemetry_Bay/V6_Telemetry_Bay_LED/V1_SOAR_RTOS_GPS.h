#ifndef SOAR_RTOS_GPS_h
#define SOAR_RTOS_GPS_h
#include <Wire.h>
#include <Adafruit_GPS.h>
#include "SOAR_LED_Protocol.h"

class SOAR_RTOS_GPS {
  public:
    SOAR_RTOS_GPS(); // Constructor
    void setup();
    char* GET_NMEA(char* nmea, int splice_index);

    private:
      Adafruit_GPS GPS;  // GPS
      uint32_t GPS_FOCUS_MAX = 5000;
    // Declare private variables and methods here
};
  


#endif

#ifndef SOAR_GPS_h
#define SOAR_GPS_h
#include <Adafruit_GPS.h>

class SOAR_GPS {
  public:
    SOAR_GPS(int serial_bus, int RX_PIN, int TX_PIN); // Constructor
    void setup();
    void GET_NMEA(char *nmea, bool *ready, bool *failed);
    private:
      HardwareSerial* GPSSerial;   // GPS
      Adafruit_GPS* GPS;  // GPS
      int RX_PIN;
      int TX_PIN;
      uint32_t GPS_FOCUS_MAX = 5000;
    // Declare private variables and methods here
};



#endif



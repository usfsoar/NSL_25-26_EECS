#include "V1_SOAR_RTOS_GPS.h"
#define GPSECHO false

SOAR_RTOS_GPS::SOAR_RTOS_GPS() : GPS(&Wire1) {}

void SOAR_RTOS_GPS::setup() {
  if (GPS.begin(0x10)) {
    Serial.println("GPS Initialization Successful");
  } else {
    Serial.println("Failed to Initialize");
  }
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_10HZ); // 10 Hz update rate
    GPS.sendCommand(PGCMD_ANTENNA);
    delay(1000);
    GPS.println(PMTK_Q_RELEASE); // Request firmware version
}

void SOAR_RTOS_GPS::GET_NMEA(char* nmea) {

  while (true) {
    char c = GPS.read();

    if (GPSECHO && c) {
      Serial.print(c);
    }

    if (GPS.parse(GPS.lastNMEA())) {

      if (GPS.fix) {

        snprintf(
          nmea,
          25,
          "%.6f,%.6f",
          GPS.latitudeDegrees,
          GPS.longitudeDegrees
        );
      }
      else {
        strncpy(nmea, "NO_FIX", 25);
      }

      return;
    }
  }
}


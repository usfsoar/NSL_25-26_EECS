#include "V1_SOAR_RTOS_GPS.h"
#define GPSECHO false

SOAR_RTOS_GPS::SOAR_RTOS_GPS():GPS(&Wire1) {}

void SOAR_RTOS_GPS::setup() {
  if (GPS.begin(0x10)) {
    Serial.println("GPS Initialization Successful");
  } else {
    Serial.println("Failed to Initialize");
  }
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_GGAONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPS.println(PMTK_Q_RELEASE); // Request firmware version
}

void SOAR_RTOS_GPS::GET_NMEA(char* nmea) {
  char* rmcgga;
  // continue until GPS is received
  while (true) {
    char c = GPS.read();
    if (GPSECHO)
        if (c) Serial.print(c);
    if (GPS.newNMEAreceived()) {
        rmcgga = GPS.lastNMEA();
        strncpy(nmea, rmcgga, std::strlen(rmcgga) + 1);
        if (GPS.parse(GPS.lastNMEA()))
            return;
    }
  }
}

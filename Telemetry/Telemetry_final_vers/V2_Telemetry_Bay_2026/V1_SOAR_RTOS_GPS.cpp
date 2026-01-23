#include "V1_SOAR_RTOS_GPS.h"
#define GPSECHO false

SOAR_RTOS_GPS::SOAR_RTOS_GPS():GPS(&Wire) {
  // Constructor implementation
}

void SOAR_RTOS_GPS::setup() {
  GPS.begin(0x10); // Set the baud rate for the GPS module
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPS.println(PMTK_Q_RELEASE); // Request firmware version
}

bool SOAR_RTOS_GPS::GET_NMEA(char* out, size_t outSize) {
  if (!out || outSize == 0) return false;

  // Pump the GPS parser a bit
  GPS.read();

  char* gps_data = GPS.lastNMEA();
  if (!gps_data || gps_data[0] == '\0') {
    out[0] = '\0';
    return false;
  }

  // Your comma-filter copy (fields 1..7-ish)
  int commas = 0;
  size_t i = 0;

  for (char* p = gps_data; *p != '\0'; p++) {
    if (*p == ',') {
      commas++;
      if (commas > 7) break;
    }
    if (commas > 0 && commas <= 7) {
      if (i + 1 < outSize) out[i++] = *p;
      else break;
    }
  }
  out[i] = '\0';
  return true;
}
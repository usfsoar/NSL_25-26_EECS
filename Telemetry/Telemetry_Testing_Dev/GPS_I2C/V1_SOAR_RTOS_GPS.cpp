#include "V1_SOAR_RTOS_GPS.h"
#define GPSECHO false

SOAR_RTOS_GPS::SOAR_RTOS_GPS():GPS(&Wire1) {
  // Constructor implementation
}

void SOAR_RTOS_GPS::setup() {
  if (GPS.begin(0x10)) {
    Serial.println("GPS Initialization Successful");
  } else {
    Serial.println("Failed to Initialize");
  }
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPS.println(PMTK_Q_RELEASE); // Request firmware version
}

char* SOAR_RTOS_GPS::GET_NMEA(char* nmea) {
  while(1) {
  char read = GPS.read();
  char* gps_data = GPS.lastNMEA();
  char* p;
  int comas = 0;
  char* start = nmea;


   for (p = gps_data; *p != '\0'; p++) {
          if (*p == ',') {
            comas++;
            if (comas > 7) {
              break;
            }
          }
          // if (comas == 0) { // Extract the second field
          //   p++;
          //   char* start = p;
          //   while (*p != ',' && *p != '\0') {
          //     p++;
          //   }
          //   strncpy(nmea, start, p - start);
          //   nmea[p - start] = '\0'; // Null-terminate the string
          //   break;
          // }
          if (comas > 0 && comas <= 7) {
            *start++ = *p;
          //   char* start = p;
          // while (*p != ',' && *p != '\0') {
          //     p++;
          //   }
          //   strncpy(*nmea, p, p - start);
          //   nmea[p - start] = '\0';
          }
        }
        *start = '\0';
        return nmea;
        }
  // *nmea = '\0'; // Null-terminate the string
  // strcpy(nmea, gps_data);

  // if (GPSECHO)
  //   if (read) Serial.println(read);
  // if (GPS.newNMEAreceived()) {
  //   // *nmea = '\0'; // Null-terminate the string
  //   // strcpy(nmea, gps_data);
  //   // Serial.print("NMEA data: ");
  //   // Serial.println(nmea);
  //   Serial.println(GPS.lastNMEA());
  //   if (!GPS.newNMEAreceived()) {
  //     Serial.println("No new NMEA sentence received.");
  // } else if (!GPS.parse(GPS.lastNMEA())) {
  //     Serial.println("Failed to parse NMEA sentence.");
  // }
  // }
  // vTaskDelay(1000);
  // }
  // }
}

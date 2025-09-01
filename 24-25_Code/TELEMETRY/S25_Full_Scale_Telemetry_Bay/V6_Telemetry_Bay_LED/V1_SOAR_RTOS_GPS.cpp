#include "V1_SOAR_RTOS_GPS.h"
#define GPSECHO false

LEDProtocol led5;

SOAR_RTOS_GPS::SOAR_RTOS_GPS():GPS(&Wire) {
  // Constructor implementation
}

void SOAR_RTOS_GPS::setup() {
  GPS.begin(0x10); // Set the baud rate for the GPS module
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  vTaskDelay(1000);
  GPS.println(PMTK_Q_RELEASE); // Request firmware version
  led5.GPS_initialize();
}

char* SOAR_RTOS_GPS::GET_NMEA(char* nmea, int splice_index) {
  while(1) {
  char read = GPS.read();
  char* gps_data = GPS.lastNMEA();
  char* p;
  int comas = 0;
  char* start = nmea;


   for (p = gps_data; *p != '\0'; p++) {
          if (*p == ',') {
            comas++;
            // if (comas > 6) {
            //   break;
            // }
          }
          if (comas == splice_index) { // Extract the indexed field
            p++;
            char* start = p;
            while (*p != ',' && *p != '\0') {
              p++;
            }
            strncpy(nmea, start, p - start);
            nmea[p - start] = '\0'; // Null-terminate the string
            break;
          }
          // if (comas > 1 && comas <= 6) {
          //   *start++ = *p;
          //   char* start = p;
          // while (*p != ',' && *p != '\0') {
          //     p++;
          //   }
          //   strncpy(*nmea, p, p - start);
          //   nmea[p - start] = '\0';

          // }
        }
        // *start = '\0';
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

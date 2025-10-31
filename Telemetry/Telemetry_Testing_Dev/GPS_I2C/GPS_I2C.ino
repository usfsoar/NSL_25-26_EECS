// #if CONFIG_FREERTOS_UNICORE
//   static const BaseType_t app_cpu = 0;
// #else
//   static const BaseType_t app_cpu = 1;
// #endif

#include "V1_SOAR_RTOS_GPS.h"
// #define GPSECHO false

SOAR_RTOS_GPS gps2;
Adafruit_GPS GPS(&Wire);

// void GPSTask (void *Parameters) {
//   while(1){

//     //  read data from the GPS in the 'main loop'
//   // char c = GPS.read();
//   // // if you want to debug, this is a good time to do it!
//   // if (GPSECHO)
//   //   if (c) Serial.print(c);
//   // // if a sentence is received, we can check the checksum, parse it...
//   // if (GPS.newNMEAreceived()) {
//   //   // a tricky thing here is if we print the NMEA sentence, or data
//   //   // we end up not listening and catching other sentences!
//   //   // so be very wary if using OUTPUT_ALLDATA and trying to print out data
//   //   Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
//   // }
//       char nmea[100];
//       gps2.GET_NMEA(nmea);
//       Serial.print("NMEA data: ");
//       Serial.println(nmea);
//     }
//   }

void setup() {
  Serial.begin(115200);
  gps2.setup();
  // xTaskCreatePinnedToCore(
  //   GPSTask,
  //   "GPS Task",
  //   4096,
  //   NULL,
  //   3,
  //   NULL,
  //   app_cpu
  // );
}

void loop() {
      char nmea[100];
      gps2.GET_NMEA(nmea);
      Serial.print("NMEA data: ");
      Serial.println(nmea);
}

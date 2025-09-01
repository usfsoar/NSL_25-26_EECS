#include "SOAR_gps.h"

SOAR_GPS::SOAR_GPS(int serial_bus, int TX, int RX) : 
   RX_PIN(RX), TX_PIN(TX){
    GPSSerial = new HardwareSerial(0);
    GPS = new Adafruit_GPS(GPSSerial);
   }

void SOAR_GPS::setup(){
  GPSSerial->begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  GPS->begin(9600);

  GPS->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS->sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS->sendCommand(PGCMD_ANTENNA);
  GPSSerial->println(PMTK_Q_RELEASE);
}


void SOAR_GPS::GET_NMEA(char* nmea, bool *ready, bool *failed){
  char read = GPS->read();
  if(!GPS->newNMEAreceived()){
    *ready = false;
    return;
  }
  if (!GPS->parse(GPS->lastNMEA())){
    *failed = true;
    return;
  }

  char* gps_data = GPS->lastNMEA();

  int comas = 0;
  for(char *p = gps_data; *p != '\0'; p++){
    if(*p == ','){
      comas++;
      if(comas == 6){
        continue; // Skip copying the comma itself
      }
    }
    if(comas > 6 && comas < 10){
      *nmea++ = *p; // Copy data between the 7th and 10th commas
    } else if(comas >= 10){
      break; // Stop copying after the 9th comma
    }
  }
  *nmea = '\0'; // Null-terminate the NMEA string
}


  //    if (millis() - gps_repeat_focus_checkpoint > GPS_FOCUS_MAX) {
  //     // gps_repeat_focus = false;
  //     gps_repeat_focus_checkpoint = millis();
  //     lora.stringPacketWTime("GSFI");
  //     Serial.println("GPS Repeat Focus Timed Out");



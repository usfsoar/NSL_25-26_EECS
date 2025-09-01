#include "_config.h"
#include <Wire.h>
// #include <SPI.h>
#include "soar_imu.h"
#include "SOAR_SD_CARD.h"
#include "ota_update.h"
#define RX A3  // GPS Pins
#define TX A2  // GPS Pins
#include "buzzer_notify.h"
#include "SOAR_Lora.h"
#include <Adafruit_GPS.h>
#include <HardwareSerial.h>
#include "utils.h"
#include "SOAR_gps.h"
// #include "SOAR_RRC3.h"
// #include "soar_speaker.h"

uint32_t GPS_FOCUS_MAX = 10000;
uint32_t ALT_FOCUS_MAX = 10000;



#define buzzerPin A0

#define SDA_PIN 5 // Define SDA pin
#define SCL_PIN 6 // Define SCL pin

// Create SOAR_IMU instance
SOAR_IMU imu_sensor;

// Create sd class instance
OTA_Update otaUpdater("soar-recovery", "TP-Link_BCBD", "10673881");
SOAR_SD_CARD sd_card(A1);
SOAR_Lora lora("7", "5", "420000000", 500);  // LoRa
SOAR_GPS gps(1, RX, TX);
// SOAR_RRC3 rrc3(2, A2, A3);
// SOAR_Speaker speaker(9, 8, 7, 30);// amp pins, volume
// GPS Hardware Serial Initiation

// HardwareSerial GPSSerial(1);   // GPS
// Adafruit_GPS GPS(&GPSSerial);  // GPS


BuzzerNotify buzzerNotify(buzzerPin);


// RRC3
// HardwareSerial altSerial(2);

// #define SLAVE_ADDRESS 0x08

byte data_to_send = 0;
byte data_to_echo = 0;
String output = "IDLE";
#define GPSECHO false

class AutomatedTelemetry
{
  private:
    int _repeat_status = 0; // 0: no repeat, 1: repeat all, 2: repeat altitude, 3: repeat distance, 4: repeat status
    uint32_t _last_repeat = 0;
    uint32_t _repeat_interval = 1000;
  public:
    AutomatedTelemetry(uint32_t repeat_interval, int init_status=0){
      _repeat_interval = repeat_interval;
      _repeat_status = init_status;
    }
    void SetRepeatStatus(int status){
      _repeat_status = status;
    }
    void Handle(float *linear, float *gravity, float *gyro, float altitude, const char* gps_nmea){
      if(_repeat_status == 0) return;
      //Check for repeat interval
      if(millis()-_last_repeat < _repeat_interval) return;
      
      switch(_repeat_status){
        case 1:
          //GPS, RRC3 and IMU data
          /*
          |`IS`|All info single | `IS{GPS NMEA string};{altitude-float}{LinearX-float}{LinearY-float}{LinearZ-float}{GravityX-float}{GravityY-float}{GravityZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{T-time}`| All info response|
          */
          lora.beginPacket();
          lora.sendChar("IS");
          lora.sendChar(gps_nmea);
          lora.sendChar(";");
          lora.sendFloat(altitude);
          lora.sendFloat(linear[0]);
          lora.sendFloat(linear[1]);
          lora.sendFloat(linear[2]);
          lora.sendFloat(gravity[0]);
          lora.sendFloat(gravity[1]);
          lora.sendFloat(gravity[2]);
          lora.sendFloat(gyro[0]);
          lora.sendFloat(gyro[1]);
          lora.sendFloat(gyro[2]);
          lora.endPacketWTime(6);
          break;
        case 2: // Only GPS data send
          lora.beginPacket();
          lora.sendChar("GS");
          lora.sendChar(gps_nmea);
          lora.endPacketWTime(6);
          break;
        case 3: //Only send altitude Data
          lora.beginPacket();
          lora.sendChar("AS");
          lora.sendFloat(altitude);
          lora.endPacketWTime(6);
          break;
        case 4: //Only send IMU data
          lora.beginPacket();
          lora.sendFloat(linear[0]);
          lora.sendFloat(linear[1]);
          lora.sendFloat(linear[2]);
          lora.sendFloat(gravity[0]);
          lora.sendFloat(gravity[1]);
          lora.sendFloat(gravity[2]);
          lora.sendFloat(gyro[0]);
          lora.sendFloat(gyro[1]);
          lora.sendFloat(gyro[2]);
          lora.endPacketWTime(6);
          break;
        default:
          break;
      }
      _last_repeat = millis();
    }
};
AutomatedTelemetry autoTelemetry(500, 1);

void setup() {

  Serial.begin(115200);
  // lora.begin(115200); // Initialize Software Serial
  // LoRa
  buzzerNotify.Setup();
  lora.begin();
  Wire.begin();

  // RRC3
  // CAUTION --------------------------------------------------------------------------------------
  // THE PINOUTS HERE NEED TO BE REVISED BEFORE UPLOADING TO ESP BECAUSE THEY MIGHT OVERLAP WITH OTHER DEFINED PINS
  // CAUTION
  // altSerial.begin(9600, SERIAL_8N1, 1, 0);
  // rrc3.setup();
  // HEADS UP
  // ----------------------------------------------------------------------------------------------
  
  // GPS
  // Pins 4 and 3, but I have to add 1??
  // https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/#other-hardware-serial
  // GPSSerial.begin(9600, SERIAL_8N1, RX, TX);
  // GPS.begin(9600);

  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);  // 1 Hz update rate

  // GPS.sendCommand(PGCMD_ANTENNA);
  // // delay(1000);
  // // Ask for firmware version
  // GPSSerial.println(PMTK_Q_RELEASE);
  gps.setup();
  Serial.println("Setup completed");
  buzzerNotify.Trigger();
  otaUpdater.Setup();

  lora.stringPacketWTime("WU",6);


  sd_card.begin();
  sd_card.deleteFile( "/imu_data.csv");
  sd_card.writeFile("/imu_data.csv", "time,accel_x, accel_y, accel_z, linear_x, linear_y, linear_z, gravity_x, gravity_y, gravity_z, quat_w, quat_x, quat_y, quat_z, gyro_x, gyro_y, gyro_z, altitude, gps_nmea\n");
  // sd_card.writeFile("/imu_data.csv", "time, accel_x, accel_y, accel_z\n");
  imu_sensor.BNO_SETUP();
}

// // bool gps_focus = false;
// bool gps_repeat_focus = false;
// // uint32_t gps_focus_checkpoint = 0;
// uint32_t gps_repeat_focus_checkpoint = 0;
// bool alt_repeat_focus = false;
// uint32_t alt_repeat_focus_checkpoint = 0;

// // uint32_t timer_write_test = 5000;
// String gpsString = "";




void loop() {
  // RRC3
  // if(altimeter_focus){
  //   // altimeter.listen();
  //   char d = altimeter.read();
  //   if (old_alti_info != d) {
  //     char* altimeter_data = d;
  //     String altimeter_data_string = String(altimeter_data);
  //     String vital_altimeter_info = "Alti: " + altimeter_data_string;

  //     // lora.listen();
  //     send_command(vital_altimeter_info);
  //   }
  // }
  float *accel = imu_sensor.GET_ACCELERATION();
  float *linear = imu_sensor.GET_LINEARACCEL();
  float *gravity = imu_sensor.GET_GRAVITY();
  float *quat = imu_sensor.GET_QUAT();
  float *gyro = imu_sensor.GET_GYROSCOPE();

  char gps_nmea[1000];
  bool gps_ready;
  bool gps_failed;
  gps.GET_NMEA(gps_nmea, &gps_ready, &gps_failed);

  float altitude;
  // bool rrc3_ready;
  // bool rrc3_failed;

  // rrc3.GET_ALTITUDE(altitude, rrc3_ready, rrc3_failed);
  // if (gps_repeat_focus) {
  //   char c = GPS.read();
  //   if ((c) && (GPSECHO)) {
  //     Serial.write(c);
  //   }

  //   if (GPS.newNMEAreceived()) {
  //     if (!GPS.parse(GPS.lastNMEA())) {
  //       Serial.println("Failed to parse");
  //       String gpsString = "GPS: Not Ready";
  //       Serial.println("GPS: Not Ready");
  //     } else {
  //       char* gps_data = GPS.lastNMEA();  // Get the NMEA sentence
  //       char vital_gps_info[27];          // Array to hold the vital part, +1 for null terminator

  //       if (strlen(gps_data) >= 44) {     // Check if the string is long enough
  //           strncpy(vital_gps_info, gps_data + 18, 26); // Copy the substring
  //           vital_gps_info[26] = '\0';    // Null-terminate the string
  //       } else {
  //           vital_gps_info[0] = '\0';     // If the string is too short, make it an empty string
  //       }

  //       // lora.listen();
  //       lora.beginPacket();
  //       lora.sendChar(vital_gps_info);
  //       lora.endPacketWTime();
  //       Serial.println(gps_data);

  //       Serial.println();
  //       Serial.print("Releasing GPS Repeat Focus. Took time: ");
  //       Serial.println(millis() - gps_focus_checkpoint);
  //       gps_repeat_focus = true;
  //       gps_repeat_focus_checkpoint = millis();
  //     }
  //   }
  //   if (millis() - gps_repeat_focus_checkpoint > GPS_FOCUS_MAX) {
  //     // gps_repeat_focus = false;
  //     gps_repeat_focus_checkpoint = millis();
  //     lora.stringPacketWTime("GSFI");
  //     Serial.println("GPS Repeat Focus Timed Out");
  //   }
  // }
  // if (alt_repeat_focus) {
  //   if (!altSerial.available()){
  //     Serial.println("Altimeter is not ready");
  //   }
  //   if(altSerial.available()){
  //     Serial.println("Received Packet");
  //     unsigned long startTime = millis();
  //     Serial.print("Received: ");
  //     String result;
  //     unsigned long milliseconds = 5000;
  //     while (millis() - StartTime < milliseconds){
  //       if(altSerial.available()){
  //         char c = altSerial.read();
  //         Serial.write(c);
  //         result += c;
  //       }
  //     }
  //     if(millis() - alt_repeat_focus_checkpoint > ALT_FOCUS_MAX) {
  //       alt_repeat_focus_checkpoint = millis();
  //       lora.stringPacketWTime("ASF");
  //       Serial.println("Altimeter Repeat Focus failed and timed out");
  //     }
  //   }
  // }
  update_current_sd_file(accel, linear, gravity, quat, gyro, altitude, gps_nmea);
  
  
  /*
  |Command|Definition|Response|Definition|
  |---|---|---|---|
  |`PI`|Ping|`PO`|Pong|
  |`GS`|GPS Single|`GS{T-time}`| GPS Single Received|
  |||`GS{NMEA String}{T-Time}`|GPS NEMA reponse|
  |||`GSF{T-time}`|GPS Failed to get a fix|
  |`GR`|GPS Repeat|`GR{T-Time}`|GPS Repeat Received|
  |`BS`|IMU Single|`BS{LinearX-float}{LinearY-float}{LinearZ-float}{GravityX-float}{GravityY-float}{GravityZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{T-time}`|IMU Data (14 bytes total)|
  |`BR`|IMU Repeat|`BR{T-time}`|IMU Repeat Received|
  |`AS`|Altitdue Single| `AS{T-time}`| Altitude Response Received|
  |||`AS{altitue-float}{T-time}`|Altitude response|
  |||`ASF{T-time}`|Altitude failed to read|
  |`AR`|Altitude Repeat| `AR{T-time}` | Altitude repeat received |
  |`IS`|All info single | `IS{GPS NMEA string};{altitude-float}{LinearX-float}{LinearY-float}{LinearZ-float}{GravityX-float}{GravityY-float}{GravityZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{T-time}`| All info response|
  |`IR`| All info repeat | `IR`| All info repeat received|
  |`PA{selection-int}` | Play audio with index of selection |`PA{selection int}` | Play audio received with index|
  |`WR{ssid string};{password string}`| Retry wifi connection | `WR{ssid-string};{password-string}`|Wifi retry received|
    */
  
  int address, length, rssi, snr;
  byte *data;
  bool lora_available = lora.read(&address, &length, &data, &rssi, &snr);
  if (lora_available && length > 0 && lora.checkChecksum(data, length)) // A command is typically 2 bytes
  {

    bool valid_command = true;
    if (length > 2) {
      char command[3] = {data[0], data[1], '\0'};
      if(!strcmp(command, "PI")){
        lora.beginPacket();
        lora.stringPacketWTime("PO",6);
      }
      else if(!strcmp(command, "GS")){}
      else if(!strcmp(command, "GR")){}
      else if(!strcmp(command, "BS")){}
      else if(!strcmp(command, "BR")){}
      else if(!strcmp(command, "AS")){}
      else if(!strcmp(command, "AR")){}
      else if(!strcmp(command, "IS")){}
      else if(!strcmp(command, "IR")){}
      else if(!strcmp(command, "PA")){}
      else if(!strcmp(command, "WR")){}
      else{
        valid_command = false;
      }
    } else{
      valid_command = false;
    }

    if(!valid_command){
      lora.beginPacket();
      lora.sendChar("NH");
      for (int i = 0; i < length; i++) {
        lora.sendByte(data[i]);
      }
      lora.endPacketWTime(6);
    }
  }

  otaUpdater.Handle();
  lora.handleQueue();
  autoTelemetry.Handle(linear, gravity, gyro, altitude, gps_nmea);
  buzzerNotify.Check();

  delete[] accel;
  delete[] linear;
  delete[] gravity;
  delete[] quat;
  delete[] gyro;

}

void update_current_sd_file(float *a, float *l, float *g, float *q, float *gyro, float altitude, const char* gps_nmea) {
  char buffer[512];  // Adjust the size as needed
  snprintf(buffer, sizeof(buffer),
           "%lu , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %.2f , %s%.2f\n",
           millis(),
           a[0], a[1], a[2],
           l[0], l[1], l[2],
           g[0], g[1], g[2],
           q[0], q[1], q[2], q[3],
           gyro[0], gyro[1], gyro[2],
           gps_nmea, altitude);

#if DEBUG_CSV
  Serial.println(buffer);
#endif

  sd_card.appendFile("/imu_data.csv", buffer);
}

// void update_current_sd_file(float *a, float *l, float *g, float *q, float *gyro, float altitude, const char* gps_nmea){
//   // String out =  String(millis()) + " , " + String(a[0]) + " , " + String(a[1]) + " , " + String(a[2]) +"\n";
//   //Include all the data in the file
//   String out =  String(millis()) + " , " +
//     String(a[0]) + " , " + String(a[1]) + " , " + String(a[2]) + " , " +
//     String(l[0]) + " , " + String(l[1]) + " , " + String(l[2]) + " , " + 
//     String(g[0]) + " , " + String(g[1]) + " , " + String(g[2]) + " , " + 
//     String(q[0]) + " , " + String(q[1]) + " , " + String(q[2]) + " , " + String(q[3]) + " , " +
//     String(gyro[0]) + " , " + String(gyro[1]) + " , " + String(gyro[2]) +"\n"+
//     gps_nmea + String(altitude);
//     ;
// #if DEBUG_CSV
//   Serial.println(out);
// #endif
//   const char * c = out.c_str();
//   sd_card.appendFile("/imu_data.csv", c);

//   return;
// }
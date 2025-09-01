// Delete SD card, GPS, RRC 3, buzzer, imu and remove the file

#include "_config.h"
#include <Wire.h>
// #include <SPI.h>
#include "ota_update.h"
#include "ota_update.cpp"
#define RX A3  // GPS Pins
#define TX A2  // GPS Pins
#include "SOAR_Lora.cpp"
#include "SOAR_Lora.h"
#include <HardwareSerial.h>
// #include "utils.h"
// #include "soar_speaker.h"


#define buzzerPin A0

#define SDA_PIN 5 // Define SDA pin
#define SCL_PIN 6 // Define SCL pin

SOAR_Lora lora("6", "5", "915000000"); // LoRa
OTA_Update otaUpdater("AV-transmit", "TP-Link_BCBD", "10673881"); // otaUpdater object


byte data_to_send = 0;
byte data_to_echo = 0;
String output = "IDLE";
#define GPSECHO false



void setup() {

  Serial.begin(115200);
  // lora.begin(115200); // Initialize Software Serial
  // LoRa
  lora.begin();

  Serial.println("Setup completed");
  otaUpdater.Setup();

  lora.stringPacketWTime("WU",6);


  
  // imu_sensor.BNO_SETUP();
}




void loop() {  
  
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

  // delete[] accel;
  // delete[] linear;
  // delete[] gravity;
  // delete[] quat;
  // delete[] gyro;

}


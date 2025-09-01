#include "_config.h"
#include "ota_update.h"

#include "SOAR_Lora.h"
#include "utils.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

OTA_Update otaUpdater("soar-l1-receiver", "TP-Link_BCBD", "10673881");
SOAR_Lora lora("6", "5", "905000000", 500);  // LoRa

void decodeChar(char *output, const byte *packetBuffer, int bufferLength) {
    int checksum = 0;
    for (int i = 0; i < bufferLength; i++) {
        output[i] = (char)packetBuffer[i];
        checksum += packetBuffer[i];
    }
    output[bufferLength] = '\0'; // Null-terminate the string
    // The checksum can be used for validation if needed
    Serial.print("Decoded checksum: ");
    Serial.println(checksum);
}


class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  // To receive data from phone/laptop/PWA
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    /*std::string value = pCharacteristic->getValue();
    if (value.length() > 0)
    {
      String value_str = "";
      for (int i = 0; i < value.length(); i++)
        value_str += value[i];
      Serial.print("Received Value: ");
      Serial.println(value_str);
      if (value_str == "A")
      {
        pCharacteristic->setValue("I have got data");
        pCharacteristic->notify();
        Serial.println("I have got data");
      }
      else if (value_str == "B")
      {
        pCharacteristic->setValue("I got other data");
        pCharacteristic->notify();
        Serial.println("I got other data");
      }
      
    }*/
  }
};



void setup() {

  Serial.begin(115200);


  Serial.println("Setup completed");
  otaUpdater.Setup();
  lora.begin();
  lora.stringPacketWTime("WU",7);
  // My idea
  // lora.stringPacketWTime("GS", 7);


  // Bluetooth setup---------------
  BLEDevice::init("SOAR_L1 Tracker");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
  pCharacteristic = pService->createCharacteristic(
      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
      BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

unsigned long prevTime = 0;
unsigned long interval = 5000;
void loop() {

  // Disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // Give the Bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("Advertising started");
    oldDeviceConnected = deviceConnected;
  }

  // Connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }

  if(millis()-prevTime > interval){
    if (deviceConnected) {
          pCharacteristic->setValue("ping");
          pCharacteristic->notify();
          Serial.println("Sending");
        }
        prevTime = millis();
  }

  // LoRa Beginif
  int address, length, rssi, snr; 
  byte *data;
  bool lora_available = lora.read(&address, &length, &data, &rssi, &snr);
  if (lora_available && length > 0 && lora.checkChecksum(data, length)) // A command is typically 2 bytes
  {
    char decodedString[length];


    bool valid_command = true;
    if (length > 2) {
      char command[3] = {data[0], data[1], '\0'};
      if(!strcmp(command, "PI")){
        lora.beginPacket();
        lora.stringPacketWTime("PO",6);
      }
      else if(!strcmp(command, "GS")){
        decodeChar(decodedString, data, length);
        // Bluetooth sending
        if (deviceConnected) {
          pCharacteristic->setValue(decodedString);
          pCharacteristic->notify();
        }
        Serial.println(decodedString);
      }
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
  // LoRa Endif

  otaUpdater.Handle();
  lora.handleQueue();


}

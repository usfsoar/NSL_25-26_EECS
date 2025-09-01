// #include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "SOAR_Lora.h"

// #include <Adafruit_BMP3XX.h>
// #include <RYLR896.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>

// Pin definitions
// #define BMP_SCK 13
// #define BMP_MISO 12
// #define BMP_MOSI 11
// #define BMP_CS 10

#define LORA_RX 2
#define LORA_TX 3

// using only 1 core for learning purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Global objects
// Adafruit_BMP3XX bmp;
// RYLR896 lora(LORA_RX, LORA_TX);


void sendLoRaTask(void *pvParameters) {
  while (1) {
    // if (xSemaphoreTake(dataReadySemaphore, portMAX_DELAY) == pdTRUE) {
    //   String message = "Altitude: " + String(altitude, 2) + " m";
    //   lora.transmit(message);
    //   Serial.println("Sent: " + message);
    // }

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

    vTaskDelay(pdMS_TO_TICKS(5000)); // Send every 5 seconds
  }
}

void setup() {
  Serial.begin(115200);
  
  Wire.begin();

  // Initialize LoRa
  lora.begin(915000000); // Set frequency to 915 MHz
  lora.stringPacketWTime("WU",7);


  xTaskCreate(
    sendLoRaTask, 
    "SendLoRa", 
    2048, 
    NULL, 
    1, 
    NULL
  );
}

void loop() {
  // Empty. Tasks are handled by FreeRTOS
}
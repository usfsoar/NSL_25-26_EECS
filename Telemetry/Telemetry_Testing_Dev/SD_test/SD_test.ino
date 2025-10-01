#include <Arduino.h>
#include "V1_SOAR_RTOS_SD_CARD.h"

#define SD_CS 5

SOAR_SD_CARD sd(SD_CS);

void setup() {
  Serial.begin(115200);
  delay(1000);

  sd.begin(); 


  sd.writeFile("/hello.txt", "Hello from ESP32!\n");

  sd.appendFile("/hello.txt", "Appended line.\n");

  sd.readFile("/hello.txt");

  sd.listDir("/", 1);

  sd.renameFile("/hello.txt", "/renamed.txt");

  sd.readFile("/renamed.txt");

  sd.testFileIO("/speedtest.bin");

  sd.deleteFile("/renamed.txt");

  Serial.println("SD test complete.");
}

void loop() {
}

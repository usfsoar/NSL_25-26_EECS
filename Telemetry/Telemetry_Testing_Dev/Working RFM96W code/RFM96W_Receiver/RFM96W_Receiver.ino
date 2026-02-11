// Receiver

#include <SPI.h>
#include <RH_RF95.h>
#include "V1_SOAR_RTOS_SD_CARD.h"
#include <string.h>
#include "_config.h"

#define RFM96W_SCK   D8
#define RFM96W_MISO  D9
#define RFM96W_MOSI  D10

#define RFM96W_CS   D0
#define RFM96W_RST  D2
#define RFM96W_INT  D3

#define RF96W_FREQ 433.0

RH_RF95 rf96w(RFM96W_CS, RFM96W_INT);
SOAR_SD_CARD sd(D1, false);
int messages = 0;

void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

  Serial.begin(115200);
  sd.begin();
  sd.deleteFile(TEST_FILEPATH);
  sd.deleteFile(TEST2_FILEPATH);
  sd.writeFile(TEST_FILEPATH, "test\n");
  sd.writeFile(TEST2_FILEPATH, "test2\n");


  while (!Serial && millis() < 2000) {}

  SPI.begin(RFM96W_SCK, RFM96W_MISO, RFM96W_MOSI, RFM96W_CS);

  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);

  if (!rf96w.init()) {
    Serial.println("RFM96W initialization failed");
    while (1) delay(100);
  }
  Serial.println("RFM96W initialization succeeded");

  if (!rf96w.setFrequency(RF96W_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) delay(100);
  }
  rf96w.setModemConfig(RH_RF95::Bw125Cr45Sf128);

  Serial.print("Receiving frequency set to ");
  Serial.println(RF96W_FREQ);
}



void loop() {
  if (rf96w.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf96w.recv(buf, &len)) {
      buf[len] = 0;
      Serial.printf("Received: %s\n", (char*)buf);
      switch (buf[0]) {
        case '0':
          break;
        case '1':
          break;
        case '2':
          break;
        case '3':
          break;
      }

      if (buf[0] == '0') {
        sd.appendFile(TEST_FILEPATH, (char*)buf);
        sd.appendFile(TEST_FILEPATH, "\n");
      } else if (buf[0] == '1') {
        sd.appendFile(TEST2_FILEPATH, (char*)buf);
        sd.appendFile(TEST2_FILEPATH, "\n");
      }

    }
  }
}

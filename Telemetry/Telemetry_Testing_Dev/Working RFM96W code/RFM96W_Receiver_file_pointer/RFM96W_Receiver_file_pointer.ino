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

#define SD_CS       D1

#define RF96W_FREQ 433.0

RH_RF95 rf96w(RFM96W_CS, RFM96W_INT);
SOAR_SD_CARD sd(D1);

FsFile testFile;
FsFile test2File;

static uint32_t count0 = 0, count1 = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(RFM96W_CS, OUTPUT);
  digitalWrite(RFM96W_CS, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

  SPI.begin(RFM96W_SCK, RFM96W_MISO, RFM96W_MOSI);

  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);

  sd.begin();

  sd.remove(TEST_FILEPATH);
  sd.remove(TEST2_FILEPATH);

  sd.open(testFile,  TEST_FILEPATH,  O_WRONLY | O_CREAT | O_AT_END);
  sd.open(test2File, TEST2_FILEPATH, O_WRONLY | O_CREAT | O_AT_END);

  sd.append(testFile,  "test\n");
  sd.append(test2File, "test2\n");
  testFile.sync();
  test2File.sync();

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
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  bool got = false;

  if (rf96w.available()) {
    got = rf96w.recv(buf, &len);
  }

  if (!got) return;

  if (len >= RH_RF95_MAX_MESSAGE_LEN) len = RH_RF95_MAX_MESSAGE_LEN - 1;
  buf[len] = 0;

  Serial.print("Received: ");
  Serial.println((char*)buf);

  if (buf[0] == '0') {
    sd.append(testFile, (char*)buf);
    sd.append(testFile, "\n");
    count0++;
  } else if (buf[0] == '1') {
    sd.append(test2File, (char*)buf);
    sd.append(test2File, "\n");
    count1++;
  }

  if (count0 == 100) {
    sd.sync(testFile);
    count0 = 0;
  }

  if (count1 == 100) {
    sd.sync(test2File);
    count1 = 0;
  }
}
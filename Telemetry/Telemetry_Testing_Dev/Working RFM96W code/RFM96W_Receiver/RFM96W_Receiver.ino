// Receiver

#include <SPI.h>
#include <RH_RF95.h>

#define RFM96W_SCK   D8
#define RFM96W_MISO  D9
#define RFM96W_MOSI  D10

#define RFM96W_CS   D0
#define RFM96W_RST  D2
#define RFM96W_INT  D3

#define RF96W_FREQ 433.0

RH_RF95 rf96w(RFM96W_CS, RFM96W_INT);

void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

  Serial.begin(115200);
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
      Serial.print("Received: ");
      Serial.println((char*)buf);
    }
  }
}
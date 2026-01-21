// Sender

#include <SPI.h>
#include <RH_RF95.h>

#define RFM96W_CS 36
#define RFM96W_RST 9
#define RFM96W_INT 2

#define RFM96W_FREQ 433.0

RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);

void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);
  
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}

  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);

  if (!rfm96w.init()) {
    Serial.println("RFM96W initialization failed");
    while (1) delay(100);
  }
  Serial.println("RFM96W initialization succeeded");

  if (!rfm96w.setFrequency(RFM96W_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) delay(100);
  }
  rfm96w.setModemConfig(RH_RF95::Bw125Cr45Sf128);

  rfm96w.setTxPower(20, false); // 20 dBm

  Serial.print("Transmit frequency set to ");
  Serial.println(RFM96W_FREQ);
}

void loop() {
  const char* msg = "Hello!";
  Serial.print("Sending: ");
  Serial.println(msg);

  rfm96w.send((uint8_t*)msg, strlen(msg));
  rfm96w.waitPacketSent();

  delay(1000);
}
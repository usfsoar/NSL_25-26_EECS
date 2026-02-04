// Sender

#include <SPI.h>
#include <RH_RF95.h>

#define RFM96W_CS 36
#define RFM96W_RST 9
#define RFM96W_INT 2

float RFM96W_FREQ = 433.0;
char* msg = (char*)malloc(sizeof (int) + 5 * sizeof (char));
int id = 1;

RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}

  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

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
  sprintf(msg, "%dData", id);
  Serial.print("Sending: ");
  Serial.println(msg);
  Serial.println(strlen(msg));

  rfm96w.send((uint8_t*)msg, strlen(msg));
  rfm96w.waitPacketSent();

  id++;
  delay(100); // sending speed
}
/* Restart function */
/*
void restartMCU() {
    SCB_AIRCR = 0x05FA0004; 
    while (1) {}
}
*/

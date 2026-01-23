// Sender

#include <SPI.h>
#include <RH_RF95.h>
#include <V1_SOAR_RTOS_SD_CARD.h>
#include <string.h>
#include "_config.h"

#define RFM96W_CS 36
#define RFM96W_RST 9
#define RFM96W_INT 2
float RFM96W_FREQ 433.0

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

void looptask() {
  const char* msg = "0DATA";
  const char* msg2 = "1DATA";
  Serial.print("Sending: ");
  Serial.println(msg);

  rfm96w.send((uint8_t*)msg, strlen(msg));
  rfm96w.waitPacketSent();

  delay(100); // sending speed

  Serial.print("Sending: ");
  Serial.println(msg2);

  rfm96w.send((uint8_t*)msg2, strlen(msg2));
  rfm96w.waitPacketSent();

  delay(100); // sending speed
}

void sendCommands(void) {
    if (serialAvailable()) {
        Serial.print("Sending: ");
        String command1 = Serial.readStringUntil('\n');

    Serial.println(command1.c_str());
    switch (command1[0]) {
        case 'freq':
        Serial.println("frequency to: ");
        command2 = Serial.readStringUntil('\n');
        Serial.println(command2.c_str());
        RFM96W_FREQ = command2.toFloat();
        break;
        case 'test':
        Serial.println("testing...");
        rfm96w.send((uint8_t*)"TEST", 4);
        rfm96w.waitPacketSent();
          if (rf96w.available()) {
            uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
            uint8_t len = sizeof(buf);

                if (rf96w.recv(buf, &len)) {
                    buf[len] = 0;
                    Serial.print("Received: ");
                    Serial.println((char*)buf);
                }
            }
            break;
        case 'restart':
            Serial.println("Restarting..., waiting for ping");
            rfm96w.send((uint8_t*)"RESTART", 7);
            break;
    rfm96w.send((uint8_t*)command1.c_str(), command1.length());
    rfm96w.waitPacketSent();
    rfm96w.changeChannel();


        }
    }
}
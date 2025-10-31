/*
  Demonstrates wake-up, transmit, receive, and frequency setting.
*/

#include "RDA818.h"

// Pin assignments. Ask Alan about changing these
#define PDN_PIN 5
#define PTT_PIN 6
#define SQ_PIN  7

// Create radio object (using Serial1 for communication)
RDA818 radio(Serial1, PDN_PIN, PTT_PIN, SQ_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("---- RDA818 Transceiver Simulation ----");

  // change values later
  radio.initialize(435.000, true, 6);

  radio.wakeUp();

 //change frequency later 
  radio.setFrequency(435.000);
}

void loop() {
  
  radio.update();

  // Simulate transmission test every few seconds
  static unsigned long lastAction = 0;
  if (millis() - lastAction > 4000) {
    lastAction = millis();

    // Alternate between transmitting and receiving
    static bool txMode = false;
    txMode = !txMode;

    if (txMode) {
      radio.transmit(true);
    } else {
      radio.transmit(false);
      radio.checkSignal(); // Simulated receive check
    }
  }
}

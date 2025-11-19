/*
  - Uses SoftwareSerial as a temporary serial on pins TX=2, RX=3 
  - Prefers HardwareSerial (Serial1, Serial2, etc.). Confirm with Alan 

  Talk to Alan 
  - Confirm PD polarity (HIGH = awake assumed). 
  - Confirm PTT polarity (LOW = TX assumed). 
  - Confirm exact AT command syntax for your DRA818 variant. Some firmware use different commands.
  - Confirm VBAT wiring & voltage externally, code does not power the module via any pin.
  - SQ pin is optional, wire it later and pass that pin to the constructor.
  -Chekc if teensy has RX/TX wired to a hardware Serial
*/

#define USE_SOFTWARE_SERIAL 1

#include <Arduino.h>
#include "DRA818.h"

#if USE_SOFTWARE_SERIAL
  // SoftwareSerial constructor: SoftwareSerial(rxPin, txPin)
  
  #include <SoftwareSerial.h>
  const int RADIO_RX_PIN = 3; 
  const int RADIO_TX_PIN = 2; 
  SoftwareSerial swRadio(RADIO_RX_PIN, RADIO_TX_PIN); // RX, TX
  Stream &radioSerial = swRadio;
#else
#endif

// Pin assignments 
const int PIN_PD  = 51; // PD pin (Power Down / enable)
const int PIN_PTT = 4;  // PTT pin (Push To Talk)
// Check SQ
const int PIN_SQ  = -1; // change later 

// Create the driver object 
DRA818 radio(radioSerial, PIN_PD, PIN_PTT, RADIO_RX_PIN, RADIO_TX_PIN, PIN_SQ);

void setup() {
  Serial.begin(115200);
  while (!Serial) 
  Serial.println("=== DRA818 driver example (Teensy 4.1) ===");

  // Initialize the serial used to talk to the radio.
  // If using SoftwareSerial, you must begin it here.
  #if USE_SOFTWARE_SERIAL
    // NOTE: SoftwareSerial reliability on Teensy may be limited at higher baud rates.
    // If your board wiring supports it, prefer a HardwareSerial (Serial1, Serial2...).
    swRadio.begin(9600); // typical default for many RDA/DRA modules; confirm for your module
  #else
  #endif

  // Initialize radio driver: freq 435.000 MHz, high power, volume level 6
  radio.begin(435.000, true, 6);

  radio.wakeUp();
 
  radio.setFrequency(435.000); // Frequency is 144, regulation frequency 
}

void loop() {
  
  radio.update();

  
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    static bool tx = false;
    tx = !tx;
    if (tx) {
      Serial.println("[Sketch] ---> starting transmission for 2s (simulated)");
      radio.transmit(true);
      delay(2000); // hold TX for 2 seconds (non-ideal blocking demo; in real code avoid delays)
      radio.transmit(false);
      Serial.println("[Sketch] <--- stopped transmission");
    } else {
      // Check squelch 
      bool signal = radio.checkSquelch();
      Serial.print("[Sketch] squelch says signal present? ");
      Serial.println(signal ? "YES" : "NO");
    }
  }

 
}

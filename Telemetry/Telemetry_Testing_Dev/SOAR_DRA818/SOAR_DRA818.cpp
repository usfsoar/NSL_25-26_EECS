/*
  RDA818.cpp
  ----------------------------------------
  Implements the RDA818 class functions.
  Currently runs as a simulated prototype (no hardware needed).
*/

#include "RDA818.h"

// Constructor: save pin assignments and serial reference
RDA818::RDA818(HardwareSerial &serialPort, int pdnPin, int pttPin, int squelchPin)
: _serial(serialPort), _pdnPin(pdnPin), _pttPin(pttPin), _squelchPin(squelchPin),
  _isTransmitting(false), _isAwake(false) {}

// Initialize function 
void RDA818::initialize(float freqMHz, bool highPower, int volumeLevel) {
    pinMode(_pdnPin, OUTPUT);
    pinMode(_pttPin, OUTPUT);
    pinMode(_squelchPin, INPUT_PULLUP);

    digitalWrite(_pdnPin, LOW);  // start powered off
    digitalWrite(_pttPin, HIGH); // not transmitting
    _serial.begin(9600);

    Serial.println("RDA818 initialized (simulation mode)");
    Serial.print("Frequency: "); Serial.print(freqMHz); Serial.println(" MHz");
    Serial.print("Power mode: "); Serial.println(highPower ? "High" : "Low");
    Serial.print("Volume level: "); Serial.println(volumeLevel);
}

// Wake-up function: simulates powering on module
void RDA818::wakeUp() {
    digitalWrite(_pdnPin, HIGH);
    _isAwake = true;
    Serial.println("[RDA818] Module is awake!");
}

// Sleep function: powers down module
void RDA818::sleep() {
    digitalWrite(_pdnPin, LOW);
    _isAwake = false;
    Serial.println("[RDA818] Module is sleeping...");
}

// Set frequency
void RDA818::setFrequency(float freqMHz) {
    if (!_isAwake) {
        Serial.println("[RDA818] Cannot set frequency while asleep.");
        return;
    }

    Serial.print("[RDA818] Setting frequency to ");
    Serial.print(freqMHz);
    Serial.println(" MHz");

    // Simulated command (would send AT command in real hardware)
    sendCommand("AT+DMOSETGROUP=0," + String(freqMHz, 4) + "," + String(freqMHz, 4) + ",0000,1,0000");
}

// Transmit control
void RDA818::transmit(bool enable) {
    if (!_isAwake) {
        Serial.println("[RDA818] Cannot transmit while asleep.");
        return;
    }

    _isTransmitting = enable;
    digitalWrite(_pttPin, enable ? LOW : HIGH); // LOW = TX active
    Serial.println(enable ? "[RDA818] Transmitting..." : "[RDA818] Transmission stopped.");
}

// Simulated signal reception
void RDA818::checkSignal() {
    if (!_isAwake) {
        Serial.println("[RDA818] Cannot check signal while asleep.");
        return;
    }

    // For simulation: randomly determine signal presence
    bool signalDetected = random(0, 10) > 6; // 30% chance, ask Alan if this is okay
    if (signalDetected) {
        Serial.println("[RDA818] Incoming signal detected!");
    } else {
        Serial.println("[RDA818] No signal detected.");
    }
}

// Non-blocking update function
void RDA818::update() {
    if (_serial.available()) {
        String response = _serial.readStringUntil('\n');
        Serial.print("[RDA818 Response] ");
        Serial.println(response);
    }
}

// Helper function to send simulated AT commands
void RDA818::sendCommand(const String &cmd) {
    _serial.print(cmd);
    _serial.print("\r\n");
    Serial.print("[Command Sent] ");
    Serial.println(cmd);
}

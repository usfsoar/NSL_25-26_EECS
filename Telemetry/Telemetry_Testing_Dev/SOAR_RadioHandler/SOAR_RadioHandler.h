#ifndef RADIO_HANDLER_H
#define RADIO_HANDLER_H

#include <Arduino.h>


// Pin Assignments for Teensy 4.1 (hardware UART Serial4)
static const int PTT_PIN = 6;    // PTT control (LOW = TX, HIGH = RX)
static const int RX_PIN  = 14;   // Radio → Teensy  (Teensy RX4)
static const int TX_PIN  = 15;   // Teensy → Radio  (Teensy TX4)
static const int SQ_PIN  = -1;   // No squelch pin used (always treat as open)

// Radio operating modes

enum RadioMode {
    RADIO_TX_MODE,
    RADIO_RX_MODE
};


// RadioHandler class prototype
class RadioHandler {
public:
    RadioHandler(HardwareSerial& serial);

    void begin(unsigned long baud = 9600);

    void setMode(RadioMode mode);

    void sendMessage(const String& msg);

    bool available();

    String readMessage();

private:
    HardwareSerial& _serial;

    RadioMode _mode;

    String _messageBuffer;
};

#endif

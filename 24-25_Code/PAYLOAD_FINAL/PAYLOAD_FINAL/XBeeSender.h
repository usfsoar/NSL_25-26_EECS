#ifndef XBEESENDER_H
#define XBEESENDER_H

#include <Arduino.h>
#include <HardwareSerial.h>

class XBeeSender {
public:
    XBeeSender(HardwareSerial& serialPort, int rxPin, int txPin, const uint8_t* destAddress); 
    void begin(unsigned long baudRate); //use this function once at the start
    void send(const char* payload); //this function expects a const char* variable so make sure it's type casted as that

private:
    HardwareSerial& xbeeSerial;
    int rxPin;
    int txPin;
    uint8_t destAddr[8];
};

#endif

/*
  Prototype RDA818 
*/

#ifndef RDA818_H
#define RDA818_H

#include <Arduino.h>

class RDA818 {
public:
    // Constructor: creates a radio object with pin connections
    RDA818(HardwareSerial &serialPort, int pdnPin, int pttPin, int squelchPin);
    void initialize();float freqMHz, bool highPower, int volumeLevel);
    void wakeUp();
    void sleep();
    void setFrequency(float freqMHz);
    void transmit(bool enable);
    void checkSignal();
    void update();

    
private:
    HardwareSerial &_serial;  // reference to the Serial interface (e.g., Serial1)
    int _pdnPin;              // power down
    int _pttPin;              // push to talk
    int _squelchPin;          // squelch
    bool _isTransmitting;     // 
    bool _isAwake;            // 

    // Helper to send serial AT commands (for real module use)
    void sendCommand(const String &cmd);
};

#endif

/*
  Assumptions:
   - PD: active high
   - PTT: active low to transmit
   - SQ not connected in schematic; handled as optional (pin = -1).
   - VBAT logic / control not implemented here
   - Baud rate: 9600, change if needed
*/

#ifndef DRA818_H
#define DRA818_H

#include <Arduino.h>

class DRA818 {
public:
  DRA818(Stream &serialPort, int pdPin, int pttPin, int rxPin, int txPin, int sqPin = -1);

  // Initialize the driver and IO pins. Should be called in setup().
  void begin(float freqMHz = 435.000, bool highPower = true, int volume = 6);

  // PD polarity is assumed HIGH = awake. Confirm with hardware. If PD is active-low,
  // change logic in implementation or invert pdActiveHigh flag.
  void wakeUp();
  void sleep();
  void setFrequency(float freqMHz);

  // assumes PTT active LOW to transmit (common). Confirm polarity on hardware.
  void transmit(bool enable);
 
  bool checkSquelch();


  void update();

  void sendATCommand(const String &cmd);

  // Accessors
  bool isAwake() const { return _isAwake; }
  bool isTransmitting() const { return _isTransmitting; }

private:
  Stream &_radioSerial;   // abstract serial for radio communication
  int _pdPin;
  int _pttPin;
  int _rxPin;             // for documentation only (serial wiring)
  int _txPin;             // for documentation only
  int _sqPin;
  bool _isAwake;
  bool _isTransmitting;

  // Internal helper to write to radio and also log to Serial for debug.
  void _writeRadio(const String &s);

  // Internal helper: small delay after sending AT commands (module may need it)
  void _pulseDelay(unsigned long ms = 50);
};

#endif // DRA818_H

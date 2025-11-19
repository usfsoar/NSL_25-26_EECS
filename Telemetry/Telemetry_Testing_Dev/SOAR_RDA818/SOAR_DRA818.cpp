
#include "DRA818.h"

// Constructor
DRA818::DRA818(Stream &serialPort, int pdPin, int pttPin, int rxPin, int txPin, int sqPin)
  : _radioSerial(serialPort),
    _pdPin(pdPin),
    _pttPin(pttPin),
    _rxPin(rxPin),
    _txPin(txPin),
    _sqPin(sqPin),
    _isAwake(false),
    _isTransmitting(false) {}

// Initialize pins and basic radio settings.
// assume VBAT is handled externally
void DRA818::begin(float freqMHz, bool highPower, int volume) {
  Serial.println("[DRA818] begin() called");


  pinMode(_pdPin, OUTPUT);
  pinMode(_pttPin, OUTPUT);

  // If SQ pin provided, set as input
  if (_sqPin >= 0) {
    pinMode(_sqPin, INPUT);
  }

  // Assumptions: PD HIGH = awake; PTT HIGH = idle (no TX). Confirm PD polarity
  digitalWrite(_pdPin, LOW);   
  digitalWrite(_pttPin, HIGH); 

  // If using a SoftwareSerial or non-HardwareSerial, make sure that object was started
  // in the sketch before calling begin. The sketch
  // should initialize its Serial object before passing it in.
  Serial.println("[DRA818] Radio serial must be initialized in the sketch (e.g., Serial1.begin(9600) or swSerial.begin(9600))");


  wakeUp();


  setFrequency(freqMHz);

 
  // verify the correct AT syntax for your DRA818 variant.
  sendATCommand(String("AT+DMOSETVOLUME=") + String(volume));

  // High power setting (if supported by module firmware)
  if (highPower) {

    sendATCommand("AT+POWER=HIGH"); //  replace with real command if different
  } else {
    sendATCommand("AT+POWER=LOW");  //  replace with real command if different
  }


}

// Wake the module from PD (power down) mode
void DRA818::wakeUp() {
  
  digitalWrite(_pdPin, HIGH);
  _pulseDelay(10); 
  _isAwake = true;
  Serial.println("[DRA818] wakeUp(): PD set HIGH .");
  _pulseDelay(200);
  Serial.println("[DRA818] radio should be awake now .");
}

// Put the module into sleep / low-power mode
void DRA818::sleep() {
  digitalWrite(_pttPin, HIGH); // ensure PTT released
  digitalWrite(_pdPin, LOW);   // PD LOW => sleep (assumption)
  _isAwake = false;
  _isTransmitting = false;
  Serial.println("[DRA818] sleep(): PD set LOW (assumption: LOW = sleep).");
}

// NOTE: Actual AT command format may differ by firmware. Confirm with module docs.
void DRA818::setFrequency(float freqMHz) {
  if (!_isAwake) {
    Serial.println("[DRA818] setFrequency(): radio is asleep. Call wakeUp() before setting frequency.");
    return;
  }

  String freqStr = String(freqMHz, 4);
  Serial.print("[DRA818] setFrequency(): setting to ");
  Serial.print(freqStr);
  Serial.println(" MHz");

  // Example AT command for DMO (simplex) group set - replace if your module needs different syntax.
  // Format used earlier in prototypes: AT+DMOSETGROUP=0,<txFreq>,<rxFreq>,0000,1,0000
  String at = "AT+DMOSETGROUP=0," + freqStr + "," + freqStr + ",0000,1,0000";
  sendATCommand(at);

  // give module a bit of time to apply setting
  _pulseDelay(50);
}

// Control transmit state (PTT). Assumes PTT LOW = transmit.
//  Confirm actual PTT polarity on your board and invert if necessary.
void DRA818::transmit(bool enable) {
  if (!_isAwake) {
    Serial.println("[DRA818] transmit(): radio asleep — cannot transmit. Call wakeUp() first.");
    return;
  }

  _isTransmitting = enable;
  if (enable) {
    // drive PTT to active state; here assumed LOW = TX active
    digitalWrite(_pttPin, LOW);
    Serial.println("[DRA818] transmit(true): PTT asserted (LOW assumed = TX).");
  } else {
    // release PTT
    digitalWrite(_pttPin, HIGH);
    Serial.println("[DRA818] transmit(false): PTT released (HIGH assumed = RX/idle).");
  }
}


// If _sqPin < 0 (not wired), prints a TODO and returns false.
bool DRA818::checkSquelch() {
  if (_sqPin < 0) {
    Serial.println("[DRA818] checkSquelch(): SQ pin not configured. Connect SQ to read squelch state.");
    return false;
  }
  int val = digitalRead(_sqPin);
  //  Confirm module's SQ behavior. Here we assume HIGH = signal present.
  bool signalPresent = (val == HIGH);
  Serial.print("[DRA818] checkSquelch(): SQ pin read = ");
  Serial.print(val);
  Serial.print(" => signalPresent=");
  Serial.println(signalPresent ? "true" : "false");
  return signalPresent;
}

// Non-blocking update: read any radio serial responses
void DRA818::update() {
  while (_radioSerial.available()) {
    // Read a single line or chunk and forward to main Serial for logging
    String rsp = _radioSerial.readStringUntil('\n');
    rsp.trim();
    if (rsp.length() > 0) {
      Serial.print("[DRA818 RX] ");
      Serial.println(rsp);
    }
  }
}

// Send an AT command to the radio. Adds CR+LF and logs to Serial.
void DRA818::sendATCommand(const String &cmd) {
  if (!_isAwake) {
    Serial.println("[DRA818] sendATCommand(): radio asleep — waking up first.");
    wakeUp();
  }
  _writeRadio(cmd);
  _pulseDelay(40);
}

// Internal helper to write to radio and log
void DRA818::_writeRadio(const String &s) {
  // Most radio modules expect "\r\n" line endings.
  _radioSerial.print(s);
  _radioSerial.print("\r\n");

  // Mirror what we sent to main Serial for debug
  Serial.print("[DRA818 TX] ");
  Serial.println(s);
}

void DRA818::_pulseDelay(unsigned long ms) {
  delay(ms);
}

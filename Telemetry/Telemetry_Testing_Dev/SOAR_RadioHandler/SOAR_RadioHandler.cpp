#include "RadioHandler.h"

// Constructor

RadioHandler::RadioHandler(HardwareSerial& serial)
: _serial(serial),
  _mode(RADIO_RX_MODE),
  _messageBuffer("")
{}

// Initialize radio Handler

void RadioHandler::begin(unsigned long baud)
{
    // Configure PTT pin
    pinMode(PTT_PIN, OUTPUT);
    digitalWrite(PTT_PIN, HIGH);   // HIGH = RX mode (PTT inactive)

    // No SQ pin used
    // If have SQ_PIN: pinMode(SQ_PIN, INPUT);

    // Start hardware serial
    _serial.begin(baud);

    _messageBuffer.reserve(128);
    _messageBuffer = "";
}


// Switch between TX and RX mode
void RadioHandler::setMode(RadioMode mode)
{
    if (mode == RADIO_TX_MODE) {
        digitalWrite(PTT_PIN, LOW);   // LOW = Transmit
    }
    else {
        digitalWrite(PTT_PIN, HIGH);  // HIGH = Receive
    }

    _mode = mode;
}



// Send message over radio

void RadioHandler::sendMessage(const String& msg)
{
    // Ensure TX mode
    setMode(RADIO_TX_MODE);

    _serial.print(msg);
    _serial.print('\n');

    // Wait for UART to finish sending before returning to RX
    _serial.flush();

    setMode(RADIO_RX_MODE);
}


// Check if a full message has been received
// ------------------------------------------------------------
bool RadioHandler::available()
{
    while (_serial.available()) {

        char c = _serial.read();

        if (c == '\n') {
            return true;    // complete message received
        }

        if (c >= 32 && c <= 126) {
            _messageBuffer += c;
        }

        // Safety overflow reset
        if (_messageBuffer.length() > 200) {
            _messageBuffer = "";
            return false;
        }
    }

    return false;
}


// Retrieve received message
String RadioHandler::readMessage()
{
    String msg = _messageBuffer;
    _messageBuffer = "";
    return msg;
}

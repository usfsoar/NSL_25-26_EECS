#include "XBeeSender.h"

XBeeSender::XBeeSender(HardwareSerial& serialPort, int rx, int tx, const uint8_t* destAddress)
    : xbeeSerial(serialPort), rxPin(rx), txPin(tx) {
    memcpy(destAddr, destAddress, 8);
}

void XBeeSender::begin(unsigned long baudRate) { //use this function once at the start
    xbeeSerial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
    delay(2000);  // Give it a moment to start up
}

void XBeeSender::send(const char* payload) { //this function expects a const char* variable so make sure it's type casted as that
    uint8_t frame[100];
    int len = strlen(payload);
    int i = 0;

    uint16_t dataLength = 14 + len;
    frame[i++] = 0x7E;
    frame[i++] = (dataLength >> 8) & 0xFF;
    frame[i++] = dataLength & 0xFF;

    frame[i++] = 0x10; // Frame type
    frame[i++] = 0x01; // Frame ID

    for (int j = 0; j < 8; j++) frame[i++] = destAddr[j];
    frame[i++] = 0xFF;
    frame[i++] = 0xFE;
    frame[i++] = 0x00; // Broadcast radius
    frame[i++] = 0x00; // Options

    for (int j = 0; j < len; j++) frame[i++] = payload[j];

    uint8_t checksum = 0;
    for (int j = 3; j < i; j++) checksum += frame[j];
    frame[i++] = 0xFF - checksum;

    xbeeSerial.write(frame, i);
    //Serial.println("API Frame sent!");
}
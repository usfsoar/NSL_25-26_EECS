#ifndef HEX_FUNCTIONS_H
#define HEX_FUNCTIONS_H

#include <stdint.h> // For uint8_t
#include <string.h> // For memcpy

void myhexfunc(const char* data);
int asciiToHexArray(const char* asciiStr, uint8_t* hexArray, size_t maxLen);
int asciiToBytes(const char* asciiStr, uint8_t* byteArray, size_t maxLen);
uint8_t calculateChecksum(const uint8_t* data, uint16_t len);

#endif // HEX_FUNCTIONS_H
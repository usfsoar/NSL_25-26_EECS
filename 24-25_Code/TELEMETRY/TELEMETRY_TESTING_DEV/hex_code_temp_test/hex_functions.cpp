#include <Arduino.h>
#include <string.h> // For memcpy
#include "hex_functions.h"

void myhexfunc(const char* data) {
  // Define the 13-byte prefix to prepend
  uint8_t prefix[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00 };
  const size_t prefixLen = sizeof(prefix);

  // Convert the ASCII string to. a hex array.
  Serial.print("XBeeArduino Frame builder: Data string (ASCII):");
  Serial.println(data);
  Serial.println("XBeeArduino Frame builder: Data string (ASCII):");
  uint8_t asciiHex[64] = { 0 };
  // asciiToHexArray(data, asciiHex, sizeof(asciiHex));
  asciiToBytes(data, asciiHex, sizeof(asciiHex));
  Serial.println("XBeeArduino Frame builder: Data string (Hex):");

  char buffer[128];
  size_t offset = 0;
  for (uint16_t i = 0; i < sizeof(asciiHex); i++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "0x%02X ", asciiHex[i]);
    if (offset >= sizeof(buffer)) {
      break; // Prevent buffer overflow
    }
  }
  Serial.println(buffer);

  uint8_t finalPayload[128] = { 0 };
  memcpy(finalPayload, prefix, prefixLen);
  memcpy(finalPayload + prefixLen, asciiHex, sizeof(asciiHex));

  uint16_t finalLength = prefixLen + sizeof(asciiHex);

  Serial.println("Final Payload (Hex):");
  offset = 0;
  for (uint16_t i = 0; i < finalLength; i++) {
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "0x%02X ", finalPayload[i]);
    if (offset >= sizeof(buffer)) {
      break; // Prevent buffer overflow
    }
  }
  Serial.println(buffer);
}

int asciiToHexArray(const char* asciiStr, uint8_t* hexArray, size_t maxLen) {
  if (!asciiStr || !hexArray) return -1;

  size_t asciiLen = strlen(asciiStr);
  size_t hexIndex = 0;

  for (size_t i = 0; i < asciiLen && hexIndex < maxLen; ) {
    // Skip non-hex characters
    while (i < asciiLen && !isxdigit(asciiStr[i])) i++;
    if (i >= asciiLen) break;

    char high = toupper(asciiStr[i++]);

    while (i < asciiLen && !isxdigit(asciiStr[i])) i++;
    char low = (i < asciiLen) ? toupper(asciiStr[i++]) : '0';  // Pad with '0' if odd length

    char hexByteStr[3] = { high, low, '\0' };
    hexArray[hexIndex++] = (uint8_t)strtol(hexByteStr, NULL, 16);
  }

  return hexIndex;
}


int asciiToBytes(const char* asciiStr, uint8_t* byteArray, size_t maxLen) {
  if (!asciiStr || !byteArray) return -1;
  size_t len = strlen(asciiStr);
  size_t copyLen = len < maxLen ? len : maxLen;
  memcpy(byteArray, asciiStr, copyLen);
  return copyLen;
}



/**
 * @brief Calculates the checksum for an API frame.
 *
 * This function computes the checksum for a given XBee API frame. The checksum
 * is calculated by summing the bytes of the frame starting from the fourth byte
 * (index 3) to the end of the frame and then subtracting the sum from 0xFF.
 * The resulting checksum ensures the integrity of the data in the API frame.
 *
 * @param[in] frame Pointer to the API frame data.
 * @param[in] len Length of the API frame data.
 *
 * @return uint8_t The calculated checksum value.
 */
uint8_t calculateChecksum(const uint8_t* frame, uint16_t len) {
  uint8_t sum = 0;
  for (uint16_t i = 3; i < len; i++) {
    sum += frame[i];
  }
  return 0xFF - sum;
}
// Small LoRa application-level protocol helpers
#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <stdint.h>
#include <string.h>
#include <LoRa.h>

// Packet types aligned to sensor types in V3_Telemetry_Bay
enum DataType : uint8_t {
  TYPE_IMU = 0,
  TYPE_ALTIMETER = 1,
  TYPE_GPS = 2,
  TYPE_GENERIC = 255
};

// Pack a data string with a 1-byte type header into outBuf.
// Returns total length written (type + data) or 0 on failure.
static inline size_t packData(uint8_t* outBuf, size_t outCap, DataType type, const char* data) {
  if (!outBuf || !data) return 0;
  size_t dlen = strlen(data);
  if (outCap < (dlen + 1)) return 0; // need room for type byte + data (no trailing NUL sent)
  outBuf[0] = (uint8_t)type;
  memcpy(outBuf + 1, data, dlen);
  return dlen + 1;
}

// Unpack a received buffer of length len. outData is null-terminated and will contain at most outCap-1 bytes.
// If outCap is zero, data is not written. outType receives the inferred DataType.
static inline void unpackData(const uint8_t* buf, uint8_t len, DataType* outType, char* outData, size_t outCap) {
  if (len == 0) {
    if (outType) *outType = TYPE_GENERIC;
    if (outCap) outData[0] = '\0';
    return;
  }
  if (outType) *outType = (DataType)buf[0];
  size_t dlen = (size_t)len - 1;
  if (outCap > 0 && outData) {
    size_t copy = (dlen < (outCap - 1)) ? dlen : (outCap - 1);
    if (copy > 0) memcpy(outData, buf + 1, copy);
    outData[copy] = '\0';
  }
}

#endif // LORA_PROTOCOL_H

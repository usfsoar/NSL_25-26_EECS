#ifndef V1_SOAR_RTOS_SD_CARD_H
#define V1_SOAR_RTOS_SD_CARD_H

#include <SdFat.h>

class SOAR_SD_CARD {
private:
  uint8_t _cs_pin;
  SdFs _sd;
  bool _is_builtin;
  
public:
  SOAR_SD_CARD(uint8_t cs_pin, bool is_builtin = false);
  bool begin();
  void writeFile(const char* path, const char* message);
  void appendFile(const char* path, const char* message);
  void deleteFile(const char* path);
  void appendBytes(const char* path, const uint8_t* data, size_t len);
};

#endif
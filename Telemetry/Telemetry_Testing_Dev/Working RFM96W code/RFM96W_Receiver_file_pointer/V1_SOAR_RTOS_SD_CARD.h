#ifndef V1_SOAR_RTOS_SD_CARD_H
#define V1_SOAR_RTOS_SD_CARD_H

#include <SdFat.h>

class SOAR_SD_CARD {
private:
  uint8_t _cs_pin;
  SdFs _sd;
  
public:
  SOAR_SD_CARD(uint8_t cs_pin);
  void begin();

  bool remove(const char* path);
  bool exists(const char* path);

  bool open(FsFile& file, const char* path, oflag_t flags);
  void close(FsFile& file);

  bool write(FsFile& file, const char* message);
  bool append(FsFile& file, const char* message);

  bool sync(FsFile& file);
};

#endif
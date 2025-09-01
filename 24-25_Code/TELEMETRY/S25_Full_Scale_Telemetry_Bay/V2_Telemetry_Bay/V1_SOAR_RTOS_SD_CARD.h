#ifndef SOAR_SD_CARD_H
#define SOAR_SD_CARD_H

#include "SdFat.h"


class SOAR_SD_CARD {
public:
  SOAR_SD_CARD(uint8_t cs_pin);
  void begin();
  void listDir(const char *dirname, uint8_t levels);
  void createDir(const char *path);
  void removeDir(const char *path);
  void readFile(const char *path);
  void writeFile(const char *path, const char *message);
  void appendFile(const char *path, const char *message);
  void renameFile(const char *path1, const char *path2);
  void deleteFile(const char *path);
  void testFileIO(const char *path);

private:
  uint8_t _cs_pin;
  SdFat SD;
};

#endif // SOAR_SD_CARD_H

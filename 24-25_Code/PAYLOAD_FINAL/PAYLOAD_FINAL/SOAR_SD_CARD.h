#ifndef SOAR_SD_CARD_H
#define SOAR_SD_CARD_H

#include "SdFat.h"
#include <SPI.h>

class SOAR_SD_CARD {
public:
  // Constructor
  SOAR_SD_CARD(uint8_t cs_pin, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin);

  // Initialization
  void begin();

  // File/Directory Operations
  bool exists(const char *path);
  void listDir(const char *dirname, uint8_t levels);
  void createDir(const char *path);
  void removeDir(const char *path);
  void readFile(const char *path);
  //void to bull writefile
  bool  writeFile(const char *path, const char *message);
  void appendFile(const char *path, const char *message);
  void renameFile(const char *path1, const char *path2);
  void deleteFile(const char *path);
  void testFileIO(const char *path);

  //new
  bool openLogFile(const char *path);
  bool logToFile(const char *message);
  bool syncLogFile();
  void closeLogFile();

  bool isReady();
  bool ready = false;


private:
  uint8_t _cs_pin;
  uint8_t _clk_pin;
  uint8_t _mosi_pin;
  uint8_t _miso_pin;
  SPIClass *_spi;
  SdFat SD;

  FsFile _logFile;  // File object for the currently open log file
  bool _logFileOpen;
};

#endif
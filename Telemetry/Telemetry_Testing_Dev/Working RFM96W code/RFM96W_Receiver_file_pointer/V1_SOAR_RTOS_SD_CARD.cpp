#include "V1_SOAR_RTOS_SD_CARD.h"

SOAR_SD_CARD::SOAR_SD_CARD(uint8_t cs_pin)
  : _cs_pin(cs_pin) {}

void SOAR_SD_CARD::begin() {
  Serial.print("Attempting to initialize SD card on pin ");
  Serial.println(_cs_pin);
  
  bool success;
  
  success = _sd.begin(SdSpiConfig(_cs_pin, SHARED_SPI, SD_SCK_MHZ(4)));
  
  if (!success) {
    Serial.print("Card Mount Failed on pin ");
    Serial.println(_cs_pin);
    
    if (_sd.card()->errorCode()) {
      Serial.print("SD errorCode: 0x");
      Serial.print(_sd.card()->errorCode(), HEX);
      Serial.print(", errorData: 0x");
      Serial.println(_sd.card()->errorData(), HEX);
    }
    return;
  }
  
  Serial.print("SD Card Initialized successfully on pin ");
  Serial.println(_cs_pin);
}

bool SOAR_SD_CARD::remove(const char* path) {
  return _sd.remove(path);
}

bool SOAR_SD_CARD::exists(const char* path) {
  return _sd.exists(path);
}

bool SOAR_SD_CARD::open(FsFile& file, const char* path, oflag_t flags) {
  // Close first if already open (safety)
  if (file) file.close();

  file = _sd.open(path, flags);

  if (!file) {
    Serial.print("Failed to open: ");
    Serial.println(path);
    return false;
  }
  return true;
}

void SOAR_SD_CARD::close(FsFile& file) {
  if (file) file.close();
}

bool SOAR_SD_CARD::write(FsFile& file, const char* message) {
  if (!file) return false;
  if (!file.seekSet(0)) return false;
  if (!file.truncate(0)) return false;
  return file.print(message) > 0;
}

bool SOAR_SD_CARD::append(FsFile& file, const char* message) {
  if (!file) return false;
  file.seekEnd();
  return file.print(message) > 0;
}

bool SOAR_SD_CARD::sync(FsFile& file) {
  if (!file) return false;
  return file.sync(); // commits to card
}
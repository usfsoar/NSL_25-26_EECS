#include "V1_SOAR_RTOS_SD_CARD.h"

SOAR_SD_CARD::SOAR_SD_CARD(uint8_t cs_pin, bool is_builtin) 
  : _cs_pin(cs_pin), _is_builtin(is_builtin) {}

void SOAR_SD_CARD::begin() {
  Serial.print("Attempting to initialize SD card on pin ");
  Serial.println(_cs_pin);
  
  bool success;
  
  if (_is_builtin) {
    // Built-in SD uses SDIO (faster, dedicated interface)
    success = _sd.begin(SdioConfig(FIFO_SDIO));
  } else {
    // External SD uses SPI
    success = _sd.begin(SdSpiConfig(_cs_pin, SHARED_SPI, SD_SCK_MHZ(4)));
  }
  
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

void SOAR_SD_CARD::writeFile(const char* path, const char* message) {
  _sd.remove(path);
  appendFile(path, message);
}

void SOAR_SD_CARD::appendFile(const char* path, const char* message) {
  FsFile file = _sd.open(path, O_RDWR | O_CREAT | O_AT_END);
  
  if (!file) {
    Serial.print("Failed to open file: ");
    Serial.println(path);
    return;
  }
  
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  
  file.close();
}

void SOAR_SD_CARD::deleteFile(const char* path) {
  if (_sd.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
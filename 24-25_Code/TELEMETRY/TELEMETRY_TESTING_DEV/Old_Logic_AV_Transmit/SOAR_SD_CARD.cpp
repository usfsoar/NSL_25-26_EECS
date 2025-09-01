#include "SOAR_SD_CARD.h"


SOAR_SD_CARD::SOAR_SD_CARD(uint8_t cs_pin) : _cs_pin(cs_pin) {}

void SOAR_SD_CARD::begin() {
  if (!SD.begin(_cs_pin)) {
    Serial.println("Card Mount Failed");
    return;
  }
  Serial.println("SD Card Initialized");
}

void SOAR_SD_CARD::listDir(const char* dirname, uint8_t levels) {
  FsFile dir, file;
  char nameBuffer[256]; // Buffer to store the file or directory name

  if (!dir.open(dirname)) {
    Serial.println("Failed to open directory");
    return;
  }

  while (file.openNext(&dir, O_RDONLY)) {
    file.getName(nameBuffer, sizeof(nameBuffer)); // Use getName to get the file name

    if (file.isDir()) {
      Serial.print("DIR : ");
      Serial.println(nameBuffer); // Use the buffer
      if (levels) {
        listDir(nameBuffer, levels - 1); // Recursively list directories
      }
    } else {
      Serial.print("FILE: ");
      Serial.print(nameBuffer); // Use the buffer
      Serial.print(" SIZE: ");
      Serial.println(file.fileSize());
    }
    file.close(); // Make sure to close the file when done
  }
  dir.close(); // Close the directory as well
}


void SOAR_SD_CARD::createDir(const char *path) {
  // Implementation remains the same as in the original function
  Serial.printf("Creating Dir: %s\n", path);
  if(SD.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void SOAR_SD_CARD::removeDir(const char *path) {
  // Implementation remains the same as in the original function
  Serial.printf("Removing Dir: %s\n", path);
  if(SD.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void SOAR_SD_CARD::readFile(const char *path) {
  FsFile file = SD.open(path, O_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}


void SOAR_SD_CARD::writeFile(const char *path, const char *message) {
  FsFile file = SD.open(path, O_CREAT | O_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();

}


void SOAR_SD_CARD::appendFile(const char *path, const char *message) {
  // Use FsFile for file operations with the SdFat library
  FsFile file = SD.open(path, O_RDWR | O_CREAT | O_AT_END);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}


void SOAR_SD_CARD::renameFile(const char *path1, const char *path2) {
  if (SD.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}


void SOAR_SD_CARD::deleteFile(const char *path) {
  if (SD.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}


void SOAR_SD_CARD::testFileIO(const char *path) {
  // Open file for writing (and reading in this context)
  FsFile file = SD.open(path, O_RDWR | O_CREAT);
  if (!file) {
    Serial.println("Failed to open file for performance test");
    return;
  }

  const size_t bufSize = 512;
  uint8_t buf[bufSize];
  memset(buf, 0, bufSize); // Clear buffer

  // Reading test
  size_t len = file.size();
  file.seek(0); // Go to the beginning of the file
  uint32_t start = millis();
  while (len > 0) {
    size_t toRead = min(bufSize, len);
    file.read(buf, toRead);
    len -= toRead;
  }
  uint32_t endRead = millis() - start;
  Serial.printf("%u bytes read in %u ms\n", file.size(), endRead);

  // Writing test
  file.seek(file.size()); // Go to the end of the file for append-like behavior
  start = millis();
  for (size_t i = 0; i < 2048; i++) {
    file.write(buf, bufSize);
  }
  file.flush(); // Ensure all data is written to the SD card
  uint32_t endWrite = millis() - start;
  Serial.printf("%u bytes written in %u ms\n", 2048 * bufSize, endWrite);

  file.close();
}


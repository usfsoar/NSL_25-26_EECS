#include "SOAR_SD_CARD.h"

// Constructor
SOAR_SD_CARD::SOAR_SD_CARD(uint8_t cs_pin, uint8_t clk_pin, uint8_t mosi_pin, uint8_t miso_pin)
  : _cs_pin(cs_pin), _clk_pin(clk_pin), _mosi_pin(mosi_pin), _miso_pin(miso_pin) {
  _spi = &SPI;
}

// Initialization func
void SOAR_SD_CARD::begin() {
  _spi->begin(_clk_pin, _miso_pin, _mosi_pin);
  Serial.println("Custom SPI pins initialized (CLK, MISO, MOSI).");

  Serial.println("Initializing SD card via SdFat...");
  SdSpiConfig sdFatConfig(_cs_pin, SHARED_SPI, SD_SCK_MHZ(20), _spi);

  if (!SD.begin(sdFatConfig)) {
    Serial.println("SdFat Card Mount Failed!");
    Serial.println("Check wiring, SD card formatting (FAT16/FAT32), and SPI pins.");
    SD.initErrorPrint(&Serial);
    ready = false;
    return;// 0;
  }
  uint8_t cardType = SD.card()->type();
  Serial.print("SD Card Type: ");
  if (cardType == SD_CARD_TYPE_SD1) {
    Serial.println("SD1");
  } else if (cardType == SD_CARD_TYPE_SD2) {
    Serial.println("SD2");
  } else if (cardType == SD_CARD_TYPE_SDHC) {
    Serial.println("SDHC/SDXC");
  } else {
    Serial.println("Unknown");
  }

  // Card Size Calculation
  uint64_t sectorCount = SD.card()->sectorCount();
  if (sectorCount == 0) {
    Serial.println("Error getting sector count.");
  } else {
    uint64_t cardSize = sectorCount * 512;  // Sector size is usually 512 bytes
    uint64_t cardSizeMB = cardSize / (1024 * 1024);
    Serial.print("SD Card Size: ");
    Serial.print(cardSizeMB);
    Serial.println(" MB");
  }

  ready = true;
  Serial.println("SD Card Initialized Successfully via SOAR_SD_CARD.");
  //return 1;
}

bool SOAR_SD_CARD::isReady() {
  return ready;
}

// Exists method
bool SOAR_SD_CARD::exists(const char *path) {
  return SD.exists(path);  // Use the internal SD (SdFat) object
}

void SOAR_SD_CARD::listDir(const char *dirname, uint8_t levels) {
  FsFile dir, file;
  char nameBuffer[256];  // Buffer to store the file or directory name

  if (!dir.open(dirname)) {
    Serial.println("Failed to open directory");
    return;
  }

  while (file.openNext(&dir, O_RDONLY)) {
    file.getName(nameBuffer, sizeof(nameBuffer));  // Use getName to get the file name

    if (file.isDir()) {
      Serial.print("DIR : ");
      Serial.println(nameBuffer);  // Use the buffer
      if (levels) {
        // Construct full path for recursion if needed, SdFat might handle relative
        // String nextPath = String(dirname) + "/" + nameBuffer;
        // listDir(nextPath.c_str(), levels - 1); // Recursively list directories
        Serial.println(" (Sub-directory listing not implemented in this example)");
      }
    } else {
      Serial.print("FILE: ");
      Serial.print(nameBuffer);  // Use the buffer
      Serial.print(" SIZE: ");
      Serial.println(file.fileSize());
    }
    file.close();  // Make sure to close the file when done
  }
  dir.close();  // Close the directory as well
}


void SOAR_SD_CARD::createDir(const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (SD.mkdir(path)) {  // Use internal SD object
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
    SD.errorPrint(&Serial);  // Print SdFat error
  }
}

void SOAR_SD_CARD::removeDir(const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (SD.rmdir(path)) {  // Use internal SD object
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
    SD.errorPrint(&Serial);  // Print SdFat error
  }
}

void SOAR_SD_CARD::readFile(const char *path) {
  FsFile file = SD.open(path, O_READ);  // Use internal SD object
  if (!file) {
    Serial.print("Failed to open file for reading: ");
    Serial.println(path);
    SD.errorPrint(&Serial);  // Print SdFat error
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println();  // Add newline after reading
  file.close();
}


/**
 * @brief Writes a null-terminated string to a file, overwriting existing content.
 * Creates the file if it doesn't exist.
 * @param path The full path to the file.
 * @param message The null-terminated string to write.
 * @return true if the file was opened and the message was written successfully, false otherwise.
 */
bool SOAR_SD_CARD::writeFile(const char *path, const char *message) {  // <-- Changed return type to bool
  // O_WRITE creates if not exists, O_TRUNC clears existing content
  FsFile file = SD.open(path, O_WRITE | O_CREAT | O_TRUNC);  // Use internal SD object
  if (!file) {
    Serial.print("Failed to open file for writing: ");
    Serial.println(path);
    SD.errorPrint(&Serial);  // Print SdFat error
    return false;            // <-- Return false on open failure
  }

  bool success = false;  // Flag to track success
  if (file.print(message)) {
    // Serial.println("File written"); // Less verbose
    success = true;  // <-- Mark success if print worked
  } else {
    Serial.println("Write failed");
    SD.errorPrint(&Serial);  // Print SdFat error
    success = false;         // <-- Mark failure (already false, but explicit)
  }

  // file.sync(); // Optional: Ensure data is written physically (slows down)
  file.close();  // Close the file regardless of write success

  return success;  // <-- Return the success status
}


void SOAR_SD_CARD::appendFile(const char *path, const char *message) {
  // O_WRITE | O_CREAT | O_APPEND opens for appending, creates if needed
  FsFile file = SD.open(path, O_WRITE | O_CREAT | O_APPEND);  // Use internal SD object
  if (!file) {
    Serial.print("Failed to open file for appending: ");
    Serial.println(path);
    SD.errorPrint(&Serial);  // Print SdFat error
    return;
  }
  if (file.print(message)) {
    // Serial.println("Message appended"); // Less verbose
  } else {
    Serial.println("Append failed");
    SD.errorPrint(&Serial);  // Print SdFat error
  }
  // file.sync(); // Ensure data is written physically (slows down)
  file.close();
}


void SOAR_SD_CARD::renameFile(const char *path1, const char *path2) {
  Serial.printf("Renaming %s to %s\n", path1, path2);
  if (SD.rename(path1, path2)) {  // Use internal SD object
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
    SD.errorPrint(&Serial);  // Print SdFat error
  }
}


void SOAR_SD_CARD::deleteFile(const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (SD.remove(path)) {  // Use internal SD object
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
    SD.errorPrint(&Serial);  // Print SdFat error
  }
}


void SOAR_SD_CARD::testFileIO(const char *path) {
  // O_RDWR | O_CREAT allows reading/writing, creates if needed
  FsFile file = SD.open(path, O_RDWR | O_CREAT);  // Use internal SD object
  if (!file) {
    Serial.println("Failed to open file for performance test");
    SD.errorPrint(&Serial);  // Print SdFat error
    return;
  }

  const size_t bufSize = 512;
  uint8_t buf[bufSize];
  size_t bytesRead;  // To store result of read()

  // Reading test
  uint64_t len = file.fileSize();  // Get file size before reading
  file.seek(0);                    // Go to the beginning of the file
  uint32_t startRead = millis();
  uint64_t totalRead = 0;
  while (totalRead < len) {
    bytesRead = file.read(buf, bufSize);
    if (bytesRead <= 0) break;  // End of file or error
    totalRead += bytesRead;
  }
  uint32_t endRead = millis() - startRead;
  Serial.printf("%llu bytes read in %lu ms\n", totalRead, endRead);

  // Writing test
  file.seek(file.size());  // Go to the end of the file for append-like behavior
  size_t totalWritten = 0;
  uint32_t startWrite = millis();
  for (size_t i = 0; i < 2048; i++) {  // Write 1MB
    int written = file.write(buf, bufSize);
    if (written != bufSize) {
      Serial.println("Write error during test!");
      SD.errorPrint(&Serial);
      break;
    }
    totalWritten += written;
  }
  if (!file.sync() || file.getWriteError()) {  // sync() pushes data and checks for write errors
    Serial.println("Sync/Write Error detected after test loop.");
    SD.errorPrint(&Serial);
  }
  uint32_t endWrite = millis() - startWrite;
  Serial.printf("%u bytes written in %lu ms\n", totalWritten, endWrite);

  file.close();
}


// --- New Methods for Optimized Logging ---

/**
 * @brief Opens a specific file for logging, keeping it open.
 * Overwrites the file if it exists.
 */
bool SOAR_SD_CARD::openLogFile(const char *path) {
  if (_logFileOpen) {
    Serial.println("Error: Log file already open. Close it first.");
    return false;
  }

  // O_WRITE | O_CREAT | O_TRUNC: Write access, create if needed, truncate to zero length
  if (_logFile.open(path, O_WRITE | O_CREAT | O_TRUNC)) {
    _logFileOpen = true;
    Serial.print("Log file opened: ");
    Serial.println(path);
    return true;
  } else {
    Serial.print("Failed to open log file: ");
    Serial.println(path);
    SD.errorPrint(&Serial);
    _logFileOpen = false;
    return false;
  }
}

/**
 * @brief Writes a message to the currently open log file (buffered).
 */
bool SOAR_SD_CARD::logToFile(const char *message) {
  if (!_logFileOpen) {
    Serial.println("Error: Log file not open for writing.");
    return false;
  }

  // Print the message to the file (goes to SdFat's buffer)
  size_t written = _logFile.print(message);

  // Check if the number of bytes written matches the length of the message
  // print() returns the number of bytes written
  if (written != strlen(message)) {
    if (_logFile.getWriteError()) {  // Check for underlying write error flag
      Serial.println("Write failed (Error Flag Set)");
      SD.errorPrint(&Serial);
      return false;
    } else {
      // This might happen if the disk is full, though sync is more likely to catch it
      Serial.println("Warning: Write mismatch, disk might be full?");
      return false;  // Treat mismatch as failure
    }
  }
  return true;  // Successfully buffered (or seemed to)
}


/**
 * @brief Flushes the write buffer of the open log file to the SD card.
 */
bool SOAR_SD_CARD::syncLogFile() {
  if (!_logFileOpen) {
    // Serial.println("Warning: Tried to sync closed log file."); // Optional warning
    return false;  // Or true? Debatable if sync on closed file is success/failure
  }

  // sync() pushes data to the card and updates metadata. THIS IS SLOW.
  if (_logFile.sync()) {
    return true;  // Sync successful
  } else {
    Serial.println("Log file sync failed!");
    SD.errorPrint(&Serial);
    return false;  // Sync failed
  }
}

/**
 * @brief Closes the currently open log file.
 */
void SOAR_SD_CARD::closeLogFile() {
  if (_logFileOpen) {
    _logFile.close();  // Close automatically syncs remaining data
    _logFileOpen = false;
    Serial.println("Log file closed.");
  } else {
    // Serial.println("Warning: Tried to close log file that was not open.");
  }
}

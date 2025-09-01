#include <XBeeArduino.h>
#include <xbee_api_frames.h>

#define CS_PIN 10 // Define your CS pin

XBee xbee;

// Initialize the htable with function pointers for SPI operations
XBeeHTable htable = {
    .PortSpiRead = PortSpiRead,
    .PortSpiWrite = PortSpiWrite,
    .PortSpiInit = PortSpiInit,
    // Initialize other function pointers as needed
};

void setup() {
  Serial.begin(115200);
  xbee.htable = &htable; // Assign the htable to the XBee instance
  xbee.useSpi = true; // Use SPI communication
  if (XBeeInit(&xbee, 1000000, CS_PIN)) { // Initialize with SPI speed 1MHz and CS pin
    Serial.println("XBee initialized with SPI.");
  }
  else {
    Serial.println("Failed to initialize XBee.");
  }
}

void loop() {
  // Your code to communicate with XBee using SPI
}
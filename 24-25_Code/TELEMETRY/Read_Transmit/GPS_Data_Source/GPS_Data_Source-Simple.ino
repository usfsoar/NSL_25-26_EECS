/*
 * GPS_Data_Source-Simple.ino
 * 
 * This program collects GPS data from an ESP32 device using the SOAR_GPS class. 
 * It initializes a GPS module connected to specific RX and TX pins and retrieves 
 * NMEA sentences from the GPS. The program then checks if the data is ready to be 
 * processed or if any parsing errors occurred.
 * 
 * Libraries Used:
 * - SOAR_GPS.h: Custom class for handling GPS communication and parsing.
 * 
 * Hardware Setup:
 * - RX_PIN: The RX pin on the ESP32 where the GPS TX is connected.
 * - TX_PIN: The TX pin on the ESP32 where the GPS RX is connected.
 * 
 * Functions:
 * - setup(): Initializes the Serial for debugging and sets up the GPS module.
 * - loop(): Retrieves and parses GPS NMEA sentences, printing results to Serial.
 * 
 * Created by Kilroy
 */

#include <SOAR_GPS.h>

#define RX_PIN 16  // Define your RX pin
#define TX_PIN 17  // Define your TX pin
#define SERIAL_BUS 1 // ESP32 Serial bus (e.g., Serial1)

SOAR_GPS gps(SERIAL_BUS, RX_PIN, TX_PIN); // Create SOAR_GPS object

void setup() {
  Serial.begin(115200); // Initialize Serial for debugging
  gps.setup(); // Initialize GPS
  Serial.println("GPS setup complete.");
}

void loop() {
  char nmea[100];       // Array to hold NMEA sentence
  bool ready = false;   // Flag for data ready
  bool failed = false;  // Flag for data parse failure

  gps.GET_NMEA(nmea, &ready, &failed); // Attempt to get NMEA data

  if (ready) {
    Serial.print("NMEA data: ");
    Serial.println(nmea); // Print the NMEA data to Serial
  } else if (failed) {
    Serial.println("Failed to parse GPS data.");
  }
}


#include <EEPROM.h>
#include "SOAR_IMU.h"
#include "SOAR_BMP.h"
#include "SOAR_BME.h"
#include "SOAR_PAYLOAD_SERVO.h"
#include "SOAR_SPH.h"
#include "SOAR_SD_CARD.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <cmath>  // Include for isnan, isinf if needed, and math functions

#include <vector>
#include <esp32-hal-ledc.h>
#include <esp32-hal-gpio.h>
#include <DRA818.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <HardwareSerial.h>
#include <cmath>  // For sin() function
#include <map>

#define PTTPIN 8               //Mar 13
#define DRA818_SERIAL Serial1  // Define the serial interface for the DRA818V
#define DRA818_PD 7
#define DRA818_PL 45

// DRA818V configuration parameters
#define DRA818_TYPE DRA818_VHF  // Module type: VHF
#define RX_FREQUENCY 146.000    // Receive frequency in MHz
#define TX_FREQUENCY 146.000    // Transmit frequency in MHz
#define SQUELCH_LEVEL 4         // Squelch level (0-8)
#define VOLUME_LEVEL 8          // Volume level (0-8)
#define CTCSS_RX 0              // CTCSS receive code (0 if not used)
#define CTCSS_TX 0              // CTCSS transmit code (0 if not used)
#define BANDWIDTH DRA818_12K5   // Bandwidth: 12.5 kHz
#define PREEMPHASIS true        // Pre-emphasis enabled
#define HIGH_PASS true          // High-pass filter enabled
#define LOW_PASS true           // Low-pass filter enabled
// --- Configuration ---

// --- NEW: Control Pin Definitions ---
const int DAC_LDAC_PIN = 9;  // Connected to LDAC
const int DAC_A0_PIN = 10;   // Connected to A0 (Address Select)

// --- I2C Address ---
// Set based on how you drive DAC_A0_PIN
// 0x4C if A0 is LOW, 0x4D if A0 is HIGH
const int DAC_I2C_ADDR = 0x4C;

// AD5693 Command Byte (VERIFY FROM DATASHEET!)
const uint8_t DAC_CMD_WRITE_UPDATE = 0x30;  // Assumed command

std::map<char, std::string> morseMap = {
  // Letters
  { 'A', ".-" },
  { 'B', "-..." },
  { 'C', "-.-." },
  { 'D', "-.." },
  { 'E', "." },
  { 'F', "..-." },
  { 'G', "--." },
  { 'H', "...." },
  { 'I', ".." },
  { 'J', ".---" },
  { 'K', "-.-" },
  { 'L', ".-.." },
  { 'M', "--" },
  { 'N', "-." },
  { 'O', "---" },
  { 'P', ".--." },
  { 'Q', "--.-" },
  { 'R', ".-." },
  { 'S', "..." },
  { 'T', "-" },
  { 'U', "..-" },
  { 'V', "...-" },
  { 'W', ".--" },
  { 'X', "-..-" },
  { 'Y', "-.--" },
  { 'Z', "--.." },

  // Digits
  { '0', "-----" },
  { '1', ".----" },
  { '2', "..---" },
  { '3', "...--" },
  { '4', "....-" },
  { '5', "....." },
  { '6', "-...." },
  { '7', "--..." },
  { '8', "---.." },
  { '9', "----." },

  // Punctuation / special characters
  { '.', ".-.-.-" },
  { ',', "--..--" },
  { '?', "..--.." },
  { '\'', ".----." },
  { '!', "-.-.--" },
  { '/', "-..-." },
  { '(', "-.--." },
  { ')', "-.--.-" },
  { '&', ".-..." },
  { ':', "---..." },
  { ';', "-.-.-." },
  { '=', "-...-" },
  { '+', ".-.-." },
  { '-', "-....-" },
  { '_', "..--.-" },
  { '\"', ".-..-." },
  { '$', "...-..-" },
  { '@', ".--.-." }
};

// Define I2C Pins
const int I2C_SDA_PIN = 1;
const int I2C_SCL_PIN = 2;

// Define I2S Pins
const int I2S_WS_PIN = 13;   // LRC/WS
const int I2S_SD_PIN = 12;   // DOUT/SD
const int I2S_SCK_PIN = 11;  // BCLK/SCK

// Define SD Card SPI Pins
const int SD_CS_PIN = 42;
const int SD_CLK_PIN = 41;
const int SD_MOSI_PIN = 6;
const int SD_MISO_PIN = 5;

// Object Instantiations
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);            // Default I2C address
SOAR_IMU soar_imu;                                                      // Uses default Wire (initialized below)
SOAR_BAROMETER soar_barometer;                                          // Uses default Wire
SPH0645_Mic soar_sph(I2S_WS_PIN, I2S_SD_PIN, I2S_SCK_PIN);              // Use defined I2S pins
SOAR_PAYLOAD_SERVO p_serv;                                              // Uses default Wire
SOAR_SD_CARD sd_card(SD_CS_PIN, SD_CLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);  // Use modified constructor
SOAR_BME soar_bme(I2C_SDA_PIN, I2C_SCL_PIN);                            // Use defined I2C pins
//DRA818* dra;

// ------ Refactored Function Prototypes ------
void readAllSensors(float& currentAltitude, float& currentTemp,
                    float& currentPressure, float& bmpPressure, float& currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float& currentGForce, float& currentVelocityMag);
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag);
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag);
void updateMaxSustainedRPM(float currentRpm);
void handleLandingSequence(float finalVelocityReading, const float finalGravityReading[3]);
bool startNewLogFile(const char* filename);
void resetCurrentFlightData();
void resetServos();
// void captureGroundLevel();
void initializeEMAs();
void restoreLastFlightData();
void resetAllFlightData();    // EEPROM reset
void displayFlightData();     // Display summary of *just completed* flight
void saveDataToEEPROM();      // Save summary to EEPROM
void displayAllFlightData();  // Display all summaries from EEPROM
void logFlightDataToSD(unsigned long timestamp, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure);  // Modified log function
void saveFlightSummaryToSD();                   // Save summary to SD Card
void playMorse(std::string message, int time_unit);
void generateTone(int duration);
bool setDacOutput(uint16_t value);

// EEPROM address definitions
#define EEPROM_SIZE 256
#define MAX_FLIGHTS 4        // Maximum number of flights to store
#define FLIGHT_DATA_SIZE 15  // Number of float/ulong values per flight
#define ADDR_FLIGHT_BASE 0   // Base address for flight data
#define ADDR_FLIGHT_COUNTER 248

// Offsets for flight data variables (relative to base address for a flight)
#define OFFSET_LANDING_TEMP 0
#define OFFSET_APOGEE 1
#define OFFSET_LANDING_TIME 2  // unsigned long
#define OFFSET_LANDING_PITCH 3
#define OFFSET_LANDING_ROLL 4
#define OFFSET_OPENED_DOOR 5  // int
#define OFFSET_MAX_VELOCITY 6
#define OFFSET_LANDING_VELOCITY 7
#define OFFSET_MAX_G_FORCE 8
#define OFFSET_MIN_TEMP 9
#define OFFSET_MAX_TEMP 10
#define OFFSET_MAX_RPM 11
#define OFFSET_MIN_PRESSURE 12
#define OFFSET_MAX_PRESSURE 13
#define OFFSET_MAX_DECIBELS 14

//led
#define LED_PIN 21
#define NUM_LEDS 1
Adafruit_NeoPixel onboardLed = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Global variables
int currentFlight = 0;     // Index (0 to MAX_FLIGHTS-1) for the *next* flight slot in EEPROM
int sd_flight_number = 0;  // Counter for SD card filenames (flight_0.csv, flight_1.csv, ...)
char current_filename[40];

// Flight state definitions
enum FlightState {
  READY,
  LAUNCHED,
  DESCENDING,
  LANDED
};
FlightState currentState = READY;

// Constants Threshold
const float LAUNCH_ACCEL_THRESHOLD = 1.2;   // G force threshold to detect launch
const float LANDING_ACCEL_THRESHOLD = 1.1;  // G force threshold difference from 1G to detect landing
const float LANDING_VEL_THRESHOLD = 0.5;    // m/s threshold to confirm landing
const int STABLE_READINGS_FOR_LAUNCH = 3;
const int STABLE_READINGS_FOR_DESCENT = 3;
const int STABLE_READINGS_FOR_LANDING = 10;    // Number of stable readings to confirm landing
const float DESCENT_ALTITUDE_THRESHOLD = 0.5;  // Meters fallen below apogee to confirm descent

// --- Smoothing Factors (Alpha for EMA) ---
const float ALPHA_GFORCE = 0.8;    // EMA alpha for G-Force
const float ALPHA_ZACCEL = 0.8;    // EMA alpha for Z-axis linear acceleration
const float ALPHA_ALTITUDE = 0.5;  // EMA alpha for Altitude
const float ALPHA_VELOCITY = 0.8;  // EMA alpha for Velocity magnitude

// --- EMA State Variables ---
float emaGForce = 1.0;  // Initialize to approx. 1G at rest
float emaZAccel = 0.0;
float emaAltitude = 0.0;
float emaVelocity = 0.0;
bool emaInitialized = false;  // Flag to track if EMAs have been initialized

// Flight data variables (for the current/ongoing flight)
//infinty
float maxGForce = 0.0;
float landingVelocity = 0.0;  // Final velocity recorded at landing detection
unsigned long flightStartTime = 0;
unsigned long flightTime = 0;    // Total flight duration in ms
float minTemperature = 1000.0;   // Initialize to high value
float maxTemperature = -1000.0;  // Initialize to low value
float minPressure = 200000.0;    // Initialize to high value
float maxPressure = 0.0;         // Initialize to low value
float maxVelocity = 0.0;         // Max velocity magnitude recorded
float maxDecibels = 0;
float maxRPM = 0.0;                        // Absolute peak RPM recorded
float apogee = 0.0;                        // Highest relative altitude recorded
float currentFlightMaxSustainedRpm = 0.0;  // Max sustained RPM for the *completed* flight
int stableReadingLaunch = 0;
int stableReadingDescent = 0;
int stableReadingLand = 0;
float groundLevel = 0.0;     // BMP ground level altitude
float bmeGroundLevel = 0.0;  // BME ground level altitude
bool descentStarted = false;

int door = -1;
String summary = "";
String broadcast_string = "KQ4FYU String Initialized";

// Sustained RPM Calculation Variables
const unsigned long RPM_SUSTAINED_INTERVAL_MS = 100;  // Not directly used in window calc, but for context
const int RPM_WINDOW_SIZE = 120;                      // Buffer size for ~1 second of data + buffer (adjust based on loop rate)
float rpmWindow[RPM_WINDOW_SIZE];                     // Circular buffer for recent RPM readings
int rpmWindowIndex = 0;                               // Current index in the circular buffer
int rpmSampleCount = 0;                               // How many samples currently in buffer (up to RPM_WINDOW_SIZE)
float maxSustainedRpm = 0.0;                          // The maximum sustained RPM calculated *so far* during flight


// SD Card Sync Interval
const unsigned long SD_SYNC_INTERVAL_MS = 1000;

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  delay(3000);  // Wait for serial monitor connection

  Serial.println("Rocket Flight Data Recorder - Waveshare ESP32-S3-Zero");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C Initialized on SDA=1, SCL=2");

  // Initialize Servo Controller
  p_serv.initialize(pwm);
  Serial.println("Servo Controller Initialized");

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    // while (1); // Halt on EEPROM failure
  } else {
    Serial.println("EEPROM initialized successfully.");
    EEPROM.get(ADDR_FLIGHT_COUNTER, currentFlight);           // Read the index for the *next* flight
    if (currentFlight >= MAX_FLIGHTS || currentFlight < 0) {  // Basic validation
      Serial.print("Invalid flight counter value found in EEPROM (");
      Serial.print(currentFlight);
      Serial.println("). Resetting to 0.");
      currentFlight = 0;
      EEPROM.put(ADDR_FLIGHT_COUNTER, currentFlight);
      EEPROM.commit();  // Save the reset counter
    }
    Serial.print("Next EEPROM Flight Slot: ");
    Serial.println(currentFlight);
    restoreLastFlightData();
    displayAllFlightData();  // Show previously stored flights
  }

  // Initialize IMU
  soar_imu.BNO_SETUP();

  // Initialize MIC
  soar_sph.begin();
  Serial.println("SPH0645 Mic Initialized");

  // --- Initialize NeoPixel ---
  onboardLed.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  onboardLed.setBrightness(50);  // Set BRIGHTNESS (optional, 0-255) - Start low!
  onboardLed.clear();            // Set all pixel colors to 'off'
  onboardLed.show();             // Send the clear command to the pixel
  Serial.println("Onboard NeoPixel Initialized.");

  // Initialize BMP
  soar_barometer.Initialize();

  // Initialize BME
  soar_bme.initialize();

  if (currentState == READY) {
    onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 50));
    sd_card.begin();
    sd_card.createDir("/flight_data");
    while (true) {
      sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
      if (!sd_card.exists(current_filename)) break;
      sd_flight_number++;
      if (sd_flight_number > 9999) {
        sprintf(current_filename, "/flight_data/flight_err.csv");
        break;
      }
    }
    // Initialize ground level and EMAs
    Serial.println("Getting initial sensor readings for ground level and EMA init...");
    // captureGroundLevel();
  }

  initializeEMAs();

  // Initialize Sustained RPM tracking variables
  for (int i = 0; i < RPM_WINDOW_SIZE; ++i) {
    rpmWindow[i] = 0.0;  // Clear the buffer
  }
  rpmWindowIndex = 0;
  rpmSampleCount = 0;
  maxSustainedRpm = 0.0;  // Reset max sustained value at power-on

  // Serial.print("Ground Level BMP Alt: ");
  // Serial.println(groundLevel);
  // Serial.print("Ground Level BME Alt: ");
  // Serial.println(bmeGroundLevel);
  Serial.print("Initial EMA Values -> GForce: ");
  Serial.print(emaGForce, 2);
  Serial.print(" | ZAccel: ");
  Serial.print(emaZAccel, 2);
  Serial.print(" | Altitude: ");
  Serial.print(emaAltitude, 2);
  Serial.print(" | Velocity: ");
  Serial.println(emaVelocity, 2);

  //radio
  pinMode(PTTPIN, OUTPUT);
  digitalWrite(PTTPIN, HIGH);

  pinMode(DRA818_PD, OUTPUT);
  digitalWrite(DRA818_PD, HIGH);
  pinMode(DRA818_PL, OUTPUT);
  digitalWrite(DRA818_PL, LOW);

  // Initialize the serial communication with the DRA818V

  /*
  DRA818_SERIAL.begin(9600, SERIAL_8N1, 43, 44);  //44, 43); // RX on GPIO16, TX on GPIO17 (rate, mode, rx,tx)
  dra = DRA818::configure(
    &DRA818_SERIAL,
    DRA818_TYPE,
    RX_FREQUENCY,
    TX_FREQUENCY,
    SQUELCH_LEVEL,
    VOLUME_LEVEL,
    CTCSS_RX,
    CTCSS_TX,
    BANDWIDTH,
    PREEMPHASIS,
    HIGH_PASS,
    LOW_PASS);

  // Check if the module was configured successfully
  if (dra == nullptr) {
    Serial.println("Failed to configure DRA818V module.");
    //while (true);
  } else {
    Serial.println("DRA818V module configured successfully.");
  }
  */

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
  //while (!Serial); // Wait for Serial Monitor to open

  Serial.println("AD5693 DAC Test Sketch (with LDAC/A0 control)");

  // --- Configure Control Pins ---
  pinMode(DAC_A0_PIN, OUTPUT);
  digitalWrite(DAC_A0_PIN, LOW);  // Set A0 LOW for I2C Address 0x4C
  Serial.println("Set A0 Pin (GPIO " + String(DAC_A0_PIN) + ") LOW for Address 0x4C");

  pinMode(DAC_LDAC_PIN, OUTPUT);
  digitalWrite(DAC_LDAC_PIN, LOW);  // Hold LDAC LOW for immediate output updates
  Serial.println("Set LDAC Pin (GPIO " + String(DAC_LDAC_PIN) + ") LOW");
  // --- End Control Pin Config ---

  delay(10);  // Short delay after setting pins

  // Initialize I2C
  Serial.println("I2C Initialized.");

  // Test I2C Connection
  Wire.beginTransmission(DAC_I2C_ADDR);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.print("DAC found at address 0x");
    Serial.println(DAC_I2C_ADDR, HEX);
  } else {
    Serial.print("DAC not found at address 0x");
    Serial.println(DAC_I2C_ADDR, HEX);
    Serial.println("Check wiring, A0 pin state, and I2C address. Halting.");
    //while (1); // Stop execution
  }

  // Set initial output to 0V
  Serial.println("Setting initial output to 0V...");
  setDacOutput(0);

  resetServos();  // Set servos to initial position

  Serial.println("Setup Complete. Waiting for launch...");
}

// --- Main Loop ---
void loop() {
  static uint32_t lastLogTime = 0;
  static uint32_t EELastLogTime = 0;
  static uint32_t lastSyncTime = 0;

  // Check for serial commands
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'D' || command == 'd') {
      Serial.println("Displaying All Stored Flight Data:");
      displayAllFlightData();
    } else if (command == 'R' || command == 'r') {
      resetAllFlightData();  // Resets EEPROM and SD counter
      // Optional: Reset current flight state variables too?
      // resetCurrentFlightData(); // If you want 'R' to fully reset the current state too
      // currentState = READY;
    }
  }

  // --- Sensor Readings ---
  float currentAltitude, currentTemp, currentPressure, currentDecibels, bmpPressure;
  float acceleration[3], linearAccel[3], gravity[3], velocity[1], rpmReading[1];
  float currentGForce, currentVelocityMag;
  //yayayay

  readAllSensors(currentAltitude, currentTemp,
                 currentPressure, bmpPressure, currentDecibels,
                 acceleration, linearAccel, gravity, velocity, rpmReading,
                 currentGForce, currentVelocityMag);

  // --- Update EMAs ---
  if (emaInitialized) {
    updateEMAs(currentGForce, linearAccel[2], currentAltitude, currentVelocityMag);
  } else {
    Serial.println("Warning: EMAs not initialized!");
    delay(100);  // Avoid spamming
    return;      // Skip the rest of the loop until initialized
  }

  // --- Data Updates & Logging (Common to Launched & Descending) ---
  if (currentState == LAUNCHED || currentState == DESCENDING) {
    // Update Min/Max values
    updateFlightExtremes(currentGForce, rpmReading[0], currentAltitude, currentTemp, currentPressure, currentDecibels, currentVelocityMag);

    // Update Max Sustained RPM (uses its own internal buffer/logic)
    updateMaxSustainedRPM(rpmReading[0]);

    // // Log data periodically SD CARD
    uint32_t now = millis();
    if (now - lastLogTime >= 100) {  // delay log to approx every 100ms (adjust rate as needed)
      logFlightDataToSD(now - flightStartTime, currentAltitude, currentTemp,
                        rpmReading[0], linearAccel[2], currentGForce, velocity[0], currentDecibels,
                        currentPressure);
      lastLogTime = now;
    }

    // Log data EEPROM
    if (now - EELastLogTime >= 2000) {  // delay log to approx every 100ms (adjust rate as needed)
      saveDataToEEPROM();
      EELastLogTime = now;
    }

    // Periodically sync the SD card buffer to physical storage
    if (now - lastSyncTime >= SD_SYNC_INTERVAL_MS) {
      if (!sd_card.syncLogFile()) {
        Serial.println("Error syncing log file!");
      }
      lastSyncTime = now;  // Update sync time even if sync failed to prevent spamming
    }
  }

  // --- Flight State Machine ---
  switch (currentState) {
    case READY:
      resetServos();  // Ensure servos are reset while waiting
      Serial.print("BME Pressure: ");
      Serial.println(currentPressure);
      Serial.print("BMP: ");
      Serial.println(bmpPressure);
      Serial.print("gforce: ");
      Serial.println(emaGForce);
      Serial.print("altitude: ");
      Serial.println(emaAltitude);
      Serial.print("velocity: ");
      Serial.println(emaVelocity);

      // Check for launch conditions using EMA values for stability
      // Condition: High G-force OR significant altitude gain (e.g., manual throw)
      //TODO: altitude condition for launch needs to be higher.
      if (emaGForce > LAUNCH_ACCEL_THRESHOLD || emaAltitude > 10.0) {
        stableReadingLaunch++;
      } else {
        stableReadingLaunch = 0;
      }

      if (stableReadingLaunch >= STABLE_READINGS_FOR_LAUNCH) {
        Serial.println("****** LAUNCH DETECTED! ******");
        currentState = LAUNCHED;
        flightStartTime = millis();

        // Reset data for the new flight
        resetCurrentFlightData();
        // Re-capture ground level precisely at launch detection for accuracy
        // captureGroundLevel(); // Re-capture ground level? Maybe not if it was stable. Use setup's values.
        //led red
        onboardLed.setPixelColor(0, onboardLed.Color(50, 0, 0));
        // Start the new log file
        sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
        if (startNewLogFile(current_filename)) {
          lastSyncTime = millis();  // Reset sync timer after successful header sync
        } else {
          // Handle error - maybe revert state? Log error?
          Serial.println("CRITICAL: Failed to start log file. Aborting flight tracking.");
          currentState = LANDED;  // Go to landed state to prevent further action without logs
        }
      }
      break;

    case LAUNCHED:
      resetServos();  // Keep servos reset during ascent
      Serial.print("gforce: ");
      Serial.println(emaGForce);
      Serial.print("altitude: ");
      Serial.println(emaAltitude);
      Serial.print("velocity: ");
      Serial.println(emaVelocity);

      // Check for apogee (using smoothed altitude)
      // if (emaAltitude > apogee) {
      //   apogee = emaAltitude;  // Update apogee while climbing
      // }

      // Check if altitude has decreased sufficiently below apogee
      // Add check to ensure a minimum apogee was reached before checking descent
      if (!descentStarted && apogee > 6.2 && emaAltitude < (apogee - DESCENT_ALTITUDE_THRESHOLD)) {
        stableReadingDescent++;
      } else {
        stableReadingDescent = 0;
      }

      if (stableReadingDescent >= STABLE_READINGS_FOR_DESCENT) {
        currentState = DESCENDING;
        descentStarted = true;  // Mark descent started
        onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 0));
        Serial.print("****** APOGEE REACHED (EMA): ");
        Serial.print(apogee, 2);
        Serial.println(" meters ******");
      }

      // Timeout failsafe - If still in LAUNCHED state after 5 minutes, assume landing
      if (((millis() - flightStartTime) / 1000.0) > 300.0) {
        Serial.println("****** FLIGHT TIMEOUT - ASSUMING LANDING ******");
        handleLandingSequence(velocity[0], gravity);  // Pass final sensor readings
        currentState = LANDED;                        // Ensure state is set
      }
      break;

    case DESCENDING:
      resetServos();  // Keep servos reset during descent
      Serial.print("gforce: ");
      Serial.println(emaGForce);
      Serial.print("altitude: ");
      Serial.println(emaAltitude);
      Serial.print("velocity: ");
      Serial.println(emaVelocity);

      // Check for landing conditions (low velocity, G-force near 1, low altitude) using EMA values
      if (emaVelocity < LANDING_VEL_THRESHOLD && abs(emaGForce - 1.0) < LANDING_ACCEL_THRESHOLD && emaAltitude < 30.0) {
        stableReadingLand++;
      } else {
        stableReadingLand = 0;  // Reset count if conditions aren't met
      }

      // Confirm landing after N stable readings or timeout
      if (stableReadingLand >= STABLE_READINGS_FOR_LANDING || ((millis() - flightStartTime) / 1000.0) > 300.0) {
        if (stableReadingLand >= STABLE_READINGS_FOR_LANDING) {
          Serial.println("****** LANDING DETECTED! ******");
        } else {
          Serial.println("****** FLIGHT TIMEOUT - ASSUMING LANDING ******");
        }
        handleLandingSequence(velocity[0], gravity);  // Pass final sensor readings
        currentState = LANDED;                        // Ensure state is set
      }
      break;

    case LANDED:
      Serial.print("gforce: ");
      Serial.println(emaGForce);
      Serial.print("altitude: ");
      Serial.println(emaAltitude);
      Serial.print("velocity: ");
      Serial.println(emaVelocity);

      bool landed_survivable = calculateSurvivability();
      float landed_current_temp = soar_bme.getTemperature();
      //KQ4FYU Temperature = 93.38 F, Apogee = 3752 ft, Battery power = 9.47 V, Maximum velocity = 470 ft/s, Time of Landing = 92.12 s, Landing Velocity = 0.8 m/s, Max G-force = 1.92 g, Survivable = True
      //std::string broadcast_string = "KQ4FYU" + "Temperature = ";
    broadcast_string = "C KQ4FYU Temp = " + String(landed_current_temp, 2) + " F, " +
                            "Apogee = " + String(apogee, 2) + " ft, " +
                            /*"Battery power = " + String(battery_power) + " V, " +*/    //No battery check
                            "Max velocity = " + String(maxVelocity, 2) + " ft/s, " +
                            "Flight Length = " + String(flightTime) + " s, " +         //No EST time of landing
                            "Landing Velocity = " + String(landingVelocity, 2) + " m/s, " +
                            "Max G-force = " + String(maxGForce, 2) + " g, " +
                            "Survivable = " + (landed_survivable ? "True" : "False");
      std::string std_broadcast_string = broadcast_string.c_str();
      Serial.println(std_broadcast_string.c_str());
      playMorse(std_broadcast_string, 50);        //Play fast
      //playMorse(std_broadcast_string, 500);       //Play slow

      // Rocket is on the ground. Wait for reset command 'R'/'r' via Serial.
      // Optional: Re-enable automatic re-launch detection if desired (removed for safety unless explicitly needed)
      // If auto-relaunch is needed, call resetCurrentFlightData(), captureGroundLevel(), startNewLogFile(), currentState = LAUNCHED;
      // Example (Use with caution):
      /*
      if (emaGForce > LAUNCH_ACCEL_THRESHOLD) { // Check for re-launch
         stableReadingLaunch++;
      } else {
         stableReadingLaunch = 0;
      }
      if (stableReadingLaunch >= STABLE_READINGS_FOR_LAUNCH) {
         Serial.println("WARNING: New launch detected automatically after landing!");
         resetCurrentFlightData(); // Reset flight variables
         captureGroundLevel(); // Get new ground level
         sd_flight_number++; // Increment SD number for the *new* flight
         sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
         if (startNewLogFile(current_filename)) {
             flightStartTime = millis();
             currentState = LAUNCHED; // Set state to LAUNCHED
             lastSyncTime = millis();
         } else {
             Serial.println("CRITICAL: Failed to start log file for auto-relaunch.");
             // Remain in LANDED state
         }
      }
      */
      break;
  }  // End switch(currentState)
  Serial.print("Time: ");
  Serial.println(millis());
  // Serial.print("BME Pressure: ");
  // Serial.println(currentPressureBme);
  // Serial.print("BMP: ");
  // Serial.println(currentPressureBmp);
  //playMorse("KQ4FYU Hello, it works!", 50);
  //playMorse("we landed radio works", 50);
  //playMorse("KQ4FYU", 50);
  //Serial.print("Radio sent");
  onboardLed.show();
}  // End loop()

// ------ Refactored Function Implementations ------

/**
 * @brief Reads all relevant sensors and calculates derived values.
 * @param currentAltitude Output: Relative BMP altitude (m)
 * @param bmeAltitude Output: Relative BME altitude (m)
 * @param currentTempBmp Output: BMP temperature (C)
 * @param currentTempBme Output: BME temperature (C)
 * @param currentPressureBmp Output: BMP pressure (Pa)
 * @param currentPressureBme Output: BME pressure (Pa)
 * @param currentDecibels Output: SPH sound level (dB)
 * @param acceleration Output: Raw acceleration [x, y, z] (m/s^2)
 * @param linearAccel Output: Linear acceleration [x, y, z] (m/s^2)
 * @param gravity Output: Gravity vector [x, y, z] (m/s^2)
 * @param velocity Output: IMU velocity estimate [magnitude or component] (m/s)
 * @param rpmReading Output: IMU RPM estimate [value]
 * @param currentGForce Output: Calculated G-force magnitude
 * @param currentVelocityMag Output: Calculated velocity magnitude (absolute value)
 */
void readAllSensors(float& currentAltitude, float& currentTemp,
                    float& currentPressure, float& bmpPressure, float& currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float& currentGForce, float& currentVelocityMag) {
  // Read IMU
  soar_imu.GET_ACCELERATION(acceleration);
  soar_imu.GET_LINEARACCEL(linearAccel);
  soar_imu.GET_VELOCITY(velocity);  // Gets IMU's calculated velocity component
  soar_imu.GET_GRAVITY(gravity);
  soar_imu.GET_RPM(rpmReading);  // Reads into the provided 'rpmReading' array

  // Read Barometers/Temp
  currentAltitude = soar_barometer.get_altitude();  // Relative altitude
  if (!soar_barometer.isReady() || isnan(currentAltitude)) {
    currentAltitude = soar_bme.getAltitude();
  }
  //bmeAltitude = soar_bme.getAltitude();             // Relative altitude

  currentTemp = soar_bme.getTemperature();
  if (!soar_bme.isReady() || isnan(currentTemp)) {
    currentTemp = soar_barometer.get_temperature();
  }

  currentPressure = soar_bme.getPressure();
  if (!soar_bme.isReady() || isnan(currentPressure)) {
    currentPressure = soar_barometer.get_pressure();
  }
  bmpPressure = soar_barometer.get_pressure();
  // currentTempBmp = soar_barometer.get_temperature();
  // currentTempBme = soar_bme.getTemperature();
  // currentPressureBmp = soar_barometer.get_pressure();
  // currentPressureBme = soar_bme.getPressure();

  // Read Microphone
  currentDecibels = soar_sph.getDecibels();

  // Calculate derived values
  currentGForce = sqrt(acceleration[0] * acceleration[0] + acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 9.81;
  currentVelocityMag = abs(velocity[0]);  // Assuming velocity[0] holds the primary magnitude/component

  // --- Safety Check for NaN/Inf ---
  // It's good practice to check sensor readings, especially after calculations
  if (isnan(currentGForce) || isinf(currentGForce)) {
    Serial.println("Warning: NaN/Inf detected in GForce calculation!");
    currentGForce = 1.0;  // Provide a default safe value
  }
  if (isnan(currentAltitude) || isinf(currentAltitude)) {
    Serial.println("Warning: NaN/Inf detected in BMP Altitude!");
    currentAltitude = 0.0;
  }
  // Add similar checks for other critical values if needed
}

/**
 * @brief Updates the Exponential Moving Averages (EMAs) for key flight parameters.
 * Requires emaInitialized == true.
 * @param currentGForce Latest raw G-Force reading.
 * @param currentZAccel Latest raw Z-axis linear acceleration reading.
 * @param currentAltitude Latest raw relative altitude reading.
 * @param currentVelocityMag Latest raw velocity magnitude reading.
 */
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag) {
  emaGForce = (currentGForce * ALPHA_GFORCE) + (emaGForce * (1.0 - ALPHA_GFORCE));
  emaZAccel = (currentZAccel * ALPHA_ZACCEL) + (emaZAccel * (1.0 - ALPHA_ZACCEL));
  emaAltitude = (currentAltitude * ALPHA_ALTITUDE) + (emaAltitude * (1.0 - ALPHA_ALTITUDE));
  emaVelocity = (currentVelocityMag * ALPHA_VELOCITY) + (emaVelocity * (1.0 - ALPHA_VELOCITY));
}

/**
 * @brief Updates the min/max values recorded during flight based on current readings.
 * @param currentGForce Latest G-force reading.
 * @param currentRpm Latest RPM reading.
 * @param currentTempBmp Latest BMP temperature reading.
 * @param currentPressureBmp Latest BMP pressure reading.
 * @param currentDecibels Latest sound level reading.
 * @param imuVelocityMag Latest IMU velocity magnitude reading.
 */
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag) {
  // Update Max G-Force
  if (currentGForce > maxGForce) maxGForce = currentGForce;

  // Update Max Absolute RPM
  if (currentRpm > maxRPM) maxRPM = currentRpm;

  if (currentAltitude > apogee) apogee = currentAltitude;

  // Update Temp Min/Max (using BMP temp)
  if (currentTemp < minTemperature) minTemperature = currentTemp;
  if (currentTemp > maxTemperature) maxTemperature = currentTemp;

  // Update Pressure Min/Max (using BMP pressure)
  if (currentPressure < minPressure) minPressure = currentPressure;
  if (currentPressure > maxPressure) maxPressure = currentPressure;

  // Update Max Decibels
  if (currentDecibels > maxDecibels) maxDecibels = currentDecibels;

  // Update Max Velocity (using magnitude of IMU velocity reading)
  if (imuVelocityMag > maxVelocity) maxVelocity = imuVelocityMag;
}

/**
 * @brief Updates the calculation for maximum sustained RPM over a defined window.
 * Modifies global variables: rpmWindow, rpmWindowIndex, rpmSampleCount, maxSustainedRpm.
 * @param currentRpm The latest RPM reading.
 */
void updateMaxSustainedRPM(float currentRpm) {
  // Add current RPM to the circular buffer
  rpmWindow[rpmWindowIndex] = currentRpm;
  rpmWindowIndex = (rpmWindowIndex + 1) % RPM_WINDOW_SIZE;
  // Increment sample count until buffer is full
  if (rpmSampleCount < RPM_WINDOW_SIZE) {
    rpmSampleCount++;
  }

  // Once the buffer is full (we have data for the full interval)
  if (rpmSampleCount >= RPM_WINDOW_SIZE) {
    // Find the minimum RPM within the current window
    float minRpmInWindow = rpmWindow[0];
    for (int i = 1; i < RPM_WINDOW_SIZE; i++) {
      if (rpmWindow[i] < minRpmInWindow) {
        minRpmInWindow = rpmWindow[i];
      }
    }

    // Update the overall maximum sustained RPM *during the flight*
    // if the current window's minimum is higher
    if (minRpmInWindow > maxSustainedRpm) {
      maxSustainedRpm = minRpmInWindow;
    }
  }
}


/**
 * @brief Executes the sequence of actions required upon landing detection.
 * Calculates flight time, saves data, displays summary, triggers servos.
 * @param finalVelocityReading The velocity reading captured just before landing detection.
 * @param finalGravityReading The gravity vector captured just before landing detection.
 */
void handleLandingSequence(float finalVelocityReading, float finalGravityReading[3]) {
  flightTime = (millis() - flightStartTime);       // Store flight time in milliseconds
  landingVelocity = finalVelocityReading;          // Record final IMU velocity reading
  currentFlightMaxSustainedRpm = maxSustainedRpm;  // Store the max sustained RPM for this completed flight
  onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 50));
  delay(2000);

  // Trigger payload servo logic
  // Note: Ensure p_serv.servoLogic uses the passed gravity or fetches a fresh reading if needed.
  door = p_serv.servoLogic(pwm, finalGravityReading);

  delay(2000);

  Serial.println("Finalizing log file...");
  if (!sd_card.syncLogFile()) {  // Sync remaining buffered data
    Serial.println("Warning: Final sync failed before closing!");
  }
  sd_card.closeLogFile();  // Close the main log file

  // Save summary data (EEPROM and SD)
  saveDataToEEPROM();       // Saves summary to EEPROM, increments EEPROM flight counter
  saveFlightSummaryToSD();  // Saves summary text file to SD card

  // Display final flight data to Serial
  displayFlightData();

  //radio
  delay(1000);
  std::string transmit = summary.c_str();
  /*
  digitalWrite(PTTPIN, LOW);
  playMorse("KQ4FYU", 50);
  playMorse("we landed radio works", 50);
  playMorse(transmit, 50);
  playMorse("KQ4FYU", 50);
  digitalWrite(PTTPIN, HIGH);
  Serial.println("radio odne");
  */

  const char* message = transmit.c_str();

  // Prepare for next potential flight (increment SD flight number)
  // Done *after* saving summary which uses the *completed* flight number.
  sd_flight_number++;
}


/**
 * @brief Opens a new log file on the SD card, writes the header, and syncs.
 * @param filename The full path and filename for the log file.
 * @return True if the file was opened and header written successfully, False otherwise.
 */
bool startNewLogFile(const char* filename) {
  Serial.print("Attempting to start new log file: ");
  Serial.println(filename);

  if (sd_card.openLogFile(filename)) {
    String header = "Time(ms),Alt_BMP(m),Alt_BME(m),Temp_BMP(C),Temp_BME(C),RPM,Accel_Z(m/s^2),G-Force,Vel_IMU(m/s),dB,Press_BME(Pa),Press_BMP(Pa)\n";
    if (sd_card.logToFile(header.c_str())) {
      // Sync immediately after writing header to ensure it's saved
      if (!sd_card.syncLogFile()) {
        Serial.println("Error syncing header to SD card!");
        sd_card.closeLogFile();  // Close the file if sync failed
        return false;
      } else {
        Serial.println("Log file opened and header written successfully.");
        return true;
      }
    } else {
      Serial.println("Error writing header to SD card buffer!");
      sd_card.closeLogFile();  // Close file if header write failed
      return false;
    }
  } else {
    Serial.println("Error opening log file!");
    return false;
  }
}

/**
 * @brief Resets the global variables holding data for the current flight.
 * Called before starting a new flight measurement.
 */
void resetCurrentFlightData() {
  maxGForce = 0.0;
  landingVelocity = 0.0;  // Reset for next flight
  // flightStartTime is set at launch detection
  flightTime = 0;
  minTemperature = 1000.0;
  maxTemperature = -1000.0;
  minPressure = 200000.0;
  maxPressure = 0.0;
  maxVelocity = 0.0;
  maxDecibels = 0;
  maxRPM = 0.0;
  apogee = 0.0;
  stableReadingLaunch = 0;
  stableReadingDescent = 0;
  stableReadingLand = 0;
  descentStarted = false;
  // Reset Sustained RPM calculation state for the new flight
  for (int i = 0; i < RPM_WINDOW_SIZE; ++i) { rpmWindow[i] = 0.0; }
  rpmWindowIndex = 0;
  rpmSampleCount = 0;
  maxSustainedRpm = 0.0;               // Reset the running max sustained RPM calculation
  currentFlightMaxSustainedRpm = 0.0;  // Reset the value saved at the end of the flight

  // Note: Does NOT reset groundLevel, bmeGroundLevel, EMAs, sd_flight_number, currentFlight (EEPROM index)
  Serial.println("Current flight data variables reset.");
}

/**
 * @brief Sets all defined payload servos to their default (0 degree) position.
 */
void resetServos() {
  p_serv.setServoAngle(pwm, 8, 0);
  p_serv.setServoAngle(pwm, 12, 0);
  p_serv.setServoAngle(pwm, 0, 0);
  p_serv.setServoAngle(pwm, 4, 0);
}

/**
 * @brief Reads BMP and BME sensors to establish ground level altitude references.
 * Updates global variables groundLevel and bmeGroundLevel.
 */
// void captureGroundLevel() {
//   Serial.println("Capturing ground level altitude...");
//   // Allow some time for sensors to stabilize if needed, or take multiple readings

//   soar_barometer.get_altitude();
//   soar_bme.getAltitude();

//   delay(500);  // Short delay
//   float totalBmpAlt = 0;
//   float totalBmeAlt = 0;
//   const int readings = 5;

//   delay(2000);
//   float groundLevel = soar_barometer.get_altitude();
//   float bmeGroundLevel = soar_bme.getAltitude();

//   // for (int i = 0; i < readings; ++i) {
//   //   totalBmpAlt += soar_barometer.get_altitude();
//   //   totalBmeAlt += soar_bme.getAltitude();
//   //   delay(50);  // Small delay between readings
//   // }
//   // groundLevel = totalBmpAlt / readings;
//   // bmeGroundLevel = totalBmeAlt / readings;

//   Serial.print("BMP Ground Level Set: ");
//   Serial.println(groundLevel);
//   Serial.print("BME Ground Level Set: ");
//   Serial.println(bmeGroundLevel);
// }

/**
 * @brief Initializes the Exponential Moving Average (EMA) filters with initial sensor readings.
 * Reads sensors, calculates initial values, and sets the emaInitialized flag.
 */
void initializeEMAs() {
  Serial.println("Initializing EMAs...");
  // Get initial readings
  float initialAccel[3], initialLinAccel[3], initialVel[1];
  soar_imu.GET_ACCELERATION(initialAccel);
  soar_imu.GET_LINEARACCEL(initialLinAccel);
  soar_imu.GET_VELOCITY(initialVel);

  // Calculate initial G-Force
  float initialGForce = sqrt(pow(initialAccel[0], 2) + pow(initialAccel[1], 2) + pow(initialAccel[2], 2)) / 9.81;
  // Safety check for NaN/Inf, common at startup if sensor init is slow
  if (isnan(initialGForce) || isinf(initialGForce) || initialGForce < 0.1) {
    initialGForce = 1.0;  // Assume 1G if reading is invalid
    Serial.println("Warning: Invalid initial G-force reading, defaulting to 1.0");
  }


  // Initialize EMA variables with first valid readings
  // Note: Use relative altitude which is 0 at ground level.
  emaAltitude = 0.0;
  emaGForce = initialGForce;
  emaZAccel = initialLinAccel[2];    // Z-axis linear acceleration
  emaVelocity = abs(initialVel[0]);  // Use magnitude
  emaInitialized = true;             // Mark EMAs as initialized

  Serial.println("EMAs Initialized.");
}


// ------ Original Function Implementations (with minor adjustments) ------


/**
 * @brief Logs a single line of flight data to the SD card buffer.
 * Uses snprintf for safe formatting and sd_card.logToFile for buffered writing.
 */
void logFlightDataToSD(unsigned long timestamp, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure) {

  // --- Use snprintf for safer and potentially faster string formatting ---
  char dataBuffer[256];  // Adjust size as needed, make sure it's large enough!
  int writtenChars = snprintf(dataBuffer, sizeof(dataBuffer),
                              "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.2f\n",
                              timestamp,         // Time(ms) since launch
                              currentAltitude,   // Alt_BMP(m)
                              currentTemp,       // Temp_BMP(C)
                              currentRpm,        // RPM
                              currentZAccel,     // Accel Z (m/s^2)
                              currentGForce,     // G-Force
                              currentVelocity,   // Vel_IMU(m/s)
                              currentDecibels,   // dB
                              currentPressure);  // Press_BME(Pa)

  // Check for truncation or encoding errors
  if (writtenChars < 0 || writtenChars >= sizeof(dataBuffer)) {
    Serial.println("Error: Data formatting failed or buffer too small in logFlightDataToSD!");
    // Handle error appropriately - maybe try logging an error message instead?
    sd_card.logToFile("FORMAT_ERROR\n");  // Log simple error if possible
    return;                               // Don't log potentially corrupt data
  }

  // --- Use the logToFile method (writes to SD card object's buffer) ---
  if (!sd_card.logToFile(dataBuffer)) {
    // Handle write error (e.g., print to Serial)
    // Note: This usually indicates an error caught by print(), disk full error
    // might only be caught during sync() or close().
    Serial.println("Error buffering log data!");
  }
  // Actual write to SD card happens during sync or close.
}


/**
 * @brief Saves a formatted summary of the completed flight to a text file on the SD card.
 * Uses sd_flight_number - 1 because sd_flight_number is incremented *after* landing sequence.
 */
void saveFlightSummaryToSD() {
  // Use sd_flight_number - 1 because it was incremented *after* landing detection
  // before this function is called.
  int flightNumToSave = sd_flight_number - 1;
  if (flightNumToSave < 0) flightNumToSave = 0;  // Safety check if called incorrectly

  char summary_filename[50];
  sprintf(summary_filename, "/flight_data/summary_%d.txt", flightNumToSave);

  Serial.print("Saving summary to: ");
  Serial.println(summary_filename);

  // Use String concatenation (acceptable for infrequent summary save)
  summary = "Flight " + String(flightNumToSave) + " Summary\n";
  summary += "--------------------\n";
  summary += "Apogee (BMP/EMA): " + String(apogee, 2) + " m\n";           // Apogee tracked using EMA of BMP
  summary += "Max Velocity (IMU): " + String(maxVelocity, 2) + " m/s\n";  // Based on IMU reading magnitude
  summary += "Max G-Force: " + String(maxGForce, 2) + " G\n";
  summary += "Flight Time: " + String(flightTime / 1000.0, 2) + " s\n";           // Convert ms to s
  summary += "Landing Velocity (IMU): " + String(landingVelocity, 2) + " m/s\n";  // IMU reading at landing detection
  summary += "Max Absolute RPM: " + String(maxRPM, 1) + "\n";
  summary += "Max Sustained RPM (" + String(RPM_WINDOW_SIZE * (100.0 / 1000.0), 1) + "s window): " + String(currentFlightMaxSustainedRpm, 1) + " RPM\n";  // Show calculated window size
  summary += "Max Decibels: " + String(maxDecibels, 1) + " dB\n";
  summary += "Min Temperature (BMP): " + String(minTemperature, 1) + " C\n";
  summary += "Max Temperature (BMP): " + String(maxTemperature, 1) + " C\n";
  summary += "Min Pressure (BMP): " + String(minPressure / 100.0, 2) + " hPa\n";  // Convert Pa to hPa
  summary += "Max Pressure (BMP): " + String(maxPressure / 100.0, 2) + " hPa\n";  // Convert Pa to hPa
  summary += "Servo Door: " + String(door) + " \n";                               // Convert Pa to hPa
  summary += "--------------------\n";

  // Use the simple writeFile method from the SD card library for the summary
  if (!sd_card.writeFile(summary_filename, summary.c_str())) {
    Serial.println("Error saving summary file!");
  } else {
    Serial.println("Summary file saved.");
  }
}


/**
 * @brief Saves the summary data of the completed flight to the next available EEPROM slot.
 * Increments and wraps the EEPROM flight counter (currentFlight).
 */
void saveDataToEEPROM() {
  // currentFlight should hold the index of the *next* slot to write to (0 to MAX_FLIGHTS-1)
  if (currentFlight < 0 || currentFlight >= MAX_FLIGHTS) {
    Serial.println("Error: Invalid currentFlight index before EEPROM save. Resetting to 0.");
    currentFlight = 0;  // Reset if invalid
  }
  int flightToSaveIndex = currentFlight;  // Use the current index

  // Calculate base address for this flight slot
  // Stride is number of data points * size of largest data type (use float for simplicity, but check FLIGHT_TIME offset)
  int baseAddress = ADDR_FLIGHT_BASE + flightToSaveIndex * FLIGHT_DATA_SIZE * sizeof(float);  // Check size calculation if mixing types significantly

  // Calculate survivability
  bool survivable = calculateSurvivability();

  // Serial.println("\n--- SAVING FLIGHT DATA TO EEPROM ---");
  // Serial.print("Saving to EEPROM Slot Index: ");
  // Serial.println(flightToSaveIndex);
  // Serial.print("Base Address: ");
  // Serial.println(baseAddress);
  // // Print values being saved for confirmation
  // Serial.print("Max G-Force: ");
  // Serial.println(maxGForce);
  // Serial.print("Landing Velocity: ");
  // Serial.println(landingVelocity);
  // Serial.print("Flight Time (ms): ");
  // Serial.println(flightTime);  // Stored as ms
  // Serial.print("Min Temperature: ");
  // Serial.println(minTemperature);
  // Serial.print("Max Temperature: ");
  // Serial.println(maxTemperature);
  // Serial.print("Min Pressure: ");
  // Serial.println(minPressure);
  // Serial.print("Max Pressure: ");
  // Serial.println(maxPressure);
  // Serial.print("Max Velocity: ");
  // Serial.println(maxVelocity);
  // Serial.print("Max Sustained RPM: ");
  // Serial.println(currentFlightMaxSustainedRpm);  // Save the value captured at landing
  // Serial.print("Max Decibels: ");
  // Serial.println(maxDecibels);
  // Serial.print("Apogee: ");
  // Serial.println(apogee);
  // Serial.print("Servo Door: ");
  // Serial.print(door);

  // Store all required data
  EEPROM.put(baseAddress + OFFSET_LANDING_TEMP * sizeof(float), soar_bme.getTemperature());  // TODO: don't save landing data all the time maybe...
  EEPROM.put(baseAddress + OFFSET_APOGEE * sizeof(float), apogee);
  EEPROM.put(baseAddress + OFFSET_LANDING_TIME * sizeof(float), flightTime);

  // Get and store orientation at landing
  float orientation[3];
  soar_imu.GET_EULER(orientation);  // TODO implement this in SOAR_IMU
  EEPROM.put(baseAddress + OFFSET_LANDING_PITCH * sizeof(float), orientation[0]);
  EEPROM.put(baseAddress + OFFSET_LANDING_ROLL * sizeof(float), orientation[1]);

  EEPROM.put(baseAddress + OFFSET_OPENED_DOOR * sizeof(int), door);
  EEPROM.put(baseAddress + OFFSET_MAX_VELOCITY * sizeof(float), maxVelocity);
  EEPROM.put(baseAddress + OFFSET_LANDING_VELOCITY * sizeof(float), landingVelocity);
  EEPROM.put(baseAddress + OFFSET_MAX_G_FORCE * sizeof(float), maxGForce);
  EEPROM.put(baseAddress + OFFSET_MIN_TEMP * sizeof(float), minTemperature);
  EEPROM.put(baseAddress + OFFSET_MAX_TEMP * sizeof(float), maxTemperature);
  EEPROM.put(baseAddress + OFFSET_MAX_RPM * sizeof(float), maxRPM);
  EEPROM.put(baseAddress + OFFSET_MIN_PRESSURE * sizeof(float), minPressure);
  EEPROM.put(baseAddress + OFFSET_MAX_PRESSURE * sizeof(float), maxPressure);
  EEPROM.put(baseAddress + OFFSET_MAX_DECIBELS * sizeof(float), maxDecibels);

  // Increment flight counter for the *next* flight, wrapping around
  currentFlight++;
  if (currentFlight >= MAX_FLIGHTS) currentFlight = 0;
  EEPROM.put(ADDR_FLIGHT_COUNTER, currentFlight);  // Save the counter for the *next* flight

  // Commit changes to EEPROM
  if (EEPROM.commit()) {
    // Serial.println("EEPROM commit successful.");
  } else {
    Serial.println("EEPROM commit failed!");
  }

  // Serial.print("Next EEPROM flight slot will be index: ");
  // Serial.println(currentFlight);
}

bool calculateSurvivability() {
  return (minTemperature >= 10.0 && maxTemperature <= 35.0 && maxRPM <= 60.0 && (minPressure / 101325.0) >= 0.0618 &&  // Convert Pa to atm
          (maxPressure / 101325.0) <= 100.0 &&                                                                         // Convert Pa to atm
          maxGForce <= 5.0 && maxDecibels <= 150.0);
}

/**
 * @brief Resets all flight data slots in EEPROM to zero and resets the flight counter.
 * Also resets the SD card flight number counter.
 */
void resetAllFlightData() {
  Serial.println("Resetting all flight data in EEPROM...");
  for (int i = 0; i < MAX_FLIGHTS; i++) {
    // Calculate base address assuming float size for stride, verify FLIGHT_TIME handling
    int baseAddress = ADDR_FLIGHT_BASE + i * FLIGHT_DATA_SIZE * sizeof(float);
    // Zero out all data points for the flight slot
    for (int j = 0; j < FLIGHT_DATA_SIZE; j++) {
      // Calculate the actual address for the j-th data point
      // This simplified offset calculation assumes all prior data points were floats.
      // Be cautious if data types/sizes before flightTime change.
      int currentOffsetAddress = baseAddress + j * sizeof(float);

      // Special case for flight time stored as unsigned long
      if (j == OFFSET_LANDING_TIME) {
        EEPROM.put(currentOffsetAddress, (unsigned long)0);  // Put 0UL
      } else {
        EEPROM.put(currentOffsetAddress, 0.0f);  // Put 0.0f for floats
      }
    }
    Serial.print("Cleared EEPROM Slot Index: ");
    Serial.println(i);
  }

  // Reset flight counter in EEPROM
  currentFlight = 0;
  EEPROM.put(ADDR_FLIGHT_COUNTER, currentFlight);

  if (EEPROM.commit()) {
    Serial.println("EEPROM reset and commit successful.");
  } else {
    Serial.println("EEPROM reset commit failed!");
  }

  // Reset SD flight number counter for the next session
  sd_flight_number = 0;
  Serial.println("All EEPROM flight slots reset. SD flight counter reset to 0.");

  // Optional: Reset current flight variables too?
  // resetCurrentFlightData();
}

/**
 * @brief Displays the summary data for the *just completed* flight from global variables.
 */
void displayFlightData() {
  Serial.println("\n--- CURRENT FLIGHT SUMMARY ---");
  Serial.print("Landing Temperature: ");
  Serial.print(soar_bme.getTemperature(), 1);
  Serial.println(" C");

  Serial.print("Apogee: ");
  Serial.print(apogee, 2);
  Serial.println(" m");

  Serial.print("Landing Time: T+");
  Serial.print(flightTime / 1000.0, 2);
  Serial.println(" s");

  float orientation[3];
  soar_imu.GET_EULER(orientation);
  Serial.print("Landing Orientation - Pitch: ");
  Serial.print(orientation[0], 1);
  Serial.print("°, Roll: ");
  Serial.print(orientation[1], 1);
  Serial.println("°");

  Serial.print("Opened Door: ");
  Serial.println(door);

  Serial.print("Max Velocity: ");
  Serial.print(maxVelocity, 2);
  Serial.println(" m/s");

  Serial.print("Landing Velocity: ");
  Serial.print(landingVelocity, 2);
  Serial.println(" m/s");

  Serial.print("Max G-Force: ");
  Serial.print(maxGForce, 2);
  Serial.println(" G");

  Serial.print("Survivability: ");
  Serial.println(calculateSurvivability() ? "PASS" : "FAIL");
  Serial.println("----------------------------");
}



/**
 * @brief Reads and displays summary data for all flight slots stored in EEPROM.
 * Correctly reads flightTime as unsigned long.
 */
void displayAllFlightData() {
  Serial.println("\n--- STORED FLIGHT DATA (EEPROM) ---");
  int validFlights = 0;
  int nextFlightIdx = 0;
  EEPROM.get(ADDR_FLIGHT_COUNTER, nextFlightIdx);  // Get which slot is next

  for (int i = 0; i < MAX_FLIGHTS; i++) {
    // Calculate base address - careful with mixed types if structure changes
    int baseAddress = ADDR_FLIGHT_BASE + i * FLIGHT_DATA_SIZE * sizeof(float);

    // Variables to hold all the flight data
    float landingTemp, apogee, landingPitch, landingRoll, maxVelocity;
    float landingVelocity, maxGForce, minTemp, maxTemp, maxRPM;
    float minPressure, maxPressure, maxDecibels;
    unsigned long landingTime;
    int openedDoor;

    // Read all flight data from EEPROM
    EEPROM.get(baseAddress + OFFSET_LANDING_TEMP * sizeof(float), landingTemp);
    EEPROM.get(baseAddress + OFFSET_APOGEE * sizeof(float), apogee);
    EEPROM.get(baseAddress + OFFSET_LANDING_TIME * sizeof(float), landingTime);
    EEPROM.get(baseAddress + OFFSET_LANDING_PITCH * sizeof(float), landingPitch);
    EEPROM.get(baseAddress + OFFSET_LANDING_ROLL * sizeof(float), landingRoll);
    EEPROM.get(baseAddress + OFFSET_OPENED_DOOR * sizeof(int), openedDoor);
    EEPROM.get(baseAddress + OFFSET_MAX_VELOCITY * sizeof(float), maxVelocity);
    EEPROM.get(baseAddress + OFFSET_LANDING_VELOCITY * sizeof(float), landingVelocity);
    EEPROM.get(baseAddress + OFFSET_MAX_G_FORCE * sizeof(float), maxGForce);
    EEPROM.get(baseAddress + OFFSET_MIN_TEMP * sizeof(float), minTemp);
    EEPROM.get(baseAddress + OFFSET_MAX_TEMP * sizeof(float), maxTemp);
    EEPROM.get(baseAddress + OFFSET_MAX_RPM * sizeof(float), maxRPM);
    EEPROM.get(baseAddress + OFFSET_MIN_PRESSURE * sizeof(float), minPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_PRESSURE * sizeof(float), maxPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_DECIBELS * sizeof(float), maxDecibels);

    // Check if slot contains valid data (non-zero landing time)
    if (landingTime > 0) {
      validFlights++;
      Serial.print("\n--- FLIGHT SLOT ");
      Serial.print(i);
      Serial.println(" ---");

      // Calculate survivability for this flight
      bool survivable = (minTemp >= 10.0 && maxTemp <= 35.0 && maxRPM <= 60.0 && (minPressure / 101325.0) >= 0.0618 && (maxPressure / 101325.0) <= 100.0 && maxGForce <= 5.0 && maxDecibels <= 150.0);

      // Display all flight metrics
      Serial.print("Landing Temp: ");
      Serial.print(landingTemp, 1);
      Serial.println("°C");
      Serial.print("Apogee: ");
      Serial.print(apogee, 2);
      Serial.println("m");
      Serial.print("Landing Time: T+");
      Serial.print(landingTime / 1000.0, 2);
      Serial.println("s");
      Serial.print("Orientation - Pitch: ");
      Serial.print(landingPitch, 1);
      Serial.print("°, Roll: ");
      Serial.print(landingRoll, 1);
      Serial.println("°");
      Serial.print("Opened Door: ");
      Serial.println(openedDoor);
      Serial.print("Max Velocity: ");
      Serial.print(maxVelocity, 2);
      Serial.println("m/s");
      Serial.print("Landing Velocity: ");
      Serial.print(landingVelocity, 2);
      Serial.println("m/s");
      Serial.print("Max G-Force: ");
      Serial.print(maxGForce, 2);
      Serial.println("G");
      Serial.print("Temp Range: ");
      Serial.print(minTemp, 1);
      Serial.print(" to ");
      Serial.print(maxTemp, 1);
      Serial.println("°C");
      Serial.print("Max RPM: ");
      Serial.print(maxRPM, 1);
      Serial.println(" RPM");
      Serial.print("Pressure Range: ");
      Serial.print(minPressure / 101325.0, 4);
      Serial.print(" to ");
      Serial.print(maxPressure / 101325.0, 4);
      Serial.println(" atm");
      Serial.print("Max Noise: ");
      Serial.print(maxDecibels, 1);
      Serial.println(" dB");
      Serial.print("Survivability: ");
      Serial.println(survivable ? "PASS" : "FAIL");
      Serial.println("-------------------");
    } else {
      // Optional: Print that the slot is empty or appears unused
      Serial.print("\n--- FLIGHT SLOT ");
      Serial.print(i);
      Serial.println(" (EMPTY) ---");
    }
  }

  if (validFlights == 0) {
    Serial.println("No valid flight data found in EEPROM.");
  }
  Serial.print("(Next flight will use EEPROM slot: ");
  Serial.print(nextFlightIdx);
  Serial.println(")");
  Serial.println("--- End of Stored Data ---");
}

void restoreLastFlightData() {
  // Find the most recent flight (previous index before currentFlight)
  int lastFlightIndex = currentFlight - 1;
  if (lastFlightIndex < 0) lastFlightIndex = MAX_FLIGHTS - 1;

  int baseAddress = ADDR_FLIGHT_BASE + lastFlightIndex * FLIGHT_DATA_SIZE * sizeof(float);

  // Read all flight data from EEPROM
  unsigned long storedFlightTime;
  EEPROM.get(baseAddress + OFFSET_LANDING_TIME * sizeof(float), storedFlightTime);

  // Only restore if we find valid flight data (non-zero flight time)
  if (storedFlightTime > 0) {
    Serial.println("Restoring data from last flight...");
    // Restore all flight metrics
    // EEPROM.get(baseAddress + OFFSET_LANDING_TEMP * sizeof(float), landingTemp); // TODO: make landing stuff only save on landing.
    EEPROM.get(baseAddress + OFFSET_APOGEE * sizeof(float), apogee);
    EEPROM.get(baseAddress + OFFSET_LANDING_TIME * sizeof(float), flightTime);
    // EEPROM.get(baseAddress + OFFSET_LANDING_PITCH * sizeof(float), landingPitch);
    // EEPROM.get(baseAddress + OFFSET_LANDING_ROLL * sizeof(float), landingRoll);
    EEPROM.get(baseAddress + OFFSET_OPENED_DOOR * sizeof(int), door);
    EEPROM.get(baseAddress + OFFSET_MAX_VELOCITY * sizeof(float), maxVelocity);
    EEPROM.get(baseAddress + OFFSET_LANDING_VELOCITY * sizeof(float), landingVelocity);
    EEPROM.get(baseAddress + OFFSET_MAX_G_FORCE * sizeof(float), maxGForce);
    EEPROM.get(baseAddress + OFFSET_MIN_TEMP * sizeof(float), minTemperature);
    EEPROM.get(baseAddress + OFFSET_MAX_TEMP * sizeof(float), maxTemperature);
    EEPROM.get(baseAddress + OFFSET_MAX_RPM * sizeof(float), maxRPM);
    EEPROM.get(baseAddress + OFFSET_MIN_PRESSURE * sizeof(float), minPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_PRESSURE * sizeof(float), maxPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_DECIBELS * sizeof(float), maxDecibels);

    // Restore flight state if flight was in progress
    if (flightTime > 0 && flightTime < 300000) {  // If flight was under 5 minutes
      currentState = DESCENDING;                  // Or determine state from other metrics
      flightStartTime = millis() - flightTime;
      Serial.println("Restored in-progress flight state");
    } else {
      currentState = LANDED;
      Serial.println("Restored completed flight data");
    }
  } else {
    Serial.println("No valid flight data to restore");
  }
}
// --- Helper Function to Set DAC Output (Unchanged from previous) ---
bool setDacOutput(uint16_t value) {
  Wire.beginTransmission(DAC_I2C_ADDR);
  Wire.write(DAC_CMD_WRITE_UPDATE);  // Send Command Byte
  Wire.write((value >> 8) & 0xFF);   // Send High Byte of Data
  Wire.write(value & 0xFF);          // Send Low Byte of Data
  byte error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("I2C Error writing to DAC: ");
    Serial.println(error);
    return false;
  }
  return true;
}

void playMorse(std::string message, int time_unit) {
  //Serial.println("HI THIS SHOULD BE PRINTING IF THE FUNCTION WORKS");
  digitalWrite(PTTPIN, LOW);      //Start transmit
  for (int i = 0; i < message.length(); i++) {
    char character = toupper(message[i]);

    if (character == ' ') {
      delay(time_unit * 7); // Word gap
      continue;
    }

    auto iterator = morseMap.find(character);    //Iterator's like auto, apparently.
    if (iterator != morseMap.end()) {
      std::string morse_code = iterator->second;
      Serial.print(morse_code.c_str());     //test print

      for (int j = 0; j < morse_code.length(); j++) {
        if (morse_code[j] == '.') {
          generateTone(time_unit);         // Dot
        } else if (morse_code[j] == '-') {
          generateTone(time_unit * 3);     // Dash
        }
        delay(time_unit); // Gap between symbols
    }

    delay(time_unit * 2); // Already had 1-time_unit pause after symbol; add 2 more to make 3-time_unit letter space
    }
  }
  delay(time_unit*5);
  digitalWrite(PTTPIN, HIGH);
  delay(time_unit*5);
}

void generateTone(int duration) {
  int amplitude = 32767; // Peak amplitude (0-255)
  int offset = 32767;    // Center value (~2.5V)
  float angle = 0;  
  
  long waveStart = millis();
  while(millis()-waveStart<duration){
    angle = 2.0 * PI * 600* (micros() - waveStart) / 1000000.0;
    if(angle>2*PI)angle = angle-2*PI;
    int dacValue = (int)(offset + amplitude * sin(angle));
    setDacOutput(dacValue);
  }
  setDacOutput(0);

  return;
}

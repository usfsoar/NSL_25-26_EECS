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
#include <cmath> // Include for isnan, isinf if needed, and math functions

#include <vector>
#include <esp32-hal-ledc.h>
#include <esp32-hal-gpio.h>
#include <DRA818.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <HardwareSerial.h>
#include <map>

// xbee
#include "XBeeSender.h"
#include <SoftwareSerial.h>

// game
#include "DIGITAL_TWIN.h"
#include "_config.h"

// eeprom
#define EEPROM_SIZE 1024
#define FLOATS_PER_SLOT 13
#define SLOT_COUNT 3
#define SLOT_SIZE (FLOATS_PER_SLOT * sizeof(float)) // 13 floats * 4 bytes = 52 bytes
#define BASE_ADDRESS 0
#define SLOT_SAVE_ADDRESS 900 // Where currentSlot is saved
#define MAGIC_NUMBER 0xA5A5
int currentSlot = 0; // Index (0 to SLOT_COUNTS-1) for the *next* flight slot in

#define PTTPIN 8
#define DRA818_SERIAL Serial1 // Define the serial interface for the DRA818V
#define DRA818_PD 7
#define DRA818_PL 45

// DRA818V configuration parameters
#define DRA818_TYPE DRA818_VHF // Module type: VHF
#define RX_FREQUENCY 144.360   // Receive frequency in MHz
#define TX_FREQUENCY 144.360   // Transmit frequency in MHz
#define SQUELCH_LEVEL 4        // Squelch level (0-8)
#define VOLUME_LEVEL 8         // Volume level (0-8)
#define CTCSS_RX 0             // CTCSS receive code (0 if not used)
#define CTCSS_TX 0             // CTCSS transmit code (0 if not used)
#define BANDWIDTH DRA818_12K5  // Bandwidth: 12.5 kHz
#define PREEMPHASIS true       // Pre-emphasis enabled
#define HIGH_PASS true         // High-pass filter enabled
#define LOW_PASS true          // Low-pass filter enabled

// --- Configuration ---

// --- NEW: Control Pin Definitions ---
const int DAC_LDAC_PIN = 9; // Connected to LDAC
const int DAC_A0_PIN = 10;  // Connected to A0 (Address Select)

// --- I2C Address ---
// Set based on how you drive DAC_A0_PIN
// 0x4C if A0 is LOW, 0x4D if A0 is HIGH
const int DAC_I2C_ADDR = 0x4C;

// AD5693 Command Byte (VERIFY FROM DATASHEET!)
const uint8_t DAC_CMD_WRITE_UPDATE = 0x30; // Assumed command

std::map<char, std::string> morseMap = {
    // Letters
    {'A', ".-"},
    {'B', "-..."},
    {'C', "-.-."},
    {'D', "-.."},
    {'E', "."},
    {'F', "..-."},
    {'G', "--."},
    {'H', "...."},
    {'I', ".."},
    {'J', ".---"},
    {'K', "-.-"},
    {'L', ".-.."},
    {'M', "--"},
    {'N', "-."},
    {'O', "---"},
    {'P', ".--."},
    {'Q', "--.-"},
    {'R', ".-."},
    {'S', "..."},
    {'T', "-"},
    {'U', "..-"},
    {'V', "...-"},
    {'W', ".--"},
    {'X', "-..-"},
    {'Y', "-.--"},
    {'Z', "--.."},

    // Digits
    {'0', "-----"},
    {'1', ".----"},
    {'2', "..---"},
    {'3', "...--"},
    {'4', "....-"},
    {'5', "....."},
    {'6', "-...."},
    {'7', "--..."},
    {'8', "---.."},
    {'9', "----."},

    // Punctuation / special characters
    {'.', ".-.-.-"},
    {',', "--..--"},
    {'?', "..--.."},
    {'\'', ".----."},
    {'!', "-.-.--"},
    {'/', "-..-."},
    {'(', "-.--."},
    {')', "-.--.-"},
    {'&', ".-..."},
    {':', "---..."},
    {';', "-.-.-."},
    {'=', "-...-"},
    {'+', ".-.-."},
    {'-', "-....-"},
    {'_', "..--.-"},
    {'\"', ".-..-."},
    {'$', "...-..-"},
    {'@', ".--.-."}};

// Define I2C Pins
const int I2C_SDA_PIN = 1;
const int I2C_SCL_PIN = 2;

// Define I2S Pins
const int I2S_WS_PIN = 13;  // LRC/WS
const int I2S_SD_PIN = 12;  // DOUT/SD
const int I2S_SCK_PIN = 11; // BCLK/SCK

// Define SD Card SPI Pins
const int SD_CS_PIN = 42;
const int SD_CLK_PIN = 41;
const int SD_MOSI_PIN = 6;
const int SD_MISO_PIN = 5;

// Object Instantiations
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);           // Default I2C address
SOAR_IMU soar_imu;                                                     // Uses default Wire (initialized below)
SOAR_BAROMETER soar_barometer;                                         // Uses default Wire
SPH0645_Mic soar_sph(I2S_WS_PIN, I2S_SD_PIN, I2S_SCK_PIN);             // Use defined I2S pins
SOAR_PAYLOAD_SERVO p_serv;                                             // Uses default Wire
SOAR_SD_CARD sd_card(SD_CS_PIN, SD_CLK_PIN, SD_MOSI_PIN, SD_MISO_PIN); // Use modified constructor
SOAR_BME soar_bme(I2C_SDA_PIN, I2C_SCL_PIN);                           // Use defined I2C pins
// DRA818* dra;

// xbee
//  SoftwareSerial XBeeSerial(4, 3);                                       //SoftwareSerial for Waveshare S3 ESP32
//  uint8_t destinationAddress[8] = { 0x00, 0x13, 0xA2, 0x00, 0x41, 0x06, 0x7F, 0x53 };     //Check this address with the RECEIVING XBee before launch
//  XBeeSender xbee(XBeeSerial, destinationAddress);                        //XBee Initialization

// ------ Refactored Function Prototypes ------
void readAllSensors(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag);
void readSimulation(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag);
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag);
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag);
void updateMaxSustainedRPM(float currentRpm);
void handleLandingSequence(const float finalGravityReading[3]);
bool startNewLogFile(const char *filename);
void resetServos();
void initializeEMAs();
void displayFlightData(); // Display summary of *just completed* flight
void logFlightDataToSD(unsigned long timestamp, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure); // Modified log function
void saveFlightSummaryToSD();                  // Save summary to SD Card
void playMorse(std::string message, int time_unit);
void generateTone(int duration);
bool setDacOutput(uint16_t value);
bool calculateSurvivability();

struct FlightData
{
  uint16_t magic = MAGIC_NUMBER;
  float emaGForce = 1.0;
  float emaAltitude = 0.0;
  float emaVelocity = 0.0;

  float groundLevel = 0.0;    // BMP ground level altitude
  float bmeGroundLevel = 0.0; // BME ground level altitude

  float apogee = 0.0;
  float flightTime = 0; // Total flight duration in ms
  float maxGForce = 0.0;
  float maxSusRPM = 0.0; // Max sustained RPM for the *completed* flight
  float maxDecibels = 0;
  float minTemperature = 1000.0;  // Initialize to high value
  float maxTemperature = -1000.0; // Initialize to low value
  float minPressure = 200000.0;   // Initialize to high value
  float maxPressure = 0.0;        // Initialize to low value
  float maxVelocity = 0.0;
  float maxRPM = 0.0;
  float currentState = 0.0; // Current flight state
};

void saveEEPROM();    // Save current flight data to EEPROM
void restoreEEPROM(); // RESTORED prototype
void resetEEPROM();
void displaySlots();
void displayEEPROM(const FlightData &var);

// led
#define LED_PIN 21
#define NUM_LEDS 1
Adafruit_NeoPixel onboardLed = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Global variables
int sd_flight_number = 0; // Counter for SD card filenames (flight_0.csv, flight_1.csv, ...)
char current_filename[40];

// Flight state definitions
// enum FlightState {
//   READY,
//   LAUNCHED,
//   DESCENDING,
//   LANDED
// };
// FlightState flightData.currentState = READY;

// Constants Threshold
const float LAUNCH_ACCEL_THRESHOLD = 1.2; // G force threshold to detect launch
const float LAUNCH_ALTITUDE_THRESHOLD = 10.0;
const float DESCENT_ALTITUDE_THRESHOLD = 1.0; // Meters fallen below flightData.apogee to confirm descent
const float DESCENT_APOGEE_THRESHOLD = 0.3;
const float LANDING_ACCEL_THRESHOLD = 1.1; // G force threshold difference from 1G to detect landing
const float LANDING_VEL_THRESHOLD = 0.7;   // m/s threshold to confirm landing
const float LANDING_ALTITUDE_THRESHOLD = -0.3;

const int STABLE_READINGS_FOR_LAUNCH = 3;
const int STABLE_READINGS_FOR_DESCENT = 3;
const int STABLE_READINGS_FOR_LANDING = 10;
const int STABLE_READINGS_FOR_LANDING_VELOCITY_GFORCE = 3;

// --- Smoothing Factors (Alpha for EMA) ---
const float ALPHA_GFORCE = 0.8;   // EMA alpha for G-Force
const float ALPHA_ZACCEL = 0.8;   // EMA alpha for Z-axis linear acceleration
const float ALPHA_ALTITUDE = 0.5; // EMA alpha for Altitude
const float ALPHA_VELOCITY = 0.8; // EMA alpha for Velocity magnitude

// --- EMA State Variables ---
FlightData flightData;

// float flightData.emaGForce = 1.0;  // Initialize to approx. 1G at rest
float emaZAccel = 0.0;
// float flightData.emaAltitude = 0.0;
// float flightData.emaVelocity = 0.0;
bool emaInitialized = false; // Flag to track if EMAs have been initialized

// Flight data variables (for the current/ongoing flight)
// infinty
// float flightData.maxGForce = 0.0;
float landingVelocity = -100.0; // Final velocity recorded at landing detection
float landingGForce = -100.0;
unsigned long flightStartTime = 0;
// unsigned long flightData.flightTime = 0;    // Total flight duration in ms
// float flightData.minTemperature = 1000.0;   // Initialize to high value
// float flightData.maxTemperature = -1000.0;  // Initialize to low value
// float flightData.minPressure = 200000.0;    // Initialize to high value
// float flightData.maxPressure = 0.0;         // Initialize to low value
float maxVelocity = 0.0; // Max velocity magnitude recorded
// float flightData.maxDecibels = 0;
float maxRPM = 0.0; // Absolute peak RPM recorded
// float flightData.apogee = 0.0;                        // Highest relative altitude recorded
// float flightData.maxSusRPM = 0.0;  // Max sustained RPM for the *completed* flight
int stableReadingLaunch = 0;
int stableReadingDescent = 0;
int stableReadingLand = 0;
bool descentStarted = false;
float prevEmaVelocity = 0;
float prevEmaGForce = 0;
float currentTemp = 0.0;
float pitch = 0.0;
float roll = 0.0;

int door = -1;
String summary = "";
String broadcast_string = "KQ4FYU String Initialized";

// Sustained RPM Calculation Variables
const unsigned long RPM_SUSTAINED_INTERVAL_MS = 100; // Not directly used in window calc, but for context
const int RPM_WINDOW_SIZE = 120;                     // Buffer size for ~1 second of data + buffer (adjust based on loop rate)
float rpmWindow[RPM_WINDOW_SIZE];                    // Circular buffer for recent RPM readings
int rpmWindowIndex = 0;                              // Current index in the circular buffer
int rpmSampleCount = 0;                              // How many samples currently in buffer (up to RPM_WINDOW_SIZE)
float maxSustainedRpm = 0.0;                         // The maximum sustained RPM calculated *so far* during flight

// SD Card Sync Interval
const unsigned long SD_SYNC_INTERVAL_MS = 1000;
static uint32_t lastSyncTime = 0; // Ensure this is declared globally

// --- Setup Function ---
void setup()
{
  Serial.begin(115200);
  delay(3000); // Wait for serial monitor connection

  Serial.println("Rocket Flight Data Recorder - Waveshare ESP32-S3-Zero");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C Initialized on SDA=1, SCL=2");

  // Initialize Servo Controller
  p_serv.initialize(pwm);
  Serial.println("Servo Controller Initialized");

  // Initialize IMU
  soar_imu.BNO_SETUP();

  // Initialize MIC
  soar_sph.begin();
  Serial.println("SPH0645 Mic Initialized");

  // --- Initialize NeoPixel ---
  onboardLed.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  onboardLed.setBrightness(50); // Set BRIGHTNESS (optional, 0-255) - Start low!
  onboardLed.clear();           // Set all pixel colors to 'off'
  onboardLed.show();            // Send the clear command to the pixel
  Serial.println("Onboard NeoPixel Initialized.");

  // Initialize BMP
  soar_barometer.Initialize();

  // Initialize BME
  soar_bme.initialize();

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialize EEPROM!");
  }
  else
  {
    Serial.println("EEPROM Initialized.");

    // 1) Restore currentSlot (might be garbage on a fresh upload)
    EEPROM.get(SLOT_SAVE_ADDRESS, currentSlot);
    if (currentSlot < 0 || currentSlot >= SLOT_COUNT)
    {
      currentSlot = 0;
    }
    Serial.println("Startup: currentSlot = " + String(currentSlot));

    // 2) Load flightData (also garbage if never saved)
    int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
    EEPROM.get(addr, flightData);
    Serial.print("Startup: read magic = 0x");
    Serial.println(flightData.magic, HEX);

    // 3) If magic is wrong, *seed* slot 0 with defaults
    if (flightData.magic != MAGIC_NUMBER)
    {
      Serial.println("Magic mismatch → Seeding EEPROM with default data");
      flightData = FlightData(); // resets all floats to 0.0 + magic
      currentSlot = 0;
      saveEEPROM();
      displaySlots();
    }
    else
    {
      // 4) magic OK: now check var13 range for restore
      if (flightData.currentState == 1.0 && flightData.currentState == 2.0)
      {
        Serial.println("***Restoring Slot***");
        Serial.print("Current State: ");
        Serial.println(flightData.currentState);
        restoreEEPROM();
      }
      else
      {
        Serial.println("***No Restore***");
      }
    }

    displaySlots();
  }

  if (flightData.currentState == 0.0)
  {
    Serial.println("***No Restore***");
    for (int i = 0; i < 3; i++)
    {
      // Serial.print("BMP Alt: ");
      soar_barometer.get_altitude();
      // Serial.print("BME Alt: ");
      soar_bme.getAltitude();
      delay(500);
    }
    flightData.groundLevel = soar_barometer.get_altitude();
    flightData.bmeGroundLevel = soar_bme.getAltitude();
    // Serial.print("BMP Ground: ");
    Serial.println(flightData.groundLevel);
    // Serial.print("BME Ground: ");
    Serial.println(flightData.bmeGroundLevel);
  }

  // --- Conditional Initialization based on Restored State --- RESTORED Logic
  if (flightData.currentState == 0.0)
  {
    onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 50)); // Cyan for READY
    onboardLed.show();

    // Initialize SD Card and find next file number
    sd_card.begin();
    sd_card.createDir("/flight_data");
    sd_flight_number = 0; // Start search from 0
    while (true)
    {
      sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
      if (!sd_card.exists(current_filename))
        break;
      sd_flight_number++;
      if (sd_flight_number > 9999)
      {
        Serial.println("Warning: SD flight number exceeded 9999!");
        sd_flight_number = 9999; // Use max number
        sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
        break; // Safety break
      }
    }
    Serial.print("Next SD file number: ");
    Serial.println(sd_flight_number);
    Serial.print("Log filename: ");
    Serial.println(current_filename);

    // Initialize ground level and EMAs only if starting fresh
    Serial.println("Getting initial sensor readings for ground level and EMA init...");
    initializeEMAs();
    // resetcurrentSlotData(); // Ensure all flight vars are reset
    resetEEPROM();
  }
  else
  {
    // Restored mid-flight (LAUNCHED or DESCENDING) or LANDED
    Serial.println("Restored previous state. Performing minimal initialization...");

    // Set LED based on restored state
    if (flightData.currentState == 1.0)
    {
      onboardLed.setPixelColor(0, onboardLed.Color(50, 0, 0)); // Red for LAUNCHED
    }
    else if (flightData.currentState == 2.0)
    {
      onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 0)); // Green for DESCENDING
    }
    else if (flightData.currentState == 3.0)
    {
      onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 50)); // Blue for LANDED
    }
    onboardLed.show();

    // Initialize SD card
    sd_card.begin();
    sd_card.createDir("/flight_data"); // Ensure directory exists

    // Determine SD flight number based on restored state
    // If LANDED, prepare for the *next* flight number.
    // If LAUNCHED/DESCENDING, assume we continue the *last* flight's log.
    // This logic might need refinement depending on exact restore requirements.
    int lastFlightIndex = currentSlot - 1;
    if (lastFlightIndex < 0)
      lastFlightIndex = SLOT_COUNT - 1;
    sd_flight_number = lastFlightIndex; // Assume continuing the log of the restored flight index

    if (flightData.currentState == 3.0)
    {
      sd_flight_number++; // If landed, prepare for the next flight number
    }

    sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
    Serial.print("Using SD flight number: ");
    Serial.println(sd_flight_number);
    Serial.print("Log filename: ");
    Serial.println(current_filename);

    // If LAUNCHED or DESCENDING, attempt to open the existing log file to append
    if (flightData.currentState == 1.0 || flightData.currentState == 2.0)
    {
      if (sd_card.exists(current_filename))
      {
        Serial.print("Attempting to append to existing log file: ");
        Serial.println(current_filename);
        if (!sd_card.openLogFile(current_filename))
        {
          Serial.println("CRITICAL: Failed to open existing log file for appending!");
          // Consider forcing state to LANDED or other error handling
          // flightData.currentState = LANDED;
        }
        else
        {
          sd_card.logToFile("\n--- RESTORED MID-FLIGHT ---\n"); // Add a marker
          lastSyncTime = millis();                              // Reset sync timer
        }
      }
      else
      {
        Serial.print("Warning: Log file for restored flight not found. Starting new file: ");
        Serial.println(current_filename);
        if (!startNewLogFile(current_filename))
        {
          Serial.println("CRITICAL: Failed to start new log file after restore!");
          // flightData.currentState = LANDED;
        }
        else
        {
          lastSyncTime = millis();
        }
      }
    }

    // EMAs need to be re-initialized, but NOT with ground level.
    // Let them reconverge from the next sensor readings.
    initializeEMAs(); // Re-initialize based on current sensor readings
    Serial.println("EMAs re-initialized (will reconverge).");

    // Do NOT call resetcurrentSlotData() if restoring mid-flight
    // Do NOT call captureflightData.groundLevel()
    // Do NOT reset servos yet (state machine will handle)
  }
  // --- End Conditional Init ---

  // Initialize Sustained RPM tracking variables
  for (int i = 0; i < RPM_WINDOW_SIZE; ++i)
  {
    rpmWindow[i] = 0.0; // Clear the buffer
  }
  rpmWindowIndex = 0;
  rpmSampleCount = 0;
  maxSustainedRpm = 0.0; // Reset max sustained value at power-on

  // Serial.print("Ground Level BMP Alt: "); Serial.println(flightData.groundLevel); // Ground level might not be set if restored
  // Serial.print("Ground Level BME Alt: "); Serial.println(flightData.bmeGroundLevel);
  Serial.print("Initial/Restored EMA Values -> GForce: ");
  Serial.print(flightData.emaGForce, 2);
  Serial.print(" | ZAccel: ");
  Serial.print(emaZAccel, 2);
  Serial.print(" | Altitude: ");
  Serial.print(flightData.emaAltitude, 2);
  Serial.print(" | Velocity: ");
  Serial.println(flightData.emaVelocity, 2);

  // radio
  pinMode(PTTPIN, OUTPUT);
  digitalWrite(PTTPIN, HIGH);

  pinMode(DRA818_PD, OUTPUT);
  digitalWrite(DRA818_PD, HIGH);
  pinMode(DRA818_PL, OUTPUT);
  digitalWrite(DRA818_PL, LOW);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);
  // while (!Serial); // Wait for Serial Monitor to open

  Serial.println("AD5693 DAC Test Sketch (with LDAC/A0 control)");

  // --- Configure Control Pins ---
  pinMode(DAC_A0_PIN, OUTPUT);
  digitalWrite(DAC_A0_PIN, LOW); // Set A0 LOW for I2C Address 0x4C
  Serial.println("Set A0 Pin (GPIO " + String(DAC_A0_PIN) + ") LOW for Address 0x4C");

  pinMode(DAC_LDAC_PIN, OUTPUT);
  digitalWrite(DAC_LDAC_PIN, LOW); // Hold LDAC LOW for immediate output updates
  Serial.println("Set LDAC Pin (GPIO " + String(DAC_LDAC_PIN) + ") LOW");
  // --- End Control Pin Config ---

  Wire.beginTransmission(DAC_I2C_ADDR);
  byte error = Wire.endTransmission();
  if (error == 0)
  {
    Serial.print("DAC found at address 0x");
    Serial.println(DAC_I2C_ADDR, HEX);
  }
  else
  {
    Serial.print("DAC not found at address 0x");
    Serial.println(DAC_I2C_ADDR, HEX);
    Serial.println("Check wiring, A0 pin state, and I2C address. Halting.");
    // while (1); // Stop execution
  }

  // --- Final Setup Steps ---
  resetServos(); // Set servos to initial position

  Serial.println("Setup Complete.");
  if (flightData.currentState != 0.0)
  {
    Serial.print("Restored state: ");
    Serial.println(flightData.currentState);
  }
  else
  {
    Serial.println("Waiting for launch...");
  }
}

// --- Main Loop ---
void loop()
{
  static uint32_t lastLogTime = 0;
  // static uint32_t lastSyncTime = 0; // Now global

  // Check for serial commands
  if (Serial.available() > 0)
  {
    char command = Serial.read();
    if (command == 'D' || command == 'd')
    {
      Serial.println("Displaying All Stored Flight Data:");
      displaySlots();
    }
    else if (command == 'R' || command == 'r')
    {
      // Optional: Reset current flight state variables too?
      resetEEPROM(); // If you want 'R' to fully reset the current state too
      ESP.restart();
      // flightData.currentState = 0.0;
      //  initializeEMAs();                                          // Re-initialize EMAs
      //  onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 50));  // Cyan for READY
      //  onboardLed.show();
    }
  }

  // --- Sensor Readings ---
  float currentAltitude, currentTemp, currentPressure, currentDecibels, bmpPressure;
  float acceleration[3], linearAccel[3], gravity[3], velocity[1], rpmReading[1];
  float currentGForce, currentVelocityMag;

  if (SIMULATION)
  {
    readSimulation(currentAltitude, currentTemp,
                   currentPressure, bmpPressure, currentDecibels,
                   acceleration, linearAccel, gravity, velocity, rpmReading,
                   currentGForce, currentVelocityMag);
  }
  else
  {
    readAllSensors(currentAltitude, currentTemp,
                   currentPressure, bmpPressure, currentDecibels,
                   acceleration, linearAccel, gravity, velocity, rpmReading,
                   currentGForce, currentVelocityMag);
  }

  // --- Update EMAs ---
  if (emaInitialized)
  {
    updateEMAs(currentGForce, linearAccel[2], currentAltitude, currentVelocityMag);
  }
  else
  {
    Serial.println("Warning: EMAs not initialized!");
    delay(100); // Avoid spamming
    return;     // Skip the rest of the loop until initialized
  }

  // --- Data Updates & Logging (Common to Launched & Descending) ---
  if (flightData.currentState == 1.0 || flightData.currentState == 2.0)
  {
    // Update Min/Max values
    updateFlightExtremes(currentGForce, rpmReading[0], currentAltitude, currentTemp, currentPressure, currentDecibels, currentVelocityMag);

    // // Update Max Sustained RPM
    // updateMaxSustainedRPM(rpmReading[0]);

    uint32_t now = millis();

    // Log data periodically SD CARD
    if (now - lastLogTime >= 100)
    { // Log approx every 100ms
      logFlightDataToSD(now - flightStartTime, currentAltitude, currentTemp,
                        rpmReading[0], linearAccel[2], currentGForce, velocity[0], currentDecibels,
                        currentPressure);
      lastLogTime = now;
    }
    // Periodically sync the SD card buffer
    if (now - lastSyncTime >= SD_SYNC_INTERVAL_MS)
    {
      if (!sd_card.syncLogFile())
      {
        Serial.println("Error syncing log file!");
      }
      lastSyncTime = now;
    }
    saveEEPROM(); // Save current flight data to EEPROM
    currentSlot = (currentSlot + 1) % SLOT_COUNT;
    Serial.println("EEPROM saved");
  }

  // --- Flight State Machine ---
  switch ((int)flightData.currentState)
  {
  case 0:
    resetServos(); // Ensure servos are reset while waiting

    // Check for launch conditions using EMA values
    if (flightData.emaGForce > LAUNCH_ACCEL_THRESHOLD || flightData.emaAltitude > LAUNCH_ALTITUDE_THRESHOLD)
    { // Added altitude check
      stableReadingLaunch++;
    }
    else
    {
      stableReadingLaunch = 0;
    }

    if (stableReadingLaunch >= STABLE_READINGS_FOR_LAUNCH)
    {
      Serial.println("****** LAUNCH DETECTED! ******");
      flightStartTime = millis();

      // resetcurrentSlotData(); // Reset flight variables for the new flight
      flightData.currentState = 1.0;

      // Set LED red
      onboardLed.setPixelColor(0, onboardLed.Color(50, 0, 0));
      onboardLed.show();

      // Start the new log file (filename already determined in setup or reset)
      if (startNewLogFile(current_filename))
      {
        lastSyncTime = millis(); // Reset sync timer for the new file
      }
      else
      {
        Serial.println("Failed to start log file.");
        // flightData.currentState = 3.0;  // Go to landed state to prevent further action
      }
    }
    break;

  case 1:
    resetServos(); // Keep servos reset during ascent

    // Check for flightData.apogee (using smoothed altitude)
    // flightData.apogee is updated in updateFlightExtremes

    // Check if altitude has decreased sufficiently below flightData.apogee
    if (!descentStarted && flightData.apogee > DESCENT_APOGEE_THRESHOLD && flightData.emaAltitude < (flightData.apogee - DESCENT_ALTITUDE_THRESHOLD))
    { // Ensure some flightData.apogee reached
      stableReadingDescent++;
    }
    else
    {
      stableReadingDescent = 0;
    }

    if (stableReadingDescent >= STABLE_READINGS_FOR_DESCENT)
    {
      flightData.currentState = 2.0;
      descentStarted = true;                                   // Mark descent started
      onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 0)); // Green for DESCENDING
      onboardLed.show();
      Serial.print("****** APOGEE REACHED (EMA): ");
      Serial.print(flightData.apogee, 2);
      Serial.println(" meters ******");
      // Save state change immediately
    }

    // Timeout failsafe
    if (flightStartTime > 0 && ((millis() - flightStartTime) / 1000.0) > 300.0)
    { // Check flightStartTime > 0
      Serial.println("****** FLIGHT TIMEOUT (LAUNCHED) - ASSUMING LANDING ******");
      handleLandingSequence(gravity); // Pass final sensor readings
      // flightData.currentState is set inside handleLandingSequence
    }
    break;

  case 2:
    resetServos(); // Keep servos reset during descent

    // Check for landing conditions using EMA values //TODO: lianding threshold
    if (flightData.emaVelocity < LANDING_VEL_THRESHOLD && abs(flightData.emaGForce - 1.0) < LANDING_ACCEL_THRESHOLD && flightData.emaAltitude < LANDING_ALTITUDE_THRESHOLD)
    { // Adjusted altitude check slightly
      stableReadingLand++;
    }
    else
    {
      stableReadingLand = 0; // Reset count if conditions aren't met
    }

    if (stableReadingLand >= STABLE_READINGS_FOR_LANDING_VELOCITY_GFORCE)
    {
      prevEmaVelocity = (flightData.emaVelocity - (currentVelocityMag * ALPHA_VELOCITY)) / (1.0 - ALPHA_VELOCITY);
      prevEmaGForce = (flightData.emaGForce - (currentGForce * ALPHA_GFORCE)) / (1.0 - ALPHA_GFORCE);

      if (prevEmaVelocity > landingVelocity)
      {
        landingVelocity = prevEmaVelocity;
      }
      if (prevEmaGForce > landingGForce)
      {
        landingGForce = prevEmaGForce;
      }
    }

    // Confirm landing after N stable readings or timeout
    if (stableReadingLand >= STABLE_READINGS_FOR_LANDING || (flightStartTime > 0 && ((millis() - flightStartTime) / 1000.0) > 300.0))
    { // Check flightStartTime > 0
      if (stableReadingLand >= STABLE_READINGS_FOR_LANDING)
      {
        Serial.println("****** LANDING DETECTED! ******");
      }
      else
      {
        Serial.println("****** FLIGHT TIMEOUT (DESCENDING) - ASSUMING LANDING ******");
      }
      flightData.currentState = 3.0;

      handleLandingSequence(gravity); // Pass final sensor readings
      // flightData.currentState is set inside handleLandingSequence
    }
    break;

  case 3:
    door = p_serv.servoLogic(pwm, gravity);
    if (SIMULATION)
    {
      setSimulatedServo(door, 44);
    }
    // Rocket is on the ground. Wait for reset command 'R'/'r' via Serial.
    // Do nothing actively, state handled by handleLandingSequence and reset command.
    delay(100); // Small delay to prevent busy-waiting if needed
    break;
  } // End switch(flightData.currentState)

  // --- Debug Prints (Optional) --- RESTORED
  Serial.print("Time: ");
  Serial.println(millis());
  Serial.print("BME Pressure: ");
  Serial.println(currentPressure);
  Serial.print("BMP Pressure: ");
  Serial.println(bmpPressure);
  Serial.print("GForce(EMA): ");
  Serial.println(flightData.emaGForce);
  Serial.print("Altitude(EMA): ");
  Serial.println(flightData.emaAltitude);
  Serial.print("Velocity(EMA): ");
  Serial.println(flightData.emaVelocity);
  Serial.print("State: ");
  Serial.println(flightData.currentState);

  // char message[100];
  // sprintf(message, "1, 4, State, %f, GForce, %.2f, Altitude, %.2f, Velocity, %.2f", currentState, emaGForce, emaAltitude, emaVelocity);
  // xbee.send(message);

  if (SIMULATION)
  {
    delay(1500);
  }

} // End loop()

// ------ Sensor Reading Function ------
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
void readAllSensors(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag)
{
  // Read IMU
  soar_imu.GET_ACCELERATION(acceleration);
  soar_imu.GET_LINEARACCEL(linearAccel);
  soar_imu.GET_VELOCITY(velocity); // Gets IMU's calculated velocity component
  soar_imu.GET_GRAVITY(gravity);
  soar_imu.GET_RPM(rpmReading); // Reads into the provided 'rpmReading' array

  // Read Barometers/Temp
  currentAltitude = soar_barometer.get_altitude() - flightData.groundLevel; // - flightData.groundLevel;  // Relative altitude
  if (!soar_barometer.isReady() || isnan(currentAltitude))
  {
    currentAltitude = soar_bme.getAltitude() - flightData.bmeGroundLevel; // - flightData.bmeGroundLevel;
  }
  // bmeAltitude = soar_bme.getAltitude();             // Relative altitude

  currentTemp = soar_bme.getTemperature();
  if (!soar_bme.isReady() || isnan(currentTemp))
  {
    currentTemp = soar_barometer.get_temperature();
  }

  currentPressure = soar_bme.getPressure();
  if (!soar_bme.isReady() || isnan(currentPressure))
  {
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
  currentVelocityMag = abs(velocity[0]); // Assuming velocity[0] holds the primary magnitude/component

  // --- Safety Check for NaN/Inf ---
  // It's good practice to check sensor readings, especially after calculations
  if (isnan(currentGForce) || isinf(currentGForce))
  {
    Serial.println("Warning: NaN/Inf detected in GForce calculation!");
    currentGForce = 1.0; // Provide a default safe value
  }
  if (isnan(currentAltitude) || isinf(currentAltitude))
  {
    Serial.println("Warning: NaN/Inf detected in BMP Altitude!");
    currentAltitude = 0.0;
  }
  // Add similar checks for other critical values if needed
}

void readSimulation(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag)
{

  // Read IM

  getSimulatedAcceleration(acceleration[0], acceleration[1], acceleration[2]);
  getSimulatedLinearAcceleration(linearAccel[0], linearAccel[1], linearAccel[2]);
  getSimulatedGravity(gravity[0], gravity[1], gravity[2]);
  velocity[0] = getVerticalVelocity();
  rpmReading[0] = 10.0;
  rpmReading[1] = 10.0;
  rpmReading[2] = 10.0;

  // Read Barometers/Temp
  currentAltitude = getSimulatedAltitude();
  // bmeAltitude = soar_bme.getAltitude();             // Relative altitude

  currentTemp = getSimulatedTemperature();

  currentPressure = getSimulatedPressure();
  // currentTempBmp = soar_barometer.get_temperature();
  // currentTempBme = soar_bme.getTemperature();
  // currentPressureBmp = soar_barometer.get_pressure();
  // currentPressureBme = soar_bme.getPressure();

  // Read Microphone
  currentDecibels = 10;

  // Calculate derived values
  currentGForce = sqrt(acceleration[0] * acceleration[0] + acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 9.81;
  currentVelocityMag = abs(velocity[0]); // Assuming velocity[0] holds the primary magnitude/component

  // --- Safety Check for NaN/Inf ---
  // It's good practice to check sensor readings, especially after calculations
  if (isnan(currentGForce) || isinf(currentGForce))
  {
    Serial.println("Warning: NaN/Inf detected in GForce calculation!");
    currentGForce = 1.0; // Provide a default safe value
  }
  if (isnan(currentAltitude) || isinf(currentAltitude))
  {
    Serial.println("Warning: NaN/Inf detected in BMP Altitude!");
    currentAltitude = 0.0;
  }
}

// ------ EMA Update Function ------
/**
 * @brief Updates the Exponential Moving Averages (EMAs) for key flight parameters.
 * Requires emaInitialized == true.
 * @param currentGForce Latest raw G-Force reading.
 * @param currentZAccel Latest raw Z-axis linear acceleration reading.
 * @param currentAltitude Latest raw relative altitude reading.
 * @param currentVelocityMag Latest raw velocity magnitude reading.
 */
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag)
{
  flightData.emaGForce = (currentGForce * ALPHA_GFORCE) + (flightData.emaGForce * (1.0 - ALPHA_GFORCE));
  emaZAccel = (currentZAccel * ALPHA_ZACCEL) + (emaZAccel * (1.0 - ALPHA_ZACCEL));
  flightData.emaAltitude = (currentAltitude * ALPHA_ALTITUDE) + (flightData.emaAltitude * (1.0 - ALPHA_ALTITUDE));
  flightData.emaVelocity = (currentVelocityMag * ALPHA_VELOCITY) + (flightData.emaVelocity * (1.0 - ALPHA_VELOCITY));
}

// ------ Flight Extremes Update Function ------
/**
 * @brief Updates the min/max values recorded during flight based on current readings.
 * @param currentGForce Latest G-force reading.
 * @param currentRpm Latest RPM reading.
 * @param currentTempBmp Latest BMP temperature reading.
 * @param currentPressureBmp Latest BMP pressure reading.
 * @param currentDecibels Latest sound level reading.
 * @param imuVelocityMag Latest IMU velocity magnitude reading.
 */
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag)
{
  // Update Max G-Force
  if (currentGForce > flightData.maxGForce)
    flightData.maxGForce = currentGForce;

  // Update Max Absolute RPM
  if (currentRpm > flightData.maxRPM)
    flightData.maxRPM = currentRpm;
  updateMaxSustainedRPM(currentRpm); // Update max sustained RPM

  if (currentAltitude > flightData.apogee)
    flightData.apogee = currentAltitude;

  // Update Temp Min/Max (using BMP temp)
  if (currentTemp < flightData.minTemperature)
    flightData.minTemperature = currentTemp;
  if (currentTemp > flightData.maxTemperature)
    flightData.maxTemperature = currentTemp;

  // Update Pressure Min/Max (using BMP pressure)
  if (currentPressure < flightData.minPressure)
    flightData.minPressure = currentPressure;
  if (currentPressure > flightData.maxPressure)
    flightData.maxPressure = currentPressure;

  // Update Max Decibels
  if (currentDecibels > flightData.maxDecibels)
    flightData.maxDecibels = currentDecibels;

  // Update Max Velocity (using magnitude of IMU velocity reading)
  if (imuVelocityMag > flightData.maxVelocity)
    flightData.maxVelocity = imuVelocityMag;
}

// ------ Sustained RPM Update Function ------
/**
 * @brief Updates the calculation for maximum sustained RPM over a defined window.
 * Modifies global variables: rpmWindow, rpmWindowIndex, rpmSampleCount, maxSustainedRpm.
 * @param currentRpm The latest RPM reading.
 */
void updateMaxSustainedRPM(float currentRpm)
{
  // Add current RPM to the circular buffer
  rpmWindow[rpmWindowIndex] = currentRpm;
  rpmWindowIndex = (rpmWindowIndex + 1) % RPM_WINDOW_SIZE;
  // Increment sample count until buffer is full
  if (rpmSampleCount < RPM_WINDOW_SIZE)
  {
    rpmSampleCount++;
  }

  // Once the buffer is full (we have data for the full interval)
  if (rpmSampleCount >= RPM_WINDOW_SIZE)
  {
    // Find the minimum RPM within the current window
    float minRpmInWindow = rpmWindow[0];
    for (int i = 1; i < RPM_WINDOW_SIZE; i++)
    {
      if (rpmWindow[i] < minRpmInWindow)
      {
        minRpmInWindow = rpmWindow[i];
      }
    }

    // Update the overall maximum sustained RPM *during the flight*
    // if the current window's minimum is higher
    if (minRpmInWindow > maxSustainedRpm)
    {
      maxSustainedRpm = minRpmInWindow;
    }
  }
}

// ------ Landing Sequence Handler ------ RESTORED
/**
 * @brief Executes the sequence of actions required upon landing detection.
 * Calculates flight time, saves data, displays summary, triggers servos.
 * @param finalVelocityReading The velocity reading captured just before landing detection.
 * @param finalGravityReading The gravity vector captured just before landing detection.
 */
void handleLandingSequence(float finalGravityReading[3])
{
  flightData.flightTime = (millis() - flightStartTime); // Store flight time in milliseconds
  // landingVelocity = finalVelocityReading;                // Record final IMU velocity reading
  flightData.maxSusRPM = maxSustainedRpm; // Store the max sustained RPM for this completed flight
  // flightData.currentState = 3.0;                      // Set state to LANDED

  onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 50)); // Blue for LANDED
  onboardLed.show();
  delay(2000);

  // Trigger payload servo logic

  Serial.println("Finalizing log file...");
  if (!sd_card.syncLogFile())
  { // Sync remaining buffered data
    Serial.println("Warning: Final sync failed before closing!");
  }
  sd_card.closeLogFile(); // Close the main log file

  delay(1000);

  door = p_serv.servoLogic(pwm, finalGravityReading);

  if (SIMULATION)
  {
    setSimulatedServo(door, 44);
  }

  if (SIMULATION)
  {
    currentTemp = getSimulatedTemperature(); // Get simulated temperature for display
  }
  else
  {
    currentTemp = soar_bme.getTemperature();
    if (!soar_bme.isReady() || isnan(currentTemp))
    {
      currentTemp = soar_barometer.get_temperature(); // Fallback to BMP sensor if BME fails
    }
  }

  float euler[3];
  soar_imu.GET_EULER(euler);

  pitch = euler[0];
  roll = euler[1];

  saveFlightSummaryToSD(); // Saves summary text file to SD card
  Serial.println(summary);

  // char sumXbee = summary.c_str();
  // xbee.send(summary);  // Send summary to XBee (or other radio)

  // Display final flight data to Serial
  displayFlightData();

  // Radio / XBee transmission (keep as is) ...
  delay(1000);

  broadcast_string = "E KQ4FYU Temp = " + String(((currentTemp * 9.0 / 5.0) + 32.0), 2) + " F, " + "Apogee = " + String((flightData.apogee * 3.28084), 2) + " ft, " + "Orientation = " + "Pitch: " + String(pitch) + "°; Roll: " + String(roll) + "Landing Time = " + String((flightData.flightTime) / 1000, 2) + " s, " + "Max velocity = " + String((flightData.maxVelocity * 3.28084), 2) + " ft/s, " + "Landing Velocity = " + String((landingVelocity * 3.28084), 2) + " ft/s, " + "Landing GForce = " + String(landingGForce, 2) + " G, " + "Survivable = " + String(calculateSurvivability() ? "True" : "False");
  std::string std_broadcast_string = broadcast_string.c_str();
  Serial.println(std_broadcast_string.c_str());

  for (int i = 0; i < 3; i++) {      
    playMorse(std_broadcast_string, 50); // Play fast
  }

//playMorse(std_broadcast_string, 50); // Play fast

Serial.println(" ");
Serial.println("radio done");

// Prepare for next potential flight (increment SD flight number)
sd_flight_number++;
sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number); // Prepare filename for next potential flight
Serial.print("Prepared for next SD flight number: ");
Serial.println(sd_flight_number);
Serial.print("Next log filename: ");
Serial.println(current_filename);

// Do NOT reset runtime variables here, wait for 'R' command or power cycle
resetEEPROM();
}

// ------ SD Card Functions ------
/**
 * @brief Opens a new log file on the SD card, writes the header, and syncs.
 * @param filename The full path and filename for the log file.
 * @return True if the file was opened and header written successfully, False otherwise.
 */
bool startNewLogFile(const char *filename)
{
  Serial.print("Attempting to start new log file: ");
  Serial.println(filename);

  if (sd_card.openLogFile(filename))
  {
    String header = "Time(ms),Alt_BMP(m),Alt_BME(m),Temp_BMP(C),Temp_BME(C),RPM,Accel_Z(m/s^2),G-Force,Vel_IMU(m/s),dB,Press_BME(Pa),Press_BMP(Pa)\n";
    if (sd_card.logToFile(header.c_str()))
    {
      // Sync immediately after writing header to ensure it's saved
      if (!sd_card.syncLogFile())
      {
        Serial.println("Error syncing header to SD card!");
        sd_card.closeLogFile(); // Close the file if sync failed
        return false;
      }
      else
      {
        Serial.println("Log file opened and header written successfully.");
        return true;
      }
    }
    else
    {
      Serial.println("Error writing header to SD card buffer!");
      sd_card.closeLogFile(); // Close file if header write failed
      return false;
    }
  }
  else
  {
    Serial.println("Error opening log file!");
    return false;
  }
}

/**
 * @brief Logs a single line of flight data to the SD card buffer.
 * Uses snprintf for safe formatting and sd_card.logToFile for buffered writing.
 */
void logFlightDataToSD(unsigned long timestamp, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure)
{

  // --- Use snprintf for safer and potentially faster string formatting ---
  char dataBuffer[256]; // Adjust size as needed, make sure it's large enough!
  int writtenChars = snprintf(dataBuffer, sizeof(dataBuffer),
                              "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.2f\n",
                              timestamp,        // Time(ms) since launch
                              currentAltitude,  // Alt_BMP(m)
                              currentTemp,      // Temp_BMP(C)
                              currentRpm,       // RPM
                              currentZAccel,    // Accel Z (m/s^2)
                              currentGForce,    // G-Force
                              currentVelocity,  // Vel_IMU(m/s)
                              currentDecibels,  // dB
                              currentPressure); // Press_BME(Pa)

  // Check for truncation or encoding errors
  if (writtenChars < 0 || writtenChars >= sizeof(dataBuffer))
  {
    Serial.println("Error: Data formatting failed or buffer too small in logFlightDataToSD!");
    // Handle error appropriately - maybe try logging an error message instead?
    sd_card.logToFile("FORMAT_ERROR\n"); // Log simple error if possible
    return;                              // Don't log potentially corrupt data
  }

  // --- Use the logToFile method (writes to SD card object's buffer) ---
  if (!sd_card.logToFile(dataBuffer))
  {
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
void saveFlightSummaryToSD()
{
  // Use sd_flight_number - 1 because it was incremented *after* landing detection
  // before this function is called.
  int flightNumToSave = sd_flight_number - 1;
  if (flightNumToSave < 0)
    flightNumToSave = 0; // Safety check if called incorrectly

  char summary_filename[50];
  sprintf(summary_filename, "/flight_data/summary_%d.txt", flightNumToSave);

  Serial.print("Saving summary to: ");
  Serial.println(summary_filename);

  // Use String concatenation (acceptable for infrequent summary save)
  summary = "Flight " + String(flightNumToSave) + " Summary\n";
  summary += "--------------------\n";
  summary += "flightData.apogee (BMP/EMA): " + String(flightData.apogee, 2) + " m\n"; // flightData.apogee tracked using EMA of BMP
  summary += "Max Velocity (IMU): " + String(maxVelocity, 2) + " m/s\n";              // Based on IMU reading magnitude
  summary += "Max G-Force: " + String(flightData.maxGForce, 2) + " G\n";
  summary += "Flight Time: " + String(flightData.flightTime / 1000.0, 2) + " s\n"; // Convert ms to s
  summary += "Landing Velocity (IMU): " + String(landingVelocity, 2) + " m/s\n";   // IMU reading at landing detection
  summary += "Max Absolute RPM: " + String(maxRPM, 1) + "\n";
  summary += "Max Sustained RPM (" + String(RPM_WINDOW_SIZE * (100.0 / 1000.0), 1) + "s window): " + String(flightData.maxSusRPM, 1) + " RPM\n"; // Show calculated window size
  summary += "Max Decibels: " + String(flightData.maxDecibels, 1) + " dB\n";
  summary += "Min Temperature (BMP): " + String(flightData.minTemperature, 1) + " C\n";
  summary += "Max Temperature (BMP): " + String(flightData.maxTemperature, 1) + " C\n";
  summary += "Min Pressure (BMP): " + String(flightData.minPressure / 100.0, 2) + " hPa\n"; // Convert Pa to hPa
  summary += "Max Pressure (BMP): " + String(flightData.maxPressure / 100.0, 2) + " hPa\n"; // Convert Pa to hPa
  summary += "Servo Door: " + String(door) + " \n";                                         // Convert Pa to hPa
  summary += "--------------------\n";

  // Use the simple writeFile method from the SD card library for the summary
  if (!sd_card.writeFile(summary_filename, summary.c_str()))
  {
    Serial.println("Error saving summary file!");
  }
  else
  {
    Serial.println("Summary file saved.");
  }
}

// ------ Flight Data Reset Function ------

// ------ Servo Reset Function ------
/**
 * @brief Sets all defined payload servos to their default (0 degree) position.
 */
void resetServos()
{
  p_serv.setServoAngle(pwm, 8, 0);
  p_serv.setServoAngle(pwm, 12, 0);
  p_serv.setServoAngle(pwm, 0, 0);
  p_serv.setServoAngle(pwm, 4, 0);
}

//   Serial.println("Capturing ground level altitude...");
//   // Allow some time for sensors to stabilize if needed, or take multiple readings

//   soar_barometer.get_altitude();
//   soar_bme.getAltitude();

//   delay(500);  // Short delay
//   float totalBmpAlt = 0;
//   float totalBmeAlt = 0;
//   const int readings = 5;

//   delay(2000);
//   float flightData.groundLevel = soar_barometer.get_altitude();
//   float flightData.bmeGroundLevel = soar_bme.getAltitude();

//   // for (int i = 0; i < readings; ++i) {
//   //   totalBmpAlt += soar_barometer.get_altitude();
//   //   totalBmeAlt += soar_bme.getAltitude();
//   //   delay(50);  // Small delay between readings
//   // }
//   // flightData.groundLevel = totalBmpAlt / readings;
//   // flightData.bmeGroundLevel = totalBmeAlt / readings;

//   Serial.print("BMP Ground Level Set: ");
//   Serial.println(flightData.groundLevel);
//   Serial.print("BME Ground Level Set: ");
//   Serial.println(flightData.bmeGroundLevel);
// }

// ------ EMA Initialization Function ------
/**
 * @brief Initializes the Exponential Moving Average (EMA) filters with initial sensor readings.
 * Reads sensors, calculates initial values, and sets the emaInitialized flag.
 */
void initializeEMAs()
{
  Serial.println("Initializing EMAs...");
  // Get initial readings
  float initialAccel[3], initialLinAccel[3], initialVel[1];
  soar_imu.GET_ACCELERATION(initialAccel);
  soar_imu.GET_LINEARACCEL(initialLinAccel);
  soar_imu.GET_VELOCITY(initialVel);

  // Calculate initial G-Force
  float initialGForce = sqrt(pow(initialAccel[0], 2) + pow(initialAccel[1], 2) + pow(initialAccel[2], 2)) / 9.81;
  // Safety check for NaN/Inf, common at startup if sensor init is slow
  if (isnan(initialGForce) || isinf(initialGForce) || initialGForce < 0.1)
  {
    initialGForce = 1.0; // Assume 1G if reading is invalid
    Serial.println("Warning: Invalid initial G-force reading, defaulting to 1.0");
  }

  // Initialize EMA variables with first valid readings
  // Note: Use relative altitude which is 0 at ground level.
  flightData.emaAltitude = 0.0;
  flightData.emaGForce = initialGForce;
  emaZAccel = initialLinAccel[2];              // Z-axis linear acceleration
  flightData.emaVelocity = abs(initialVel[0]); // Use magnitude
  emaInitialized = true;                       // Mark EMAs as initialized

  Serial.println("EMAs Initialized.");
}

/**
 * @brief Reads and displays summary data for all flight slots stored in .
 * Uses old offset structure and sizeof(float).
 */
// void stats() {

//     // Check if slot contains valid data (non-zero landing time AND state is LANDED)
//     // Or maybe just non-zero time is enough for basic validity check in old system
//     if (readLandingTime > 0) {
//       validFlights++;
//       Serial.print("\n--- FLIGHT SLOT "); Serial.print(i); Serial.println(" ---");
//       Serial.print("  State: "); Serial.println(readState); // Display state

//       // Calculate survivability for this flight (using read values)
//       bool survivable = (readMinTemp >= 10.0 && readMaxTemp <= 35.0 && readMaxRPM <= 60.0 && (readflightData.minPressure / 101325.0) >= 0.0618 && (readflightData.maxPressure / 101325.0) <= 100.0 && readflightData.maxGForce <= 5.0 && readflightData.maxDecibels <= 150.0);

//       // Display all flight metrics
//       Serial.print("  Last Recorded Temp: "); Serial.print(readLandingTemp, 1); Serial.println(" C"); // Temp at last save
//       Serial.print("  flightData.apogee: "); Serial.print(readflightData.apogee, 2); Serial.println(" m");
//       Serial.print("  Flight Time (at last save): "); Serial.print(readLandingTime / 1000.0, 2); Serial.println(" s");
//       Serial.print("  Last Orientation (P,R): "); Serial.print(readLandingPitch, 1); Serial.print(", "); Serial.print(readLandingRoll, 1); Serial.println(" deg");
//       Serial.print("  Last Door State: "); Serial.println(readOpenedDoor);
//       Serial.print("  Max Velocity: "); Serial.print(readMaxVelocity, 2); Serial.println(" m/s");
//       Serial.print("  Landing Velocity (if landed): "); Serial.print(readLandingVelocity, 2); Serial.println(" m/s");
//       Serial.print("  Max G-Force: "); Serial.print(readflightData.maxGForce, 2); Serial.println(" G");
//       Serial.print("  Temp Range: "); Serial.print(readMinTemp, 1); Serial.print(" to "); Serial.print(readMaxTemp, 1); Serial.println(" C");
//       Serial.print("  Max RPM (Abs): "); Serial.print(readMaxRPM, 1); Serial.println(" RPM");
//       Serial.print("  Pressure Range: "); Serial.print(readflightData.minPressure / 100.0, 2); Serial.print(" to "); Serial.print(readflightData.maxPressure / 100.0, 2); Serial.println(" hPa");
//       Serial.print("  Max Noise: "); Serial.print(readflightData.maxDecibels, 1); Serial.println(" dB");
//       Serial.print("  Survivability (based on extremes): "); Serial.println(survivable ? "PASS" : "FAIL");
//       Serial.println("-------------------");
//     } else {
//       // Optional: Print that the slot is empty or appears unused
//       // Serial.print("\n--- FLIGHT SLOT "); Serial.print(i); Serial.println(" (EMPTY) ---");
//     }
// }

/**
 * @brief Displays the summary data for the *just completed* flight from global variables.
 * Uses old structure.
 */
void displayFlightData()
{
  Serial.println("\n--- CURRENT FLIGHT SUMMARY ---");
  float currentTemp = soar_bme.getTemperature(); // Get current temp
  if (!soar_bme.isReady() || isnan(currentTemp))
  {
    currentTemp = soar_barometer.get_temperature();
  }
  Serial.print("Landing Temperature: ");
  Serial.print(currentTemp, 1);
  Serial.println(" C");

  Serial.print("flightData.apogee: ");
  Serial.print(flightData.apogee, 2);
  Serial.println(" m");
  Serial.print("Flight Time: T+");
  Serial.print(flightData.flightTime / 1000.0, 2);
  Serial.println(" s"); // Use final flightData.flightTime

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
  Serial.print(flightData.maxVelocity, 2);
  Serial.println(" m/s");
  Serial.print("Landing Velocity: ");
  Serial.print(landingVelocity, 2);
  Serial.println(" m/s");
  Serial.print("Max G-Force: ");
  Serial.print(flightData.maxGForce, 2);
  Serial.println(" G");
  Serial.print("Max RPM (Abs): ");
  Serial.print(flightData.maxRPM, 1);
  Serial.println(" RPM");
  Serial.print("Max RPM (Sust): ");
  Serial.print(flightData.maxSusRPM, 1);
  Serial.println(" RPM"); // Use the final value
  Serial.print("Max Decibels: ");
  Serial.print(flightData.maxDecibels, 1);
  Serial.println(" dB");
  Serial.print("Temp Range: ");
  Serial.print(flightData.minTemperature, 1);
  Serial.print(" to ");
  Serial.print(flightData.maxTemperature, 1);
  Serial.println(" C");
  Serial.print("Pressure Range: ");
  Serial.print(flightData.minPressure / 100.0, 2);
  Serial.print(" to ");
  Serial.print(flightData.maxPressure / 100.0, 2);
  Serial.println(" hPa");

  Serial.print("Survivability: ");
  Serial.println(calculateSurvivability() ? "PASS" : "FAIL");
  Serial.println("------------------------------------------");
}

/**
 * @brief Calculates survivability based on recorded flight extremes.
 */
bool calculateSurvivability()
{
  // Use the global variables holding the completed flight's data
  return (flightData.minTemperature >= 10.0 && flightData.maxTemperature <= 35.0 && flightData.maxSusRPM <= 60.0 && // Check sustained RPM TODO:<10 rpm
          (flightData.minPressure / 101325.0) >= 0.0618 &&                                                          // Convert Pa to atm
          (flightData.maxPressure / 101325.0) <= 100.0 &&                                                           // Convert Pa to atm
          flightData.maxGForce <= 5.0 && flightData.maxDecibels <= 150.0 && pitch >= -10.0 && pitch <= 10.0);
}

// void missionRequirements() {

// }

// --- REMOVED Active State Functions ---
// void saveActiveFlightState() { /* ... REMOVED ... */ }
// bool restoreActiveFlightState() { /* ... REMOVED ... */ return false; }

// --- DAC & Morse Code Functions ---
/**
 * @brief Sets the DAC output to the specified value.
 * @param value The 16-bit value to set the DAC output to.
 * @return True if the operation was successful, false otherwise.
 */
bool setDacOutput(uint16_t value)
{
  Wire.beginTransmission(DAC_I2C_ADDR);
  Wire.write(DAC_CMD_WRITE_UPDATE); // Send Command Byte
  Wire.write((value >> 8) & 0xFF);  // Send High Byte of Data
  Wire.write(value & 0xFF);         // Send Low Byte of Data
  byte error = Wire.endTransmission();

  if (error != 0)
  {
    Serial.print("I2C Error writing to DAC: ");
    Serial.println(error);
    return false;
  }
  return true;
}

/**
 * @brief Modulates the Morse code signal for the specified duration.
 * @param duration The duration of the Morse code signal in microseconds.
 */
void modulateMorse(int duration)
{
  pinMode(4, OUTPUT);
  analogWriteFrequency(4, 440000);
  int amplitude = 32767; // Peak amplitude (0-255)
  int offset = 32767;    // Center value (~2.5V)
  long waveStart = micros();
  float angle = 0;

  while (micros() - waveStart < duration)
  {
    angle = 2.0 * PI * 600 * (micros() - waveStart) / 1000000.0;
    if (angle > 2 * PI)
      angle = angle - 2 * PI;
    int dacValue = (int)(offset + amplitude * sin(angle));
    setDacOutput(dacValue);

    // delayMicroseconds(10);
  }
  return;
}

/**
 * @brief Plays the Morse code for the specified message.
 * @param message The message to be played in Morse code.
 */
void playMorse(std::string message, int time_unit)
{
  // Serial.println("HI THIS SHOULD BE PRINTING IF THE FUNCTION WORKS");
  digitalWrite(PTTPIN, LOW); // Start transmit
  for (int i = 0; i < message.length(); i++)
  {
    char character = toupper(message[i]);

    if (character == ' ')
    {
      delay(time_unit * 7); // Word gap
      continue;
    }

    auto iterator = morseMap.find(character); // Iterator's like auto, apparently.
    if (iterator != morseMap.end())
    {
      std::string morse_code = iterator->second;
      Serial.print(morse_code.c_str()); // test print

      for (int j = 0; j < morse_code.length(); j++)
      {
        if (morse_code[j] == '.')
        {
          generateTone(time_unit); // Dot
        }
        else if (morse_code[j] == '-')
        {
          generateTone(time_unit * 3); // Dash
        }
        delay(time_unit); // Gap between symbols
      }

      delay(time_unit * 2); // Already had 1-time_unit pause after symbol; add 2 more to make 3-time_unit letter space
    }
  }
  delay(time_unit * 5);
  digitalWrite(PTTPIN, HIGH);
  delay(time_unit * 5);
}

void generateTone(int duration)
{
  int amplitude = 32767; // Peak amplitude (0-255)
  int offset = 32767;    // Center value (~2.5V)
  float angle = 0;

  long waveStart = millis();
  while (millis() - waveStart < duration)
  {
    angle = 2.0 * PI * 600 * (micros() - waveStart) / 1000000.0;
    if (angle > 2 * PI)
      angle = angle - 2 * PI;
    int dacValue = (int)(offset + amplitude * sin(angle));
    setDacOutput(dacValue);
  }
  setDacOutput(0);

  return;
}

void saveEEPROM()
{
  flightData.magic = MAGIC_NUMBER;
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.put(addr, flightData);
  EEPROM.put(SLOT_SAVE_ADDRESS, currentSlot);
  EEPROM.commit();
  Serial.println("→ [EEPROM] Saved.");
}

void restoreEEPROM()
{
  // NOTE: assumes currentSlot already restored
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.get(addr, flightData);
  Serial.println("→ [EEPROM] Data loaded from slot " + String(currentSlot));
}

void displaySlots()
{
  Serial.println("[EEPROM] Dumping all slots:");
  for (int s = 0; s < SLOT_COUNT; s++)
  {
    FlightData tmp;
    int addr = BASE_ADDRESS + (s * sizeof(FlightData));
    EEPROM.get(addr, tmp);
    Serial.println(" Slot " + String(s) + ":");
    displayEEPROM(tmp);
  }
}

void displayEEPROM(const FlightData &var)
{
  Serial.print("  Magic: 0x");
  Serial.println(var.magic, HEX);
  Serial.print("Current Slot: ");
  Serial.println(currentSlot);
  Serial.print("State: ");
  Serial.println(var.currentState);
  Serial.print("Flight Time: ");
  Serial.println(var.flightTime);
  Serial.print("flightData.apogee: ");
  Serial.println(var.apogee);
  Serial.print("Max G-Force: ");
  Serial.println(var.maxGForce);
  Serial.print("Max Sus RPM: ");
  Serial.println(var.maxSusRPM);
  Serial.print("Max Decibels: ");
  Serial.println(var.maxDecibels);
  Serial.print("Min Temperature: ");
  Serial.println(var.minTemperature);
  Serial.print("Max Temperature: ");
  Serial.println(var.maxTemperature);
  Serial.print("Min Pressure: ");
  Serial.println(var.minPressure);
  Serial.print("Max Pressure: ");
  Serial.println(var.maxPressure);
  Serial.print("EMA GForce: ");
  Serial.println(var.emaGForce);
  Serial.print("EMA Altitude: ");
  Serial.println(var.emaAltitude);
  Serial.print("EMA Velocity: ");
  Serial.println(var.emaVelocity);
}

void resetEEPROM()
{
  Serial.println("[EEPROM] Resetting all used bytes...");
  // Erase flightData slots
  for (int i = 0; i < SLOT_COUNT * sizeof(FlightData); i++)
  {
    EEPROM.write(BASE_ADDRESS + i, 0xFF);
  }
  // Erase saved slot index
  for (int i = 0; i < sizeof(int); i++)
  {
    EEPROM.write(SLOT_SAVE_ADDRESS + i, 0xFF);
  }
  EEPROM.commit();
  Serial.println("[EEPROM] Erase complete.");
}
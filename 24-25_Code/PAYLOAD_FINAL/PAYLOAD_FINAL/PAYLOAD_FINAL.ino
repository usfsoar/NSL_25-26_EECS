/*
WELCOME TO THE SOAR PAYLOAD CODE!

things to do:
a) check rpm sustained logic - ctrl f maxSustainedRPM

b) sama - for xbee - ctrl f xbee. some transmission in setup and functions is nessesary
i need to know whether we can transmit every iteration or only when state changes.
right now preference is every iteration every 2-3 seconds
also according to the class method, XBeeSender xbee(XBeeSerial, destinationAddress), first parameter takes HardwareSerial, but we are using SoftwareSerial. I think it should be SoftwareSerial, but check with Jay.

c) thoroughly test the transmission and surviviability logic and check if imperial to freedom units conversion is correct.

d) how to simulation
1. go to _.config.h, and put SIMULATION and SIMULATION_OUT to 1.
2. upload code to esp
3. open the simulation. - if you do not have it, its on github called payload_digital_twing or somethign like that
4. above the button begin payload serial, you need to put the port number of the esp
5. click begin payload serial
6. you will see the serial monitor now.
7. click zero errors then launch.
notes-simulation does not have microphone and gyroscope, but i made a calculation for it without decibels and rpm, so dont worry.

e) jay was not able to change the reciveing frequency, so if we get a new frequency, lowkey cooked. maybe checked dra gpio ports again
f) make sure to edit the checklist i sent on discord.
https://usfedu-my.sharepoint.com/:w:/g/personal/samueljohnson_usf_edu/ERLZ51Ec43tAvDBu8DvY1W8BPR7CNPkP8hGmbqgRk8UMHQ?e=cw6ZyM

g) ready = 0.0 = purple
   launched = 1.0 = red
   descending = 2.0 = green
   landed = 3.0 = blue

h) good luch! -sam
*/


// libraries and header files
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
#include <cmath>
#include <vector>
#include <esp32-hal-ledc.h>
#include <esp32-hal-gpio.h>
#include <DRA818.h>  //DRA818V
#include <iostream>
#include <iomanip>
#include <string>
#include <HardwareSerial.h>
#include <map>
#include "XBeeSender.h"      //xbee
#include <SoftwareSerial.h>  //xbee
#include "DIGITAL_TWIN.h"    //sim
#include "_config.h"         //sim


/*eeprom set values
EEPROM bytes = SLOT_COUNT * sizeof(FlightData) + sizeof(currentSlot) = 3 * 68 + 4 = 220 bytes -> 256 bytes
*/
#define EEPROM_SIZE 256
#define FLOATS_PER_SLOT 17
#define SLOT_COUNT 3
#define SLOT_SIZE (FLOATS_PER_SLOT * sizeof(float))  // 17 floats * 4 bytes = 68 bytes
#define BASE_ADDRESS 0
#define SLOT_SAVE_ADDRESS (256 - sizeof(int))  // Where currentSlot is saved
#define MAGIC_NUMBER 0xA5A5
int currentSlot = 0;  // Index (0 to SLOT_COUNTS-1) for the *next* flight slot in

//dra set values
#define PTTPIN 8
#define DRA818_SERIAL Serial1  // Define the serial interface for the DRA818V
#define DRA818_PD 7
#define DRA818_PL 45

// DRA818V configuration parameters
#define DRA818_TYPE DRA818_VHF  // Module type: VHF
#define RX_FREQUENCY 146.00     //144.360    // Receive frequency in MHz
#define TX_FREQUENCY 146.00     //144.360    // Transmit frequency in MHz
#define SQUELCH_LEVEL 4         // Squelch level (0-8)
#define VOLUME_LEVEL 8          // Volume level (0-8)
#define CTCSS_RX 0              // CTCSS receive code (0 if not used)
#define CTCSS_TX 0              // CTCSS transmit code (0 if not used)
#define BANDWIDTH DRA818_12K5   // Bandwidth: 12.5 kHz
#define PREEMPHASIS true        // Pre-emphasis enabled
#define HIGH_PASS true          // High-pass filter enabled
#define LOW_PASS true           // Low-pass filter enabled

//control pins
const int DAC_LDAC_PIN = 9;
const int DAC_A0_PIN = 10;

// --- I2C Address ---
// Set based on how you drive DAC_A0_PIN
// 0x4C if A0 is LOW, 0x4D if A0 is HIGH
const int DAC_I2C_ADDR = 0x4C;

// AD5693 Command Byte (VERIFY FROM DATASHEET!)
const uint8_t DAC_CMD_WRITE_UPDATE = 0x30;  // Assumed command

//morse code translation
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
DRA818 *dra;

// xbee
//  SoftwareSerial XBeeSerial(4, 3);                                       //SoftwareSerial for Waveshare S3 ESP32
//  uint8_t destinationAddress[8] = { 0x00, 0x13, 0xA2, 0x00, 0x41, 0x06, 0x7F, 0x53 };     //Check this address with the RECEIVING XBee before launch
//  XBeeSender xbee(XBeeSerial, destinationAddress);                        //XBee Initialization

//function prototypes
//data functions
void readAllSensors(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag);  //float linearAccel[3],
void readSimulation(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag);
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag);
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag);
void updateMaxSustainedRPM(float currentRpm);

//miscellaneous functions
void handleLandingSequence(const float finalGravityReading[3]);
void resetServos();
bool calculateSurvivability();
void checkReady();

//sd card functions
bool startNewLogFile(const char *filename);
void initializeEMAs();
void displayFlightData();  // Display summary of *just completed* flight
void logFlightDataToSD(unsigned long timestamp, float currentState, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure);  // Modified log function
void saveFlightSummaryToSD();                   // Save summary to SD Card

//radio functions
void playMorse(std::string message, int time_unit);
void generateTone(int duration);
bool setDacOutput(uint16_t value);


//data stored to eeprom
//magic number is there to check if the data is corrupted
struct FlightData {
  uint16_t magic = MAGIC_NUMBER;
  float emaGForce = 1.0;
  float emaAltitude = 0.0;
  float emaVelocity = 0.0;

  float groundLevel = 0.0;     // BMP ground level altitude
  float bmeGroundLevel = 0.0;  // BME ground level altitude

  float apogee = 0.0;
  unsigned long flightTime = 0;  // milliseconds
  float maxGForce = -INFINITY;
  float maxDecibels = -INFINITY;
  float minTemperature = INFINITY;   // Initialize to high value
  float maxTemperature = -INFINITY;  // Initialize to low value
  float minPressure = INFINITY;
  float maxPressure = -INFINITY;
  float maxVelocity = -INFINITY;
  float maxRPM = -INFINITY;
  float maxSustainedRPM = -INFINITY;  // Max sustained RPM for the *completed* flight

  float currentState = 0.0;
};

//eeprom functions, struct goes above this for some reason
void saveEEPROM();
void restoreEEPROM();
void resetEEPROM();
void displaySlots();
void displayEEPROM(const FlightData &var);

//initalize struct
FlightData flightData;

// led
#define LED_PIN 21
#define NUM_LEDS 1
Adafruit_NeoPixel onboardLed = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// sd card variables
int sd_flight_number = 0;
char current_filename[40];


// FLIGHT CONSTANTS THRESHOLDS                      //drop test       //hand test
const float LAUNCH_ACCEL_THRESHOLD = 1.5;       //1.2             //1.2
const float LAUNCH_ALTITUDE_THRESHOLD = 10.0;   //5.0             //1.0
const float DESCENT_ALTITUDE_THRESHOLD = 10.0;  //3.0             //-0.3
const float DESCENT_APOGEE_THRESHOLD = 5.0;     //1.0             //1.0
const float LANDING_ACCEL_THRESHOLD = 0.2;      // > abs(gforce -1 )      //0.2
const float LANDING_VEL_THRESHOLD = 0.8;        //0.8
const float LANDING_ALTITUDE_THRESHOLD = 3.0;   //1.0

const int STABLE_READINGS_FOR_LAUNCH = 3;
const int STABLE_READINGS_FOR_DESCENT = 3;
const int STABLE_READINGS_FOR_LANDING = 10;
const int STABLE_READINGS_FOR_LANDING_VELOCITY_GFORCE = 3;

//ema alpha values
const float ALPHA_GFORCE = 0.8;
const float ALPHA_ZACCEL = 0.8;
const float ALPHA_ALTITUDE = 0.5;
const float ALPHA_VELOCITY = 0.8;


//global variables
//float emaZAccel = 0.0;
bool emaInitialized = false;
float landingVelocity = -INFINITY;
float landingGForce = -INFINITY;
float landingTemp = 0.0;
unsigned long flightStartTime = 0;
float maxVelocity = 0.0;
float maxRPM = 0.0;
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
String checkSensor = "Sensor Check: ";
bool draReady = false;
bool dacReady = false;
bool eepromReady = false;

//max sus rpm variables
const unsigned long RPM_LOG_INTERVAL_MS = 100;  // matches your log→SD cadence
const int RPM_WINDOW_SIZE = 1000 / RPM_LOG_INTERVAL_MS;
// 1000ms / 100ms = 10 samples → 1 s window
float rpmWindow[RPM_WINDOW_SIZE] = { 0.0 };  // Circular buffer for recent RPM readings
int rpmWindowIndex = 0;
int rpmSampleCount = 0;
float maxSustainedRPM = 0.0;

// SD Card Sync Interval
const unsigned long SD_SYNC_INTERVAL_MS = 1000;
static uint32_t lastSyncTime = 0;  // Ensure this is declared globally

//SETUP
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Rocket Flight Data Recorder - Waveshare ESP32-S3-Zero");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C Initialized on SDA=1, SCL=2");

  //initalize sensors, servo
  p_serv.initialize(pwm);
  soar_imu.BNO_SETUP();
  soar_barometer.Initialize();
  soar_bme.initialize();
  soar_sph.begin();

  //radio initialization
  //initialize the dra
  DRA818_SERIAL.begin(9600, SERIAL_8N1, 17, 18);  // RX on GPIO16, TX on GPIO17 (rate, mode, rx,tx)
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

  if (dra == nullptr) {
    draReady = false;
  } else {
    Serial.begin(115200);
    draReady = true;
  }

  Wire.setClock(400000);

  //Serial.println("AD5693 DAC Test Sketch (with LDAC/A0 control)");

  // --- Configure Control Pins ---
  pinMode(DAC_A0_PIN, OUTPUT);
  digitalWrite(DAC_A0_PIN, LOW);  // Set A0 LOW for I2C Address 0x4C
  //Serial.println("Set A0 Pin (GPIO " + String(DAC_A0_PIN) + ") LOW for Address 0x4C");

  pinMode(DAC_LDAC_PIN, OUTPUT);
  digitalWrite(DAC_LDAC_PIN, LOW);  // Hold LDAC LOW for immediate output updates
  //Serial.println("Set LDAC Pin (GPIO " + String(DAC_LDAC_PIN) + ") LOW");
  // --- End Control Pin Config ---

  pinMode(PTTPIN, OUTPUT);
  digitalWrite(PTTPIN, HIGH);
  pinMode(DRA818_PD, OUTPUT);
  digitalWrite(DRA818_PD, HIGH);
  pinMode(DRA818_PL, OUTPUT);
  digitalWrite(DRA818_PL, LOW);

  Wire.beginTransmission(DAC_I2C_ADDR);
  byte error = Wire.endTransmission();
  if (error == 0) {
    dacReady = true;
    //Serial.print("DAC found at address 0x");
    //Serial.println(DAC_I2C_ADDR, HEX);
  } else {
    dacReady = false;
    // Serial.print("DAC not found at address 0x");
    // Serial.println(DAC_I2C_ADDR, HEX);
    // Serial.println("Check wiring, A0 pin state, and I2C address. Halting.");
  }

  //initalize led
  onboardLed.begin();
  onboardLed.setBrightness(50);  //brightness 0-255, 50 good enough
  onboardLed.clear();            // turn off color
  onboardLed.show();             // turn on the led
  Serial.println("Onboard NeoPixel Initialized.");


  //eeprom initialization
  if (!EEPROM.begin(EEPROM_SIZE)) {
    eepromReady = false;
    // Serial.println("Failed to initialize EEPROM!");
  } else {
    eepromReady = true;
    // Serial.println("EEPROM Initialized.");

    // 1) Restore currentSlot
    EEPROM.get(SLOT_SAVE_ADDRESS, currentSlot);
    if (currentSlot < 0 || currentSlot >= SLOT_COUNT) {
      currentSlot = 0;
    }
    Serial.println("Startup: currentSlot = " + String(currentSlot));

    // 2) Load flightData
    int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
    EEPROM.get(addr, flightData);
    Serial.print("Startup: read magic = 0x");
    Serial.println(flightData.magic, HEX);

    // 3) If magic is wrong, *seed* slot 0 with defaults
    if (flightData.magic != MAGIC_NUMBER) {
      //xbee.send("payload disconnected, reconnected, magic mismatch, eeprom corrupted, resetting to defaults");
      flightData = FlightData();  // resets struct
      currentSlot = 0;
      saveEEPROM();
      displaySlots();
    } else {
      // 4) magic OK: now check currentState range for restore
      if (flightData.currentState == 1.0 || flightData.currentState == 2.0) {
        Serial.println("***Restoring Slot***");
        Serial.print("Current State: ");
        Serial.println(flightData.currentState);
        //xbee.send("payload disconnected, reconnected, currentState: %f", flightData.currentState);
        restoreEEPROM();
      } else {
        Serial.println("***No Restore***");
      }
    }

    displaySlots();
  }

  if (flightData.currentState == 0.0) {
    Serial.println("***No Restore***");
    resetEEPROM();

    //set ground level
    for (int i = 0; i < 3; i++) {
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
    //xbee.send("ground level: %f", flightData.groundLevel); //do not need bmeGroundLevel
  }



  //set up sd card file and led in ready state, if restored, continue logging
  if (flightData.currentState == 0.0) {
    onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 50));
    onboardLed.show();

    // Initialize SD Card and find next file number
    sd_card.begin();
    sd_card.createDir("/flight_data");
    sd_flight_number = 0;  // Start search from 0
    while (true) {
      sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
      if (!sd_card.exists(current_filename))
        break;
      sd_flight_number++;
      if (sd_flight_number > 9999) {
        Serial.println("Warning: SD flight number exceeded 9999!");
        sd_flight_number = 9999;  // Use max number
        sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
        break;  // Safety break
      }
    }
    Serial.print("Next SD file number: ");
    Serial.println(sd_flight_number);
    Serial.print("Log filename: ");
    Serial.println(current_filename);
    initializeEMAs();
  } else {
    Serial.println("Restored previous state. Performing minimal initialization...");

    // led based on restored state
    if (flightData.currentState == 1.0) {
      onboardLed.setPixelColor(0, onboardLed.Color(50, 0, 0));  // Red for LAUNCHED
    } else if (flightData.currentState == 2.0) {
      onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 0));  // Green for DESCENDING
    } else if (flightData.currentState == 3.0) {
      onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 50));  // Blue for LANDED
    }
    onboardLed.show();

    // Initialize SD card
    sd_card.begin();
    sd_card.createDir("/flight_data");  // Ensure directory exists

    // Determine SD flight number based on restored state
    // If LAUNCHED/DESCENDING, assume we continue the *last* flight's log.
    // This logic might need refinement depending on exact restore requirements.
    int lastFlightIndex = currentSlot - 1;
    if (lastFlightIndex < 0)
      lastFlightIndex = SLOT_COUNT - 1;
    sd_flight_number = lastFlightIndex;  // Assume continuing the log of the restored flight index

    if (flightData.currentState == 3.0) {
      sd_flight_number++;  // If landed, prepare for the next flight number
    }

    sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);
    Serial.print("Using SD flight number: ");
    Serial.println(sd_flight_number);
    Serial.print("Log filename: ");
    Serial.println(current_filename);

    // If LAUNCHED or DESCENDING, attempt to open the existing log file to append
    if (flightData.currentState == 1.0 || flightData.currentState == 2.0) {
      if (sd_card.exists(current_filename)) {
        Serial.print("Attempting to append to existing log file: ");
        Serial.println(current_filename);
        if (!sd_card.openLogFile(current_filename)) {
          Serial.println("CRITICAL: Failed to open existing log file for appending!");
          // Consider forcing state to LANDED or other error handling
          // flightData.currentState = LANDED;
        } else {
          sd_card.logToFile("\n--- RESTORED MID-FLIGHT ---\n");  // Add a marker
          lastSyncTime = millis();                               // Reset sync timer
        }
      } else {
        Serial.print("Warning: Log file for restored flight not found. Starting new file: ");
        Serial.println(current_filename);
        if (!startNewLogFile(current_filename)) {
          Serial.println("CRITICAL: Failed to start new log file after restore!");
          // flightData.currentState = LANDED;
        } else {
          lastSyncTime = millis();
        }
      }
    }

    initializeEMAs();  // Re-initialize based on current sensor readings
    Serial.println("EMAs re-initialized.");
  }
  //end of big if else statement

  // Initialize Sustained RPM tracking variables
  for (int i = 0; i < RPM_WINDOW_SIZE; ++i) {
    rpmWindow[i] = 0.0;  // Clear the buffer
  }
  rpmWindowIndex = 0;
  rpmSampleCount = 0;
  maxSustainedRPM = 0.0;  // Reset max sustained value at power-on

  //check inital ema values
  Serial.print("Initial/Restored EMA Values -> GForce: ");
  Serial.print(flightData.emaGForce, 2);
  // Serial.print(" | ZAccel: ");
  // Serial.print(emaZAccel, 2);
  Serial.print(" | Altitude: ");
  Serial.print(flightData.emaAltitude, 2);
  Serial.print(" | Velocity: ");
  Serial.println(flightData.emaVelocity, 2);


  // --- Final Setup Steps ---
  resetServos();  // Set servos to initial position
  checkReady();

  Serial.println("Setup Complete.");
  if (flightData.currentState != 0.0) {
    Serial.print("Restored state: ");
    Serial.println(flightData.currentState);
  } else {
    Serial.println("Waiting for launch...");
  }
}



// --- Main Loop ---
void loop() {
  static uint32_t lastLogTime = 0;
  static uint32_t lastXbeeTime = 0;
  static uint32_t lastEEPROMTime = 0;

  // Check for serial commands
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'E' || command == 'e') {
      Serial.println("Displaying EEPROM:");
      displaySlots();
    } else if (command == 'R' || command == 'r') {
      Serial.println("RESETTING EVERYTHING");
      resetEEPROM();  // If you want 'R' to fully reset the current state too
      ESP.restart();  //this goes back to setup
    } else if (command == 'S' || command == 's') {
      Serial.println("Survivability Metrics: ");
      Serial.println(summary);
    } else if (command == 'D' || command == 'd') {
      Serial.println("Transmission String");
      Serial.println(broadcast_string);
    }
  }

  // --- Sensor Readings ---
  float currentAltitude, currentTemp, currentPressure, currentDecibels, bmpPressure;
  float acceleration[3], linearAccel[3], gravity[3], velocity[1], rpmReading[1];
  float currentGForce, currentVelocityMag;

  if (SIMULATION) {
    readSimulation(currentAltitude, currentTemp,
                   currentPressure, bmpPressure, currentDecibels,
                   acceleration, linearAccel, gravity, velocity, rpmReading,
                   currentGForce, currentVelocityMag);
  } else {
    readAllSensors(currentAltitude, currentTemp,
                   currentPressure, bmpPressure, currentDecibels,
                   acceleration, gravity, velocity, rpmReading,
                   currentGForce, currentVelocityMag);  //linearAccel
  }

  // --- Update EMAs ---
  if (emaInitialized) {
    updateEMAs(currentGForce, linearAccel[2], currentAltitude, currentVelocityMag);
  } else {
    Serial.println("Warning: EMAs not initialized!");
    delay(100);  // Avoid spamming
    return;      // Skip the rest of the loop until initialized
  }

  // --- Data Updates & Logging (Common to Launched & Descending) ---
  if (flightData.currentState == 1.0 || flightData.currentState == 2.0) {
    // Update Min/Max values
    updateFlightExtremes(currentGForce, rpmReading[0], currentAltitude, currentTemp, currentPressure, currentDecibels, currentVelocityMag);

    // // Update Max Sustained RPM
    // updateMaxSustainedRPM(rpmReading[0]);

    uint32_t now = millis();

    //log to sd card every 100ms
    if (now - lastLogTime >= 100) {  // Log approx every 100ms
      logFlightDataToSD(now - flightStartTime, flightData.currentState, currentAltitude, currentTemp,
                        rpmReading[0], linearAccel[2], currentGForce, velocity[0], currentDecibels,
                        currentPressure);
      lastLogTime = now;
    }

    // Periodically sync the SD card buffer
    if (now - lastSyncTime >= SD_SYNC_INTERVAL_MS) {
      if (!sd_card.syncLogFile()) {
        Serial.println("Error syncing log file!");
      }
      lastSyncTime = now;
    }

    //save to eeprom every 2.5 seconds
    if (now - lastEEPROMTime >= 2500) {
      saveEEPROM();
      lastEEPROMTime = now;
    }

    saveEEPROM();  // Save current flight data to EEPROM
    currentSlot = (currentSlot + 1) % SLOT_COUNT;
    Serial.println("EEPROM saved");
  }

  //increase 1000ms to whatever
  // if (now - lastXbee >= 3000) {}
  // char message[100];
  // sprintf(message, "1, 4, State, %f, GForce, %.2f, Altitude, %.2f, Velocity, %.2f", currentState, emaGForce, emaAltitude, emaVelocity);
  // xbee.send(message);
  // lastXbee = now;
  // }


  // --- Flight State Machine ---
  switch ((int)flightData.currentState) {
    case 0:
      resetServos();  // Ensure servos are reset while waiting

      // Check for launch conditions using EMA values
      if (flightData.emaGForce > LAUNCH_ACCEL_THRESHOLD || flightData.emaAltitude > LAUNCH_ALTITUDE_THRESHOLD) {  // Added altitude check
        stableReadingLaunch++;
      } else {
        stableReadingLaunch = 0;
      }

      if (stableReadingLaunch >= STABLE_READINGS_FOR_LAUNCH) {
        Serial.println("****** LAUNCH DETECTED! ******");
        flightStartTime = millis();

        // resetcurrentSlotData(); // Reset flight variables for the new flight
        flightData.currentState = 1.0;

        // Set LED red
        onboardLed.setPixelColor(0, onboardLed.Color(50, 0, 0));
        onboardLed.show();

        // Start the new log file (filename already determined in setup or reset)
        if (startNewLogFile(current_filename)) {
          lastSyncTime = millis();  // Reset sync timer for the new file
        } else {
          Serial.println("Failed to start log file.");
          // flightData.currentState = 3.0;  // Go to landed state to prevent further action
        }
      }
      break;

    case 1:
      resetServos();  // Keep servos reset during ascent

      // Check if altitude has decreased sufficiently below flightData.apogee
      if (!descentStarted && flightData.apogee > DESCENT_APOGEE_THRESHOLD && flightData.emaAltitude < (flightData.apogee - DESCENT_ALTITUDE_THRESHOLD)) {  // Ensure some flightData.apogee reached
        stableReadingDescent++;
      } else {
        stableReadingDescent = 0;
      }

      if (stableReadingDescent >= STABLE_READINGS_FOR_DESCENT) {
        flightData.currentState = 2.0;
        saveEEPROM();
        descentStarted = true;                                    // Mark descent started
        onboardLed.setPixelColor(0, onboardLed.Color(0, 50, 0));  // Green for DESCENDING
        onboardLed.show();
        Serial.print("****** APOGEE REACHED (EMA): ");
        Serial.print(flightData.apogee, 2);
        Serial.println(" meters ******");
      }

      // Timeout failsafe
      if (flightStartTime > 0 && ((millis() - flightStartTime) / 1000.0) > 300.0) {  // Check flightStartTime > 0
        Serial.println("****** FLIGHT TIMEOUT (LAUNCHED) - ASSUMING LANDING ******");
        handleLandingSequence(gravity);  // Pass final sensor readings
      }
      break;

    case 2:
      resetServos();  // Keep servos reset during descent

      // Check for landing conditions using EMA values
      if (flightData.emaVelocity < LANDING_VEL_THRESHOLD && abs(flightData.emaGForce - 1.0) < LANDING_ACCEL_THRESHOLD && flightData.emaAltitude < LANDING_ALTITUDE_THRESHOLD) {  // Adjusted altitude check slightly
        stableReadingLand++;
      } else {
        stableReadingLand = 0;  // Reset count if conditions aren't met
      }

      //landing velocity and gforce conditions
      if (stableReadingLand >= STABLE_READINGS_FOR_LANDING_VELOCITY_GFORCE) {
        prevEmaVelocity = (flightData.emaVelocity - (currentVelocityMag * ALPHA_VELOCITY)) / (1.0 - ALPHA_VELOCITY);
        prevEmaGForce = (flightData.emaGForce - (currentGForce * ALPHA_GFORCE)) / (1.0 - ALPHA_GFORCE);

        if (prevEmaVelocity > landingVelocity) {
          landingVelocity = prevEmaVelocity;
        }
        if (prevEmaGForce > landingGForce) {
          landingGForce = prevEmaGForce;
        }
      }

      // Confirm landing after stable readings or timeout
      if (stableReadingLand >= STABLE_READINGS_FOR_LANDING || (flightStartTime > 0 && ((millis() - flightStartTime) / 1000.0) > 300.0)) {  // Check flightStartTime > 0
        if (stableReadingLand >= STABLE_READINGS_FOR_LANDING) {
          Serial.println("****** LANDING DETECTED! ******");
        } else {
          Serial.println("****** FLIGHT TIMEOUT (DESCENDING) - ASSUMING LANDING ******");
        }
        flightData.currentState = 3.0;
        saveEEPROM();

        handleLandingSequence(gravity);  // Pass final sensor readings
      }
      break;

    case 3:
      door = p_serv.servoLogic(pwm, gravity);
      if (SIMULATION) {
        setSimulatedServo(door, 44);
      }
      delay(1000);  // Small delay to prevent busy-waiting if needed
      break;
  }

  Serial.print("Time: ");
  Serial.println(millis());
  // Serial.print("BME Pressure: ");
  // Serial.println(currentPressure);
  // Serial.print("BMP Pressure: ");
  // Serial.println(bmpPressure);
  Serial.print("GForce(EMA): ");
  Serial.println(flightData.emaGForce);
  Serial.print("Altitude(EMA): ");
  Serial.println(flightData.emaAltitude);
  Serial.print("Velocity(EMA): ");
  Serial.println(flightData.emaVelocity);
  Serial.print("State: ");
  Serial.println(flightData.currentState);

  if (SIMULATION) {
    delay(1500);
  }

  //test radio
  //("radio tranmitted", 50);

}  // End loop()


//reading data from pcb sensors
void readAllSensors(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag) {  //float linearAccel[3],
  // Read IMU
  soar_imu.GET_ACCELERATION(acceleration);
  //soar_imu.GET_LINEARACCEL(linearAccel);
  soar_imu.GET_VELOCITY(velocity);
  soar_imu.GET_GRAVITY(gravity);
  soar_imu.GET_RPM(rpmReading);

  // Read Barometers/Temp
  currentAltitude = soar_barometer.get_altitude() - flightData.groundLevel;
  if (!soar_barometer.isReady() || isnan(currentAltitude)) {
    currentAltitude = soar_bme.getAltitude() - flightData.bmeGroundLevel;
  }
  // else if (!soar_barometer.isReady() || !soar_bme.isReady() ||isnan(currentAltitude)) {
  //   currentAltitude = flightData.emaAltitude - flightData.GroundLevel;
  // }

  currentTemp = soar_bme.getTemperature();
  if (!soar_bme.isReady() || isnan(currentTemp)) {
    currentTemp = soar_barometer.get_temperature();
  }

  currentPressure = soar_bme.getPressure();
  if (!soar_bme.isReady() || isnan(currentPressure)) {
    currentPressure = soar_barometer.get_pressure();
  }

  //this is for printing
  bmpPressure = soar_barometer.get_pressure();

  // Read Microphone
  currentDecibels = soar_sph.getDecibels();

  // Calculate derived values
  currentGForce = sqrt(acceleration[0] * acceleration[0] + acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 9.81;
  currentVelocityMag = abs(velocity[0]);  // Assuming velocity[0] holds the primary magnitude/component
}

//sensor reading from the simulation
void readSimulation(float &currentAltitude, float &currentTemp,
                    float &currentPressure, float &bmpPressure, float &currentDecibels,
                    float acceleration[3], float linearAccel[3], float gravity[3], float velocity[1], float rpmReading[1],
                    float &currentGForce, float &currentVelocityMag) {

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

  // Read Microphone
  currentDecibels = 10;

  // Calculate derived values
  currentGForce = sqrt(acceleration[0] * acceleration[0] + acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 9.81;
  currentVelocityMag = abs(velocity[0]);  // Assuming velocity[0] holds the primary magnitude/component

  if (isnan(currentGForce) || isinf(currentGForce)) {
    Serial.println("Warning: NaN/Inf detected in GForce calculation!");
    currentGForce = 1.0;  // Provide a default safe value
  }
  if (isnan(currentAltitude) || isinf(currentAltitude)) {
    Serial.println("Warning: NaN/Inf detected in BMP Altitude!");
    currentAltitude = 0.0;
  }
}

//ema updates for gforce, altitude, velocity
void updateEMAs(float currentGForce, float currentZAccel, float currentAltitude, float currentVelocityMag) {
  flightData.emaGForce = (currentGForce * ALPHA_GFORCE) + (flightData.emaGForce * (1.0 - ALPHA_GFORCE));
  //emaZAccel = (currentZAccel * ALPHA_ZACCEL) + (emaZAccel * (1.0 - ALPHA_ZACCEL));
  flightData.emaAltitude = (currentAltitude * ALPHA_ALTITUDE) + (flightData.emaAltitude * (1.0 - ALPHA_ALTITUDE));
  flightData.emaVelocity = (currentVelocityMag * ALPHA_VELOCITY) + (flightData.emaVelocity * (1.0 - ALPHA_VELOCITY));
}

//get max and min data values
void updateFlightExtremes(float currentGForce, float currentRpm, float currentAltitude, float currentTemp, float currentPressure, float currentDecibels, float imuVelocityMag) {
  // Update Max G-Force
  if (currentGForce > flightData.maxGForce)
    flightData.maxGForce = currentGForce;

  // Update Max Absolute RPM
  if (currentRpm > flightData.maxRPM)
    flightData.maxRPM = currentRpm;

  // Update Max Sustained RPM
  updateMaxSustainedRPM(currentRpm);
  if (maxSustainedRPM > flightData.maxSustainedRPM) {
    flightData.maxSustainedRPM = maxSustainedRPM;
  }

  if (currentAltitude > flightData.apogee)
    flightData.apogee = currentAltitude;

  // Update Temp Min/Max (using BMP temp)
  if (currentTemp < flightData.minTemperature)
    flightData.minTemperature = currentTemp;
  if (currentTemp > flightData.maxTemperature)
    flightData.maxTemperature = currentTemp;

  // Update Pressure Min/Max (using BMP pressure))
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

//max sustained rpm
void updateMaxSustainedRPM(float currentRpm) {
  // 1) Add new sample into circular buffer
  rpmWindow[rpmWindowIndex] = currentRpm;
  rpmWindowIndex = (rpmWindowIndex + 1) % RPM_WINDOW_SIZE;
  if (rpmSampleCount < RPM_WINDOW_SIZE)
    rpmSampleCount++;

  // 2) Once we have a full 1 s of samples, compute that window’s average
  if (rpmSampleCount == RPM_WINDOW_SIZE) {
    float sum = 0;
    for (int i = 0; i < RPM_WINDOW_SIZE; i++)
      sum += rpmWindow[i];
    float avg = sum / RPM_WINDOW_SIZE;

    // 3) Update the overall max sustained RPM
    if (avg > maxSustainedRPM)
      maxSustainedRPM = avg;
  }
}

//after flight lands, instructions after flight lands
void handleLandingSequence(float finalGravityReading[3]) {
  flightData.flightTime = (millis() - flightStartTime);  // Store flight time in milliseconds
  // landingVelocity = finalVelocityReading;                // Record final IMU velocity reading
  flightData.maxSustainedRPM = maxSustainedRPM;  // Store the max sustained RPM for this completed flight
  // flightData.currentState = 3.0;                      // Set state to LANDED

  delay(2000);

  // Trigger payload servo logic

  Serial.println("Finalizing log file...");
  if (!sd_card.syncLogFile()) {  // Sync remaining buffered data
    Serial.println("Warning: Final sync failed before closing!");
  }
  sd_card.closeLogFile();  // Close the main log file

  delay(1000);

  //open servo door pointed up
  door = p_serv.servoLogic(pwm, finalGravityReading);

  if (SIMULATION) {
    setSimulatedServo(door, 44);
  }

  //get landing temp
  if (SIMULATION) {
    landingTemp = getSimulatedTemperature();  // Get simulated temperature for display
  } else {
    landingTemp = soar_bme.getTemperature();
    if (!soar_bme.isReady() || isnan(landingTemp)) {
      landingTemp = soar_barometer.get_temperature();  // Fallback to BMP sensor if BME fails
    }
  }

  //get orientation data
  float euler[3];
  soar_imu.GET_EULER(euler);

  pitch = euler[0];
  roll = euler[1];

  //fill up eeprom
  for (int i = 0; i < 3; i++) {
    saveEEPROM();  // Save final flight data to EEPROM
  }

  saveFlightSummaryToSD();  // Saves summary text file to SD card
  Serial.println(summary);

  // char sum_xbee = summary.c_str();
  // xbee.send(summary);  // Send summary to XBee (or other radio)

  // Display final flight data to Serial
  displayFlightData();

  delay(1000);

  broadcast_string =
    "E KQ4FYU Land Temp = " + String(((landingTemp * 9.0 / 5.0) + 32.0), 2) + " F, " + 
    "Apogee = " + String((flightData.apogee * 3.28084), 2) + " ft, " + 
    "Orientation = Pitch: " + String(pitch) + "°; Roll: " + String(roll) + 
    ", Land Time = " + String((flightData.flightTime) / 1000, 2) + " s, " + 
    "Max Vel = " + String((flightData.maxVelocity * 3.28084), 2) + " ft/s, " + "Land Vel = " + String((landingVelocity * 3.28084), 2) + " ft/s, " + 
    "Land G = " + String(landingGForce, 2) + " G, " + 
    "Survive = " + String(calculateSurvivability() ? "True" : "False") + " KQ4FYU";

  //put broadcast string on sd card
  char transmission_filename[50];
  sprintf(transmission_filename, "/flight_data/transmission_%d.txt", sd_flight_number-1);
  sd_card.writeFile(transmission_filename, broadcast_string.c_str());
  //xbee.send(broadcast_string.c_str())

  std::string std_broadcast_string = broadcast_string.c_str();
  Serial.println(std_broadcast_string.c_str());

  for (int i = 0; i < 3; i++) {
    playMorse(std_broadcast_string, 50);  // Play fast
    Serial.print("completed: ");
    Serial.println(i);
    delay(1000);  // Delay between plays
  }

  Serial.println(" ");
  Serial.println("radio done");

  //sd card readys the next flight number
  sd_flight_number++;
  sprintf(current_filename, "/flight_data/flight_%d.csv", sd_flight_number);  // Prepare filename for next potential flight
  Serial.print("Prepared for next SD flight number: ");
  Serial.println(sd_flight_number);
  Serial.print("Next log filename: ");
  Serial.println(current_filename);

  onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 50));  // Blue for LANDED
  onboardLed.show();
}

//open a new log file on sd card
bool startNewLogFile(const char *filename) {
  Serial.print("Attempting to start new log file: ");
  Serial.println(filename);

  if (sd_card.openLogFile(filename)) {
    String header = "Time(ms),State,Alt(m),Temp(C),RPM,Accel_Z(m/s^2),G-Force,Vel_IMU(m/s),dB,Press(Pa)\n";
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

//log data every iteration to sd card
void logFlightDataToSD(unsigned long timestamp, float currentState, float currentAltitude, float currentTemp,
                       float currentRpm, float currentZAccel, float currentGForce, float currentVelocity, float currentDecibels,
                       float currentPressure) {

  // --- Use snprintf for safer and potentially faster string formatting ---
  char dataBuffer[256];  // Adjust size as needed, make sure it's large enough!
  int writtenChars = snprintf(dataBuffer, sizeof(dataBuffer),
                              "%lu,%.2f,%f,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.2f\n",
                              timestamp,  // Time(ms) since launch
                              currentState,
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

// save summary to sd card
void saveFlightSummaryToSD() {
  // Use sd_flight_number - 1 because it was incremented *after* landing detection
  // before this function is called.
  int flightNumToSave = sd_flight_number - 1;
  if (flightNumToSave < 0)
    flightNumToSave = 0;  // Safety check if called incorrectly

  char summary_filename[50];
  sprintf(summary_filename, "/flight_data/summary_%d.txt", flightNumToSave);
  Serial.print("Saving summary to: ");
  Serial.println(summary_filename);

  // Use String concatenation (acceptable for infrequent summary save)
  summary = "Flight " + String(flightNumToSave) + " Summary\n";
  summary += "--------------------\n";
  summary += "flightData.apogee (BMP/EMA): " + String(flightData.apogee, 2) + " m\n";  // flightData.apogee tracked using EMA of BMP
  summary += "Max Velocity (IMU): " + String(maxVelocity, 2) + " m/s\n";               // Based on IMU reading magnitude
  summary += "Max G-Force: " + String(flightData.maxGForce, 2) + " G\n";
  summary += "Flight Time: " + String(flightData.flightTime / 1000.0, 2) + " s\n";  // Convert ms to s
  summary += "Landing Velocity (IMU): " + String(landingVelocity, 2) + " m/s\n";    // IMU reading at landing detection
  summary += "Max Absolute RPM: " + String(maxRPM, 1) + "\n";
  summary += "Max Sustained RPM (" + String(RPM_WINDOW_SIZE * (100.0 / 1000.0), 1) + "s window): " + String(flightData.maxSustainedRPM, 1) + " RPM\n";  // Show calculated window size
  summary += "Max Decibels: " + String(flightData.maxDecibels, 1) + " dB\n";
  summary += "Min Temperature (BMP): " + String(flightData.minTemperature, 1) + " C\n";
  summary += "Max Temperature (BMP): " + String(flightData.maxTemperature, 1) + " C\n";
  summary += "Min Pressure (BMP): " + String(flightData.minPressure / 100.0, 2) + " hPa\n";  // Convert Pa to hPa
  summary += "Max Pressure (BMP): " + String(flightData.maxPressure / 100.0, 2) + " hPa\n";  // Convert Pa to hPa
  summary += "Servo Door: " + String(door) + " \n";                                          // Convert Pa to hPa
  summary += "--------------------\n";

  // Use the simple writeFile method from the SD card library for the summary
  if (!sd_card.writeFile(summary_filename, summary.c_str())) {
    Serial.println("Error saving summary file!");
  } else {
    Serial.println("Summary file saved.");
  }
}

//make servo stay at 0 degrees
void resetServos() {
  p_serv.setServoAngle(pwm, 8, 0);
  p_serv.setServoAngle(pwm, 12, 0);
  p_serv.setServoAngle(pwm, 0, 0);
  p_serv.setServoAngle(pwm, 4, 0);
}

//initialize ema values
void initializeEMAs() {
  Serial.println("Initializing EMAs...");
  // Get initial readings
  float initialAccel[3], initialVel[1];  // initialLinAccel[3],
  soar_imu.GET_ACCELERATION(initialAccel);
  //soar_imu.GET_LINEARACCEL(initialLinAccel);
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
  flightData.emaAltitude = 0.0;
  flightData.emaGForce = initialGForce;
  //emaZAccel = initialLinAccel[2];               // Z-axis linear acceleration
  flightData.emaVelocity = abs(initialVel[0]);  // Use magnitude
  emaInitialized = true;                        // Mark EMAs as initialized

  Serial.println("EMAs Initialized.");
}


//display final data
void displayFlightData() {
  Serial.println("\n--- CURRENT FLIGHT SUMMARY ---");
  float currentT = soar_bme.getTemperature();  // Get current temp
  if (!soar_bme.isReady() || isnan(currentT)) {
    currentT = soar_barometer.get_temperature();
  }
  Serial.print("Landing Temperature: ");
  Serial.print(currentT, 1);
  Serial.println(" C");

  Serial.print("flightData.apogee: ");
  Serial.print(flightData.apogee, 2);
  Serial.println(" m");
  Serial.print("Flight Time: T+");
  Serial.print(flightData.flightTime / 1000.0, 2);
  Serial.println(" s");  // Use final flightData.flightTime

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
  Serial.print(flightData.maxSustainedRPM, 1);
  Serial.println(" RPM");  // Use the final value
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

//calculate survivability
bool calculateSurvivability() {
  // Use the global variables holding the completed flight's data
  if (SIMULATION) {
    return (flightData.minTemperature >= 10.0 && flightData.maxTemperature <= 35.0 &&  // Check sustained RPM
            (flightData.minPressure / 101325.0) >= 0.0618 &&                           // Convert Pa to atm
            (flightData.maxPressure / 101325.0) <= 100.0 &&                            // Convert Pa to atm
            flightData.maxGForce <= 5.0);
  } else {

    return (flightData.minTemperature >= 10.0 && flightData.maxTemperature <= 35.0 && flightData.maxSustainedRPM <= 10.0 &&  // Check sustained RPM
            (flightData.minPressure / 101325.0) >= 0.0618 &&                                                                 // Convert Pa to atm
            (flightData.maxPressure / 101325.0) <= 100.0 &&                                                                  // Convert Pa to atm
            flightData.maxGForce <= 5.0 && flightData.maxDecibels <= 150.0);
  }
}

//set dac output
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

// //do not know what this does and it is not used
// void modulateMorse(int duration) {
//   pinMode(4, OUTPUT);
//   analogWriteFrequency(4, 440000);
//   int amplitude = 32767;  // Peak amplitude (0-255)
//   int offset = 32767;     // Center value (~2.5V)
//   long waveStart = micros();
//   float angle = 0;

//   while (micros() - waveStart < duration) {
//     angle = 2.0 * PI * 600 * (micros() - waveStart) / 1000000.0;
//     if (angle > 2 * PI)
//       angle = angle - 2 * PI;
//     int dacValue = (int)(offset + amplitude * sin(angle));
//     setDacOutput(dacValue);
//     // delayMicroseconds(10);
//   }
//   return;
// }

//plays morse code for transmission
void playMorse(std::string message, int time_unit) {
  // Serial.println("HI THIS SHOULD BE PRINTING IF THE FUNCTION WORKS");
  digitalWrite(PTTPIN, LOW);  // Start transmit
  for (int i = 0; i < message.length(); i++) {
    char character = toupper(message[i]);

    if (character == ' ') {
      delay(time_unit * 7);  // Word gap
      continue;
    }

    auto iterator = morseMap.find(character);  // Iterator's like auto, apparently.
    if (iterator != morseMap.end()) {
      std::string morse_code = iterator->second;
      Serial.print(morse_code.c_str());  // test print

      for (int j = 0; j < morse_code.length(); j++) {
        if (morse_code[j] == '.') {
          generateTone(time_unit);  // Dot
        } else if (morse_code[j] == '-') {
          generateTone(time_unit * 3);  // Dash
        }
        delay(time_unit);  // Gap between symbols
      }

      delay(time_unit * 2);  // Already had 1-time_unit pause after symbol; add 2 more to make 3-time_unit letter space
    }
  }
  delay(time_unit * 5);
  digitalWrite(PTTPIN, HIGH);
  delay(time_unit * 5);
}

//helper to get the sine wave for the morse code
void generateTone(int duration) {
  int amplitude = 32767;  // Peak amplitude (0-255)
  int offset = 32767;     // Center value (~2.5V)
  float angle = 0;

  long waveStart = millis();
  while (millis() - waveStart < duration) {
    angle = 2.0 * PI * 600 * (micros() - waveStart) / 1000000.0;
    if (angle > 2 * PI)
      angle = angle - 2 * PI;
    int dacValue = (int)(offset + amplitude * sin(angle));
    setDacOutput(dacValue);
  }
  setDacOutput(0);

  return;
}

//saves data to eeprom, just saves the struct
void saveEEPROM() {
  flightData.magic = MAGIC_NUMBER;
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.put(addr, flightData);
  EEPROM.put(SLOT_SAVE_ADDRESS, currentSlot);
  EEPROM.commit();
  Serial.println("→ [EEPROM] Saved.");
}

//if disconnected, it will load the last slot saved in eerpom
void restoreEEPROM() {
  // NOTE: assumes currentSlot already restored
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.get(addr, flightData);
  Serial.println("→ [EEPROM] Data loaded from slot " + String(currentSlot));
}

//press d and it displays current eeprom slots
void displaySlots() {
  Serial.println("[EEPROM] Dumping all slots:");
  for (int s = 0; s < SLOT_COUNT; s++) {
    FlightData tmp;
    int addr = BASE_ADDRESS + (s * sizeof(FlightData));
    EEPROM.get(addr, tmp);
    Serial.println(" Slot " + String(s) + ":");
    displayEEPROM(tmp);
  }
}

//helper function to display data
void displayEEPROM(const FlightData &var) {
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
  Serial.println(var.maxSustainedRPM);
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

//press r and it clears all eeprom data and goes through void setup() again somehow
void resetEEPROM() {
  Serial.println("[EEPROM] Resetting all used bytes...");
  // Erase flightData slots
  for (int i = 0; i < SLOT_COUNT * sizeof(FlightData); i++) {
    EEPROM.write(BASE_ADDRESS + i, 0xFF);
  }
  // Erase saved slot index
  for (int i = 0; i < sizeof(int); i++) {
    EEPROM.write(SLOT_SAVE_ADDRESS + i, 0xFF);
  }
  EEPROM.commit();
  Serial.println("[EEPROM] Erase complete.");
}

void checkReady() {
  if (eepromReady) {
    checkSensor += "EEPROM Ready; ";
  } else {
    checkSensor += "EEPROM Error; ";
  }

  if (soar_imu.isReady()) {
    checkSensor += "IMU Ready; ";
  } else {
    checkSensor += "IMU Error; ";
  }

  if (soar_bme.isReady()) {
    checkSensor += "BME Ready; ";
  } else {
    checkSensor += "BME Error; ";
  }

  if (soar_barometer.isReady()) {
    checkSensor += "BMP Ready; ";
  } else {
    checkSensor += "BMP Error; ";
  }

  if (soar_sph.isReady()) {
    checkSensor += "SPH Ready; ";
  } else {
    checkSensor += "SPH Error; ";
  }

  if (p_serv.isReady()) {
    checkSensor += "Servo Ready; ";
  } else {
    checkSensor += "Servo Error; ";
  }

  if (sd_card.isReady()) {
    checkSensor += "SD Card Ready; ";
  } else {
    checkSensor += "SD Card Error; ";
  }

  if (draReady) {
    checkSensor += "DRA Ready; ";
  } else {
    checkSensor += "DRA Error; ";
  }

  if (dacReady) {
    checkSensor += "DAC Ready; ";
  } else {
    checkSensor += "DAC Error; ";
  }

  //i would do xbee, but the point of this function is to send to xbee

  Serial.println(checkSensor);

  //xbee
  //xbee.send(checkSensor.c_str());
}

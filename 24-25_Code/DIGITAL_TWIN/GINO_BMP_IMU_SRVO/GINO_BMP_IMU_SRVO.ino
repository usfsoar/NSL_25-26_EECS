#include "_config.h"
#include <EEPROM.h>
#include "SOAR_IMU.h"
#include "SOAR_BMP.h"
#include "SOAR_PAYLOAD_SERVO.h"
#include <Wire.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Function prototypes
void resetAllFlightData();
void displayFlightData();
void saveDataToEEPROM();
void displayAllFlightData();

// EEPROM address definitions
#define EEPROM_SIZE 512
#define MAX_FLIGHTS 12    // Maximum number of flights to store
#define FLIGHT_DATA_SIZE 9
#define ADDR_FLIGHT_BASE 0 // Base address for flight data
#define ADDR_FLIGHT_COUNTER 508

// Offsets for flight data variables
#define OFFSET_MAX_G_FORCE 0
#define OFFSET_LANDING_VELOCITY 1
#define OFFSET_FLIGHT_TIME 2
#define OFFSET_MIN_TEMP 3
#define OFFSET_MAX_TEMP 4
#define OFFSET_MIN_PRESSURE 5
#define OFFSET_MAX_PRESSURE 6
#define OFFSET_MAX_VELOCITY 7
#define OFFSET_APOGEE 8

// Global variables
int currentFlight = 0; // Current flight number (0 to 11)

// Flight state definitions
enum FlightState {
  READY,
  LAUNCHED,
  APOGEE_REACHED,
  DESCENDING,
  LANDED
};

// Constants
const float LAUNCH_ACCEL_THRESHOLD = 3;   // G force threshold to detect launch
const float LANDING_ACCEL_THRESHOLD = 1.5;  // G force threshold to detect landing
const float LANDING_VEL_THRESHOLD = 0.5;    // m/s threshold to confirm landing
const int STABLE_READINGS_FOR_LANDING = 10;  // Number of stable readings to confirm landing
const float DESCENT_VELOCITY_THRESHOLD = -0.5; // Negative velocity confirms descent
const float DESCENT_ALTITUDE_THRESHOLD = 1; // Meters fallen below apogee to confirm descent

// Global variables
SOAR_IMU soar_imu;
SOAR_BAROMETER soar_barometer;
FlightState currentState = READY;
SOAR_PAYLOAD_SERVO p_serv;

// Flight data
float maxGForce = 0.0;
float landingVelocity = 0.0;
unsigned long flightStartTime = 0;
unsigned long flightTime = 0;
float minTemperature = 1000.0;  // Initialize to high value
float maxTemperature = -1000.0; // Initialize to low value
float minPressure = 200000.0;   // Initialize to high value
float maxPressure = 0.0;        // Initialize to low value
float maxVelocity = 0.0;
float apogee = 0.0;
int stableReadingCount = 0;
int iteration = 0;
float prevAltitude = 0.0;
float groundLevel = 0.0;
bool descentStarted = false;

void setup() {
  Serial.begin(115200);
  delay(3000);  // Wait for serial monitor

  Serial.println("Rocket Flight Data Recorder");

  // Initialize I2C
  Wire.begin();

  p_serv.initialize(pwm);

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    while (1);
  } else {
    Serial.println("EEPROM initialized successfully.");
  }
  // Displays flight data
  displayAllFlightData();

  // Load the current flight counter from EEPROM
  EEPROM.get(ADDR_FLIGHT_COUNTER, currentFlight);
  if (currentFlight >= MAX_FLIGHTS) {
    currentFlight = 0; // Reset if invalid
  }

  // Initialize IMU
  soar_imu.BNO_SETUP();

  // Initizlize IMU
  soar_barometer.Initialize();
  // if (!soar_barometer.bmp.begin_I2C()) {
  //   Serial.println("Could not find a valid BMP3 sensor, check wiring!");
  //   while (1);
  // }
  

  Serial.println("Waiting for launch...");
  delay(1000);

  Serial.print("Loaded Flight Counter: ");
  Serial.println(currentFlight);

  // Initialize altitude reading
  prevAltitude = soar_barometer.get_altitude();

  // Init ground level
  groundLevel = soar_barometer.get_altitude();
}

void loop() {
  
  // Waits for data or reset command
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'D' || command == 'd') { // Reset all flight data when 'R' or 'r' is received
      Serial.println("Displaying Flight Data: ");
      displayAllFlightData();
    } else if (command == 'R' || command == 'r') { // Reset all flight data when 'R' or 'r' is received
      resetAllFlightData();
    }
  }

  // Pre-allocated arrays
  float acceleration[3];
  float linearAccel[3];
  float gravity[3]; // Pre-allocated array
  float velocity[1];

  // Get sensor readings
  soar_imu.GET_ACCELERATION(acceleration);
  soar_imu.GET_LINEARACCEL(linearAccel);
  soar_imu.GET_VELOCITY(velocity); // Pass pre-allocated array as argument
  soar_imu.GET_GRAVITY(gravity); // Pass the array to the function
  // Serial.print("Velocity: ");
  // Serial.println(velocity[0]);

  // Calculate G force (vector magnitude of acceleration)
  float gForce = sqrt(acceleration[0] * acceleration[0] + acceleration[1] * acceleration[1] + acceleration[2] * acceleration[2]) / 9.81;

  // Update temperature and pressure min/max
  float currentAltitude = soar_barometer.get_altitude() - groundLevel;
  float currentTemp = soar_barometer.get_temperature();
  float currentPressure = soar_barometer.get_pressure();

  if (currentTemp < minTemperature) minTemperature = currentTemp;
  if (currentTemp > maxTemperature) maxTemperature = currentTemp;
  if (currentPressure < minPressure) minPressure = currentPressure;
  if (currentPressure > maxPressure) maxPressure = currentPressure;

  // Update velocity max
  float currentVelocity = (abs(velocity[0]));
  if (currentVelocity > maxVelocity) maxVelocity = currentVelocity;
  // Serial.println(maxVelocity);

  // Flight state machine
  switch (currentState) {
    case READY:
      // Check for launch
      if (gForce > LAUNCH_ACCEL_THRESHOLD) {
        currentState = LAUNCHED;
        flightStartTime = millis();
        Serial.println("LAUNCH DETECTED!");
      }
      break;

    case LAUNCHED:
      // Update max G force
      if (gForce > maxGForce) maxGForce = gForce;

      // Check if current altitude is higher than previous
      if (currentAltitude > prevAltitude) {
        if (currentAltitude > apogee) {
          apogee = currentAltitude;
        }
      } else {
        // If altitude is decreasing, we've reached apogee
        if (!descentStarted && currentAltitude < (apogee - DESCENT_ALTITUDE_THRESHOLD)) {
          currentState = APOGEE_REACHED;
          descentStarted = true;
          Serial.print("APOGEE REACHED: ");
          Serial.print(apogee);
          Serial.println(" meters");
        }
      }
      break;

    case APOGEE_REACHED:
      // Update max G force during descent
      if (gForce > maxGForce) maxGForce = gForce;

      // Already in descent phase, waiting for landing
      currentState = DESCENDING;
      Serial.println("DESCENDING");
      break;

    case DESCENDING:
      // Update max G force during descent
      if (gForce > maxGForce) maxGForce = gForce;

      // Check for landing (stable readings near ground)
      if (abs(velocity[0]) < LANDING_VEL_THRESHOLD &&
          gForce < LANDING_ACCEL_THRESHOLD &&
          currentAltitude < 10.0) {  // Within 10 meters of ground level
        stableReadingCount++;
      } else {
        stableReadingCount = 0;
      }

      // Confirm landing after several stable readings
      if (stableReadingCount >= STABLE_READINGS_FOR_LANDING) {
        currentState = LANDED;
        flightTime = (millis() - flightStartTime) / 1000; // Convert to seconds
        landingVelocity = velocity[0]; // Final velocity at landing
        Serial.println("LANDING DETECTED!");
        saveDataToEEPROM();
        displayFlightData();
        p_serv.servoLogic(pwm, velocity, gravity);
      }
      break;

    case LANDED:
      // Flight complete, do nothing until reset
      delay(1000);
      maxGForce = 0.0;
      landingVelocity = 0.0;
      flightStartTime = 0;
      flightTime = 0;
      minTemperature = 1000.0;  // Initialize to high value
      maxTemperature = -1000.0; // Initialize to low value
      minPressure = 200000.0;   // Initialize to high value
      maxPressure = 0.0;        // Initialize to low value
      maxVelocity = 0.0;
      apogee = 0.0;
      stableReadingCount = 0;
      iteration = 0;
      prevAltitude = 0.0;
      descentStarted = false;
      currentState = READY;

      break;
  }

  // Cleanup and update variables
  prevAltitude = currentAltitude;

  delay(100); // Sampling rate
}

// Save all flight data to EEPROM
void saveDataToEEPROM() {
  int baseAddress = ADDR_FLIGHT_BASE + currentFlight * FLIGHT_DATA_SIZE * sizeof(float);

  // Debug: Print values being saved
  Serial.println("\n--- SAVING FLIGHT DATA ---");
  Serial.print("Max G-Force: "); Serial.println(maxGForce);
  Serial.print("Landing Velocity: "); Serial.println(landingVelocity);
  Serial.print("Flight Time: "); Serial.println(flightTime);
  Serial.print("Min Temperature: "); Serial.println(minTemperature);
  Serial.print("Max Temperature: "); Serial.println(maxTemperature);
  Serial.print("Min Pressure: "); Serial.println(minPressure);
  Serial.print("Max Pressure: "); Serial.println(maxPressure);
  Serial.print("Max Velocity: "); Serial.println(maxVelocity);
  Serial.print("Apogee: "); Serial.println(apogee);

  // Save flight data using EEPROM.put
  EEPROM.put(baseAddress + OFFSET_MAX_G_FORCE * sizeof(float), maxGForce);
  EEPROM.put(baseAddress + OFFSET_LANDING_VELOCITY * sizeof(float), landingVelocity);
  EEPROM.put(baseAddress + OFFSET_FLIGHT_TIME * sizeof(float), flightTime);
  EEPROM.put(baseAddress + OFFSET_MIN_TEMP * sizeof(float), minTemperature);
  EEPROM.put(baseAddress + OFFSET_MAX_TEMP * sizeof(float), maxTemperature);
  EEPROM.put(baseAddress + OFFSET_MIN_PRESSURE * sizeof(float), minPressure);
  EEPROM.put(baseAddress + OFFSET_MAX_PRESSURE * sizeof(float), maxPressure);
  EEPROM.put(baseAddress + OFFSET_MAX_VELOCITY * sizeof(float), maxVelocity);
  EEPROM.put(baseAddress + OFFSET_APOGEE * sizeof(float), apogee);

  // Increment flight counter
  currentFlight++;
  if (currentFlight >= MAX_FLIGHTS) {
    currentFlight = 0; // Wrap around if exceeding max flights
  }

  EEPROM.put(ADDR_FLIGHT_COUNTER, currentFlight);
  EEPROM.commit();

  Serial.print("Current Flight: ");
  Serial.println(currentFlight);

  Serial.println("Flight data saved to EEPROM.");
}

// Function to reset all flight data in EEPROM
void resetAllFlightData() {
  for (int i = 0; i < MAX_FLIGHTS; i++) {
    int baseAddress = ADDR_FLIGHT_BASE + i * FLIGHT_DATA_SIZE * sizeof(float);
    for (int j = 0; j < FLIGHT_DATA_SIZE; j++) {
      EEPROM.put(baseAddress + j * sizeof(float), 0.0f);
    }
  }

  // Reset flight counter
  currentFlight = 0;
  EEPROM.put(ADDR_FLIGHT_COUNTER, currentFlight);
  EEPROM.commit();

  Serial.println("All flight data reset to 0.");
}

// Display flight data on serial monitor
void displayFlightData() {
  Serial.println("\n--- FLIGHT DATA ---");
  Serial.print("Max G-Force: "); Serial.print(maxGForce); Serial.println(" G");
  Serial.print("Landing Velocity: "); Serial.print(landingVelocity); Serial.println(" m/s");
  Serial.print("Flight Time: "); Serial.print(flightTime); Serial.println(" seconds");
  Serial.print("Min Temperature: "); Serial.print(minTemperature); Serial.println(" °C");
  Serial.print("Max Temperature: "); Serial.print(maxTemperature); Serial.println(" °C");
  Serial.print("Min Pressure: "); Serial.print(minPressure); Serial.println(" hPa");
  Serial.print("Max Pressure: "); Serial.print(maxPressure); Serial.println(" hPa");
  Serial.print("Max Velocity: "); Serial.print(maxVelocity); Serial.println(" m/s");
  Serial.print("Apogee: "); Serial.print(apogee); Serial.println(" meters");
  Serial.println("-------------------");
}

// Display all stored flight data
void displayAllFlightData() {
  for (int i = 0; i < MAX_FLIGHTS; i++) {
    int baseAddress = ADDR_FLIGHT_BASE + i * FLIGHT_DATA_SIZE * sizeof(float);

    Serial.print("\n--- FLIGHT ");
    Serial.print(i + 1);
    Serial.println(" DATA ---");

    float maxGForce, landingVelocity, flightTime, minTemperature, maxTemperature, minPressure, maxPressure, maxVelocity, apogee;

    // Read flight data using EEPROM.get
    EEPROM.get(baseAddress + OFFSET_MAX_G_FORCE * sizeof(float), maxGForce);
    EEPROM.get(baseAddress + OFFSET_LANDING_VELOCITY * sizeof(float), landingVelocity);
    EEPROM.get(baseAddress + OFFSET_FLIGHT_TIME * sizeof(float), flightTime);
    EEPROM.get(baseAddress + OFFSET_MIN_TEMP * sizeof(float), minTemperature);
    EEPROM.get(baseAddress + OFFSET_MAX_TEMP * sizeof(float), maxTemperature);
    EEPROM.get(baseAddress + OFFSET_MIN_PRESSURE * sizeof(float), minPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_PRESSURE * sizeof(float), maxPressure);
    EEPROM.get(baseAddress + OFFSET_MAX_VELOCITY * sizeof(float), maxVelocity);
    EEPROM.get(baseAddress + OFFSET_APOGEE * sizeof(float), apogee);

    // Debug: Print values being read
    Serial.print("Max G-Force: "); Serial.println(maxGForce);
    Serial.print("Landing Velocity: "); Serial.println(landingVelocity);
    Serial.print("Flight Time: "); Serial.println(flightTime);
    Serial.print("Min Temperature: "); Serial.println(minTemperature);
    Serial.print("Max Temperature: "); Serial.println(maxTemperature);
    Serial.print("Min Pressure: "); Serial.println(minPressure);
    Serial.print("Max Pressure: "); Serial.println(maxPressure);
    Serial.print("Max Velocity: "); Serial.println(maxVelocity);
    Serial.print("Apogee: "); Serial.println(apogee);
    Serial.println("-------------------");
  }
}
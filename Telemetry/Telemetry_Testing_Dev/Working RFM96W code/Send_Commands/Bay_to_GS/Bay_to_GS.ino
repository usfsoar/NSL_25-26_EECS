#include <SPI.h>
#include <RH_RF95.h>
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "V1_SOAR_RTOS_GPS.h"
#include "sensor_data_types.h"
#include "_config.h"
#include "RTC_Test.h"
#include "SOAR_BMP581.h"
#include "SOAR_BNO085.h"
#include <math.h>
#include "kalman.h"
#include "matrix.h"
#include "extras.h"
#include <string.h>
#include <stdlib.h>   // strtoul, strtod
#include <stdint.h>

#define RFM96W_CS  36
#define RFM96W_RST 9
#define RFM96W_INT 2

SOAR_SD_CARD sd_card(254, true);  // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);            // External, use SPI
SOAR_RTC rtc;
SOAR_RTOS_GPS gps2;               // Wire 1
Adafruit_GPS gps_hw(&Wire1);
BMP581Sensor barometer;           // Wire 2
SOAR_BNO085 imu;                  // Wire
RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);

static float currentFreqMHz = 430.0;

char* msg = (char*)malloc(MAX_DATA);
float RFM96W_FREQ = 430.0;
const int FILE_WRITE_DELAY = 100;

matrix * quat;
matrix * dir;
matrix * acc;
double magnitude;
double kalman_altitude;
double kalman_velocity;
double kalman_acceleration;
float altitude;
float pressure;
float temperature;
float i_altitude;
int stage = 0;

uint32_t t_now, t_prev;
double dt = 0.05; /* must be accurate to data rate */
const double sigma_j = 0.2; /* process StdDev: TUNED */
const double sigma_s = 0.1666667; /* altitude reading StdDev */
const double sigma_a = 0.179; /* acceleration reading StdDev */
const int states = 3;
const int observations = 2;
const double MIN_ALT = 3; /* trust sensors below this altitude [m]: allows filter to adapt quickly */
const double APOGEE = 3048; // [m] 10k ft
kalmanFilter *filter = NULL;

char dataBuffer[512];

void write_sd_file_headers(SOAR_SD_CARD& sd) {
  Serial.println("Writing SD file headers");
  sd.deleteFile(IMU_FILEPATH);
  sd.deleteFile(ALTIMETER_FILEPATH);
  sd.deleteFile(GPS_FILEPATH);
  sd.deleteFile(KALMAN_FILEPATH);

  sd.writeFile(IMU_FILEPATH, "time_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd.writeFile(GPS_FILEPATH, "time_stamp,gps_data\n");
  sd.writeFile(KALMAN_FILEPATH, "time_stamp,altitude,velocity,acceleration\n");
}

void writeToBothCards(const char* filename, const char* data) {
  sd_card.appendFile(filename, data);
  delay(50);
  sd_card2.appendFile(filename, data);
}

void writeSensorDataToSD(SensorData& sensor_data) {
  const char* filename = nullptr;
  int len = 0;
  
  switch (sensor_data.type) {
    case IMU:
      filename = IMU_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer),
        "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.imu.accel[0], sensor_data.data.imu.accel[1], sensor_data.data.imu.accel[2],
        sensor_data.data.imu.linear[0], sensor_data.data.imu.linear[1], sensor_data.data.imu.linear[2],
        sensor_data.data.imu.gravity[0], sensor_data.data.imu.gravity[1], sensor_data.data.imu.gravity[2],
        sensor_data.data.imu.quat[0], sensor_data.data.imu.quat[1], sensor_data.data.imu.quat[2], sensor_data.data.imu.quat[3],
        sensor_data.data.imu.gyro[0], sensor_data.data.imu.gyro[1], sensor_data.data.imu.gyro[2]);
      break;
      
    case ALTIMETER:
      filename = ALTIMETER_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.alt.altitude,
        sensor_data.data.alt.temp,
        sensor_data.data.alt.pressure);
      break;
      
    case GPS:
      filename = GPS_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%s\n",
        sensor_data.timestamp,
        sensor_data.data.gps.nmea);
      break;
    
    case KALMAN:
      filename = KALMAN_FILEPATH;
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s,%.6f,%.6f,%.6f\n",
        sensor_data.timestamp,
        sensor_data.data.kalman.kalman_altitude,
        sensor_data.data.kalman.kalman_velocity,
        sensor_data.data.kalman.kalman_acceleration);
      break;
  }
  
  if (filename && len > 0 && (size_t)len < sizeof(dataBuffer)) {
    writeToBothCards(filename, dataBuffer);
    Serial.print("Data written: ");
    Serial.println(filename);
  }
}

// ---- Telemetry pause control ----
static uint32_t quietUntilMs = 0;                  // don't send telemetry before this time
static const uint32_t QUIET_AFTER_CMD_MS = 1200;   // pause after receiving any command
static const uint32_t QUIET_AFTER_SWITCH_MS = 1200;// pause after switching freq

// ---- Duplicate protection ----
static uint32_t lastFreqSeq = 0;    // last seq for FREQ handled
static float    lastFreqNew = 0.0f; // last requested freq

// ---------------- Helpers ----------------
static bool startsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

static bool parseFreqCmd(const char* s, uint32_t& seqOut, float& fOut) {
  // "FREQ,<seq>,<mhz>"
  if (!startsWith(s, "FREQ,")) return false;
  const char* p = s + 5;

  char* end1 = nullptr;
  unsigned long seq = strtoul(p, &end1, 10);
  if (!end1 || *end1 != ',') return false;

  float f = (float)strtod(end1 + 1, nullptr);
  seqOut = (uint32_t)seq;
  fOut = f;
  return true;
}

static bool parsePingCmd(const char* s, uint32_t& seqOut) {
  // "PING,<seq>"
  if (!startsWith(s, "PING,")) return false;
  seqOut = (uint32_t)strtoul(s + 5, nullptr, 10);
  return true;
}

static bool parseRebootCmd(const char* s, uint32_t& seqOut) {
  // "REBOOT,<seq>"
  if (!startsWith(s, "REBOOT,")) return false;
  seqOut = (uint32_t)strtoul(s + 7, nullptr, 10);
  return true;
}

static void sendAscii(const char* msg) {
  rfm96w.setModeIdle();
  rfm96w.send((uint8_t*)msg, (uint8_t)strlen(msg));
  rfm96w.waitPacketSent();
  rfm96w.setModeRx();
}

static void setFreq(float mhz) {
  rfm96w.setModeIdle();
  if (rfm96w.setFrequency(mhz)) {
    currentFreqMHz = mhz;
  }
  rfm96w.setModeRx();
}

static void pauseTelemetry(uint32_t ms) {
  uint32_t now = millis();
  uint32_t until = now + ms;
  if ((int32_t)(until - quietUntilMs) > 0) quietUntilMs = until; // extend if needed
}

static void teensyReboot() {
  // ARM Cortex-M: system reset request
  // Equivalent to NVIC_SystemReset(), but available without extra includes.
  SCB_AIRCR = 0x05FA0004; // VECTKEY (0x5FA) | SYSRESETREQ
  while (1) { }           // wait for reset
}

void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}

  // Initialize both SD cards
  Serial.println("Initializing SD card 1 (pin 254)...");
  sd_card.begin();
  
  Serial.println("Initializing SD card 2 (pin 10)...");
  sd_card2.begin();

  // Write headers to both cards
  Serial.println("Writing headers to SD card 1...");
  write_sd_file_headers(sd_card);
  
  Serial.println("Writing headers to SD card 2...");
  write_sd_file_headers(sd_card2);

  // Reset radio
  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);

  if (!rfm96w.init()) {
    Serial.println("RFM96W initialization failed");
    while (1) delay(100);
  }
  Serial.println("RFM96W initialization succeeded");

  if (!rfm96w.setFrequency(currentFreqMHz)) {
    Serial.println("setFrequency failed");
    while (1) delay(100);
  }

  rfm96w.setSignalBandwidth(100000);
  rfm96w.setSpreadingFactor(7);
  rfm96w.setTxPower(20, false);

  Serial.print("Bay freq set to ");
  Serial.println(currentFreqMHz, 3);

  rfm96w.setModeRx();

  Wire1.begin();
  Wire1.setClock(100000);
  gps2.setup();

  Wire2.begin();
  Wire2.setClock(400000);
  barometer.begin();

  delay(1000);
  i_altitude = barometer.get_altitude();

  quat = matrixCreate(4, 1);
  dir = matrixCreate(3, 1);
  acc = matrixCreate(3, 1);
  setElement(dir, 3, 1, 1);

  filter = kalmanFilterCreate(states, observations);

  /* set filter matrices and vectors matrices
   * matrices are initially all 0s so no need to set them
   * initialize x_k_prev and P_k_prev not current time */
  
  /* F_k for constant acceleration assumption */
  setElement(filter->F_k, 1, 1, 1);
  setElement(filter->F_k, 1, 2, dt);
  setElement(filter->F_k, 1, 3, 0.5*dt*dt);
  setElement(filter->F_k, 2, 2, 1);
  setElement(filter->F_k, 2, 3, dt);
  setElement(filter->F_k, 3, 3, 1);
  
  setElement(filter->H_k, 1, 1, 1);
  setElement(filter->H_k, 2, 3, 1);
  
  /* process covar */
  setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
  setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));
  
  /* reading covar */
  setElement(filter->R_k, 1, 1, sigma_s * sigma_s);
  setElement(filter->R_k, 2, 2, sigma_a * sigma_a);
  
  /* uncertainty of initial velocity and acceleration */
  setElement(filter->P_k_prev, 2, 2, 100);
  setElement(filter->P_k_prev, 3, 3, 100);
  
  t_prev = micros();
  Serial.println("Setup complete!");
}

void loop() {
  // --------- 1) Listen for commands ----------
  if (rfm96w.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;

    if (rfm96w.recv(buf, &len)) {
      buf[len] = 0;
      const char* s = (const char*)buf;

      uint32_t seq;
      float fNew;

      // Any command triggers a quiet window to reduce airtime collisions
      pauseTelemetry(QUIET_AFTER_CMD_MS);

      if (parseFreqCmd(s, seq, fNew)) {
        Serial.print("[Bay] RX ");
        Serial.println(s);

        // Always ACK on old freq
        char ack[64];
        snprintf(ack, sizeof(ack), "ACKFREQ,%lu,%.3f", (unsigned long)seq, fNew);
        Serial.print("[Bay] TX ");
        Serial.println(ack);
        sendAscii(ack);

        // Avoid thrashing: if we already handled this exact FREQ seq, don't switch again.
        if (seq == lastFreqSeq && fNew == lastFreqNew) {
          Serial.println("[Bay] Duplicate FREQ seq; ACK re-sent, not switching again.");
        } else {
          lastFreqSeq = seq;
          lastFreqNew = fNew;

          delay(50); // guard time so ACK finishes before retune

          Serial.print("[Bay] Switching to ");
          Serial.println(fNew, 3);
          setFreq(fNew);

          // After switching, extend quiet window to allow GS to switch + ping cleanly
          pauseTelemetry(QUIET_AFTER_SWITCH_MS);
        }
      }
      else if (parsePingCmd(s, seq)) {
        Serial.print("[Bay] RX ");
        Serial.println(s);

        char ack[32];
        snprintf(ack, sizeof(ack), "ACKPING,%lu", (unsigned long)seq);
        Serial.print("[Bay] TX ");
        Serial.println(ack);
        sendAscii(ack);

        // Give a short quiet window after ACKPING too
        pauseTelemetry(300);
      }
      else if (parseRebootCmd(s, seq)) {
        Serial.print("[Bay] RX ");
        Serial.println(s);

        char ack[40];
        snprintf(ack, sizeof(ack), "ACKREBOOT,%lu", (unsigned long)seq);
        Serial.print("[Bay] TX ");
        Serial.println(ack);
        sendAscii(ack);

        delay(200);
        Serial.println("[Bay] Rebooting...");
        teensyReboot();
      }
      else {
        Serial.print("[Bay] RX unknown: ");
        Serial.println(s);
      }
    }
  }

  // --------- 2) Telemetry sender (paused during handshake) ----------
  if ((int32_t)(millis() - quietUntilMs) >= 0) {
    // Get timestamp
    t_now = micros();
    dt = (double)(t_now - t_prev) * 1e-6; /* new dt in seconds */
    Serial.printf("\nloop time = %.2lf\n", dt);
    t_prev = micros();
    String ts = rtc.getTimestamp(true);

    /* F_k for constant acceleration assumption */
    setElement(filter->F_k, 1, 2, dt);
    setElement(filter->F_k, 1, 3, 0.5*dt*dt);
    setElement(filter->F_k, 2, 2, 1);
    setElement(filter->F_k, 2, 3, dt);

    /* process covar */
    setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
    setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
    setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
    setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
    setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
    setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
    setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
    setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
    setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));

    // Read GPS data
    char nmea_tmp[100];
    gps2.GET_NMEA(nmea_tmp, sizeof(nmea_tmp));
    SensorData gps_data;
    gps_data.type = GPS;
    strncpy(gps_data.timestamp, ts.c_str(), sizeof(gps_data.timestamp) - 1);
    gps_data.timestamp[sizeof(gps_data.timestamp) - 1] = '\0';
    strncpy(gps_data.data.gps.nmea, nmea_tmp, sizeof(gps_data.data.gps.nmea) - 1);
    gps_data.data.gps.nmea[sizeof(gps_data.data.gps.nmea) - 1] = '\0';

    Serial.println("Writing gps data...");
    writeSensorDataToSD(gps_data);
    delay(FILE_WRITE_DELAY);

    dataToString(gps_data, msg);
    Serial.printf("Sending gps: %s\n", msg); 
    sendAscii(msg);

    // Read altimeter data
    altitude = barometer.get_altitude() - i_altitude;
    pressure = barometer.get_pressure();
    temperature = barometer.get_temperature();
    if (altitude == 0) {
        Serial.println("Altitude is 0, retrying...");
        barometer.begin();
    }

    SensorData altimeter_data;
    altimeter_data.type = ALTIMETER;
    strncpy(altimeter_data.timestamp, ts.c_str(), sizeof(altimeter_data.timestamp) - 1);
    altimeter_data.timestamp[sizeof(altimeter_data.timestamp) - 1] = '\0';
    altimeter_data.data.alt.altitude = altitude;
    altimeter_data.data.alt.temp = temperature;
    altimeter_data.data.alt.pressure = pressure;
    
    Serial.println("Writing altimeter data...");
    writeSensorDataToSD(altimeter_data);
    delay(FILE_WRITE_DELAY);

    dataToString(altimeter_data, msg);
    Serial.printf("Sending alt: %s\n", msg);
    sendAscii(msg);

    // Read IMU data
    imu.update();
    SensorData imu_data;
    imu_data.type = IMU;
    strncpy(imu_data.timestamp, ts.c_str(), sizeof(imu_data.timestamp) - 1);
    imu_data.timestamp[sizeof(imu_data.timestamp) - 1] = '\0';
    imu_data.data.imu.accel[0] = imu.sensorData.acceleration.x;
    imu_data.data.imu.accel[1] = imu.sensorData.acceleration.y;
    imu_data.data.imu.accel[2] = imu.sensorData.acceleration.z;
    imu_data.data.imu.linear[0] = imu.sensorData.linearAcceleration.x;
    imu_data.data.imu.linear[1] = imu.sensorData.linearAcceleration.y;
    imu_data.data.imu.linear[2] = imu.sensorData.linearAcceleration.z;
    imu_data.data.imu.gravity[0] = imu.sensorData.gravity.x;
    imu_data.data.imu.gravity[1] = imu.sensorData.gravity.y;
    imu_data.data.imu.gravity[2] = imu.sensorData.gravity.z;
    imu_data.data.imu.quat[0] = imu.sensorData.orientation.w;
    imu_data.data.imu.quat[1] = imu.sensorData.orientation.x;
    imu_data.data.imu.quat[2] = imu.sensorData.orientation.y;
    imu_data.data.imu.quat[3] = imu.sensorData.orientation.z;
    imu_data.data.imu.gyro[0] = imu.sensorData.gyroscope.x;
    imu_data.data.imu.gyro[1] = imu.sensorData.gyroscope.y;
    imu_data.data.imu.gyro[2] = imu.sensorData.gyroscope.z;

    Serial.println("Writing IMU data...");
    writeSensorDataToSD(imu_data);
    delay(FILE_WRITE_DELAY);

    dataToString(imu_data, msg);
    Serial.printf("Sending imu: %s\n", msg);
    sendAscii(msg);

    // Kalman filter
    if (altitude < MIN_ALT) { // fully trust sensors
          setElement(filter->P_k_prev, 2, 2, 100);
          setElement(filter->P_k_prev, 3, 3, 100);
    }

    setElement(quat, 1, 1, imu_data.data.imu.quat[0]);
    setElement(quat, 2, 1, imu_data.data.imu.quat[1]);
    setElement(quat, 3, 1, imu_data.data.imu.quat[2]);
    setElement(quat, 4, 1, imu_data.data.imu.quat[3]);

    setElement(acc, 1, 1, imu_data.data.imu.linear[0]);
    setElement(acc, 2, 1, imu_data.data.imu.linear[1]);
    setElement(acc, 3, 1, imu_data.data.imu.linear[2]);

    vectorComponent(acc, quat, dir, &magnitude);

    setElement(filter->z_k, 1, 1, altitude);
    setElement(filter->z_k, 2, 1, magnitude);
    kalmanFilterPredict(filter);
    kalmanFilterUpdate(filter);

    getElement(filter->x_k, 1, 1, &kalman_altitude);
    getElement(filter->x_k, 2, 1, &kalman_velocity);
    getElement(filter->x_k, 3, 1, &kalman_acceleration);

    SensorData kalman_data;
    kalman_data.type = KALMAN;
    strncpy(kalman_data.timestamp, ts.c_str(), sizeof(kalman_data.timestamp) - 1);
    kalman_data.timestamp[sizeof(kalman_data.timestamp) - 1] = '\0';
    kalman_data.data.kalman.kalman_altitude = kalman_altitude;
    kalman_data.data.kalman.kalman_velocity = kalman_velocity;
    kalman_data.data.kalman.kalman_acceleration = kalman_acceleration;
    
    Serial.println("Writing kalman data...");
    writeSensorDataToSD(kalman_data);
    delay(FILE_WRITE_DELAY);

    dataToString(kalman_data, msg);
    Serial.printf("Sending kalman: %s\n", msg);
    sendAscii(msg);

    // Stage update
    switch (stage) {
      case 0: // On Ground
          if((kalman_acceleration > 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) {
              stage = 1;
          }
          break;
      case 1: // Thrust Phase
          if((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) {
              stage = 2;
          }
          break;
      case 2: // Falling Upwards
          if((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > APOGEE)) {
              stage = 3;
          }
          break;
      case 3: // Passing Apogee
          if((kalman_velocity < 0) && (kalman_altitude < APOGEE)) {
              stage = 4;
          }
          break;
      case 4: // Falling Below Apogee
          if(kalman_altitude < MIN_ALT) {
              stage = 0;
          }
          break;
    }

    Serial.println("GPS: " + String(nmea_tmp));
    Serial.println("Altitude: " + String(altitude));
    Serial.println("Pressure: " + String(pressure));
    Serial.println("Temperature: " + String(temperature));
    snprintf(dataBuffer, sizeof(dataBuffer),
        "IMU: %.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
        imu_data.data.imu.accel[0], imu_data.data.imu.accel[1], imu_data.data.imu.accel[2],
        imu_data.data.imu.linear[0], imu_data.data.imu.linear[1], imu_data.data.imu.linear[2],
        imu_data.data.imu.gravity[0], imu_data.data.imu.gravity[1], imu_data.data.imu.gravity[2],
        imu_data.data.imu.quat[0], imu_data.data.imu.quat[1], imu_data.data.imu.quat[2], imu_data.data.imu.quat[3],
        imu_data.data.imu.gyro[0], imu_data.data.imu.gyro[1], imu_data.data.imu.gyro[2]);
    snprintf(dataBuffer, sizeof(dataBuffer),
        "KALMAN: [%.6f] %.2f,%.2f,%.2f", dt,
        kalman_data.data.kalman.kalman_altitude, kalman_data.data.kalman.kalman_velocity, kalman_data.data.kalman.kalman_acceleration);
    Serial.println("Stage: " + String(stage));
    Serial.println(dataBuffer);
    delay(100);
  } else {
    // While quiet, keep loop responsive
    delay(5);
  }
}

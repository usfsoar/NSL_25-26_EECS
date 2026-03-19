/*
 * V3_Telemetry_Bay_2026.ino
 * 
 * Integrated Telemetry System for Teensy 4.1
 * Reads data from IMU (BNO085), Barometer (BMP581), and GPS sensors
 * Integrated Kalman filter
 * Logs all data to SD card in CSV format
 * 
 * No RTOS - uses simple sequential polling in loop()
 */

#include <SPI.h>
#include <RH_RF95.h>
#include <Watchdog_t4.h>

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

// ------------------- Hardware / modules -------------------
WDT_T4<WDT3> wdt;
SOAR_SD_CARD sd_card(254, true);  // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);            // External, use SPI
SOAR_RTC rtc;
SOAR_RTOS_GPS gps2;               // Wire 1
Adafruit_GPS gps_hw(&Wire1);
BMP581Sensor barometer;           // Wire 2
SOAR_BNO085 imu;                  // Wire
RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);

// ------------------- Radio params -------------------
static float RFM96W_FREQ = 430.0f;
static const uint32_t RFM_BW_HZ = 60000;
static const uint8_t  RFM_SF    = 9;
bool telemetry_on = false;
const char initial_msg[100] = "Callsign: KR4IJA | Team: 24 | Beginning Transmissions \n";

// ------------------- Buffers / misc -------------------
static const int FILE_WRITE_DELAY = 0;
char* msg = nullptr;                 // payload buffer from dataToString()
char dataBuffer[512];

// ------------------- Kalman / math state -------------------
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

// ------------------- Telemetry pause control -------------------
static uint32_t quietUntilMs = 0;
static const uint32_t QUIET_AFTER_CMD_MS = 1200;
static const uint32_t QUIET_AFTER_SWITCH_MS = 1200;

// ------------------- Duplicate protection for FREQ -------------------
static uint32_t lastFreqSeq = 0;
static float    lastFreqNew = 0.0f;

// ------------------- Telemetry ACK protocol state -------------------
static uint32_t bay_seq = 1;        // Bay telemetry sequence (independent)
static uint32_t last_gs_seq = 0;    // last GS ACK seq seen (independent)

static bool waitingAck = false;
static uint8_t  inflight_type = 0;
static uint32_t inflight_seq  = 0;
static uint32_t ack_deadline_ms = 0;

// No resend requested:
static const uint32_t ACK_TIMEOUT_MS = 1500;

// Store payload for inflight packet (for logging/debug; not resending)
static char inflight_payload[RH_RF95_MAX_MESSAGE_LEN + 1];

// Transmit order
// 0=IMU,1=ALT,2=GPS,3=KALMAN
static uint8_t next_type_to_send = 2;

// ------------------- Scheduling (optimize delays) -------------------
// Sensor update intervals (ms). Tune to your real sensor rates.
static const uint32_t GPS_PERIOD_MS    = 200; // 5 Hz
static const uint32_t ALT_PERIOD_MS    = 50;  // 20 Hz
static const uint32_t IMU_PERIOD_MS    = 20;  // 50 Hz
static const uint32_t KALMAN_PERIOD_MS = 50;  // 20 Hz

// TX pacing: we only TX when not waitingAck. This is the *minimum* gap between packets.
static const uint32_t TX_GAP_MS = 10;

static uint32_t lastGpsMs = 0;
static uint32_t lastAltMs = 0;
static uint32_t lastImuMs = 0;
static uint32_t lastKalmanMs = 0;
static uint32_t lastTxMs = 0;

// Keep latest sensor snapshots (so we can send one-per-ACK while still updating filters)
static SensorData latest_gps{};
static SensorData latest_alt{};
static SensorData latest_imu{};
static SensorData latest_kalman{};
static bool have_gps = false, have_alt = false, have_imu = false, have_kalman = false;

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

static bool parseAckTlm(const char* s, uint8_t& typeOut, uint32_t& baySeqOut, uint32_t& gsSeqOut) {
  // "ACKTLM,<type>,<bay_seq>,<gs_seq>"
  if (!startsWith(s, "ACKTLM,")) return false;

  const char* p = s + 7; // after "ACKTLM,"
  char* end = nullptr;

  unsigned long t = strtoul(p, &end, 10);
  if (!end || *end != ',') return false;

  unsigned long bseq = strtoul(end + 1, &end, 10);
  if (!end || *end != ',') return false;

  unsigned long gseq = strtoul(end + 1, nullptr, 10);

  typeOut = (uint8_t)t;
  baySeqOut = (uint32_t)bseq;
  gsSeqOut = (uint32_t)gseq;
  return true;
}

static void sendAscii(const char* m) {
  rfm96w.setModeIdle();
  rfm96w.send((uint8_t*)m, (uint8_t)strlen(m));
  rfm96w.waitPacketSent();
  rfm96w.setModeRx();
}

static void setFreq(float mhz) {
  rfm96w.setModeIdle();
  if (rfm96w.setFrequency(mhz)) {
    RFM96W_FREQ = mhz;
  }
  rfm96w.setModeRx();
}

static void pauseTelemetry(uint32_t ms) {
  uint32_t now = millis();
  uint32_t until = now + ms;
  if ((int32_t)(until - quietUntilMs) > 0) quietUntilMs = until;
}

static void teensyReboot() {
  SCB_AIRCR = 0x05FA0004; // SYSRESETREQ
  while (1) { }
}

// Build "TLM,<type>,<seq>,<payload>" safely into out[]
// If payload is too long, it truncates payload to fit.
static void buildTlmPacketTrunc(uint8_t type, uint32_t seq, const char* payload, char* out, size_t outSize) {
  if (!payload) payload = "";

  // First print header without payload
  int hn = snprintf(out, outSize, "TLM,%u,%lu,", (unsigned)type, (unsigned long)seq);
  if (hn <= 0 || (size_t)hn >= outSize) {
    out[0] = '\0';
    return;
  }

  // Remaining space for payload (leave room for null)
  size_t rem = outSize - (size_t)hn - 1;
  strncpy(out + hn, payload, rem);
  out[hn + rem] = '\0';
}

static void startTelemetryTxn(uint8_t type, const char* payload) {
  inflight_type = type;
  inflight_seq  = bay_seq;
  waitingAck = true;
  ack_deadline_ms = millis() + ACK_TIMEOUT_MS;

  // store payload for debug
  strncpy(inflight_payload, payload ? payload : "", sizeof(inflight_payload) - 1);
  inflight_payload[sizeof(inflight_payload) - 1] = '\0';

  static char pkt[RH_RF95_MAX_MESSAGE_LEN + 1];
  buildTlmPacketTrunc(inflight_type, inflight_seq, inflight_payload, pkt, sizeof(pkt));

  if (pkt[0] == '\0') {
    Serial.println("[Bay] ERROR: could not build TLM packet.");
    waitingAck = false;
    return;
  }

  Serial.printf("[Bay] TX %s\n", pkt);
  sendAscii(pkt);
  lastTxMs = millis();
}

// ------------------- SD helpers -------------------
void write_sd_file_headers(SOAR_SD_CARD& sd) {
  sd.appendFile(IMU_FILEPATH, "\n\nInitializing\n\ntime_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.appendFile(ALTIMETER_FILEPATH, "\n\nInitializing\n\ntime_stamp,altitude,temperature,pressure\n");
  sd.appendFile(GPS_FILEPATH, "\n\nInitializing\n\ntime_stamp,gps_data\n");
  sd.appendFile(KALMAN_FILEPATH, "\n\nInitializing\n\ntime_stamp,altitude,velocity,acceleration\n");
}

void writeToBothCards(const char* filename, const char* data) {
  // Avoid artificial delays; SD libs already block as needed.
  sd_card.appendFile(filename, data);
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
  }
}

// ------------------- Sensor update functions (scheduled) -------------------
static void getTimestamp(char* ts, size_t n) {
  rtc.getTimestamp(ts, n, true);
}

static void updateGPS(const char* ts) {
  char nmea_tmp[100];
  gps2.GET_NMEA(nmea_tmp);

  latest_gps.type = GPS;
  strncpy(latest_gps.timestamp, ts, sizeof(latest_gps.timestamp) - 1);
  latest_gps.timestamp[sizeof(latest_gps.timestamp) - 1] = '\0';
  strncpy(latest_gps.data.gps.nmea, nmea_tmp, sizeof(latest_gps.data.gps.nmea) - 1);
  latest_gps.data.gps.nmea[sizeof(latest_gps.data.gps.nmea) - 1] = '\0';

  writeSensorDataToSD(latest_gps);
  have_gps = true;
}

static void updateALT(const char* ts) {
  altitude = barometer.get_altitude() - i_altitude;
  pressure = barometer.get_pressure();
  temperature = barometer.get_temperature();

  if (altitude == 0) {
    // Don't block with retries; just re-init once.
    barometer.begin();
  }

  latest_alt.type = ALTIMETER;
  strncpy(latest_alt.timestamp, ts, sizeof(latest_alt.timestamp) - 1);
  latest_alt.timestamp[sizeof(latest_alt.timestamp) - 1] = '\0';
  latest_alt.data.alt.altitude = altitude;
  latest_alt.data.alt.temp = temperature;
  latest_alt.data.alt.pressure = pressure;

  writeSensorDataToSD(latest_alt);
  have_alt = true;
}

static void updateIMU(const char* ts) {
  imu.update();

  latest_imu.type = IMU;
  strncpy(latest_imu.timestamp, ts, sizeof(latest_imu.timestamp) - 1);
  latest_imu.timestamp[sizeof(latest_imu.timestamp) - 1] = '\0';

  latest_imu.data.imu.accel[0] = imu.sensorData.acceleration.x;
  latest_imu.data.imu.accel[1] = imu.sensorData.acceleration.y;
  latest_imu.data.imu.accel[2] = imu.sensorData.acceleration.z;

  latest_imu.data.imu.linear[0] = imu.sensorData.linearAcceleration.x;
  latest_imu.data.imu.linear[1] = imu.sensorData.linearAcceleration.y;
  latest_imu.data.imu.linear[2] = imu.sensorData.linearAcceleration.z;

  latest_imu.data.imu.gravity[0] = imu.sensorData.gravity.x;
  latest_imu.data.imu.gravity[1] = imu.sensorData.gravity.y;
  latest_imu.data.imu.gravity[2] = imu.sensorData.gravity.z;

  latest_imu.data.imu.quat[0] = imu.sensorData.orientation.w;
  latest_imu.data.imu.quat[1] = imu.sensorData.orientation.x;
  latest_imu.data.imu.quat[2] = imu.sensorData.orientation.y;
  latest_imu.data.imu.quat[3] = imu.sensorData.orientation.z;

  latest_imu.data.imu.gyro[0] = imu.sensorData.gyroscope.x;
  latest_imu.data.imu.gyro[1] = imu.sensorData.gyroscope.y;
  latest_imu.data.imu.gyro[2] = imu.sensorData.gyroscope.z;

  writeSensorDataToSD(latest_imu);
  have_imu = true;
}

static void updateKalman(const char* ts) {
  // Needs altitude + IMU magnitude; use latest values if available.
  if (!have_alt || !have_imu) return;

  // Update dt-dependent matrices (dt already updated in loop)
  setElement(filter->F_k, 1, 2, dt);
  setElement(filter->F_k, 1, 3, 0.5 * dt * dt);
  setElement(filter->F_k, 2, 2, 1);
  setElement(filter->F_k, 2, 3, dt);

  setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
  setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));

  if (altitude < MIN_ALT) {
    setElement(filter->P_k_prev, 2, 2, 100);
    setElement(filter->P_k_prev, 3, 3, 100);
  }

  setElement(quat, 1, 1, latest_imu.data.imu.quat[0]);
  setElement(quat, 2, 1, latest_imu.data.imu.quat[1]);
  setElement(quat, 3, 1, latest_imu.data.imu.quat[2]);
  setElement(quat, 4, 1, latest_imu.data.imu.quat[3]);

  setElement(acc, 1, 1, latest_imu.data.imu.linear[0]);
  setElement(acc, 2, 1, latest_imu.data.imu.linear[1]);
  setElement(acc, 3, 1, latest_imu.data.imu.linear[2]);

  vectorComponent(acc, quat, dir, &magnitude);

  setElement(filter->z_k, 1, 1, altitude);
  setElement(filter->z_k, 2, 1, magnitude);

  kalmanFilterPredict(filter);
  kalmanFilterUpdate(filter);

  getElement(filter->x_k, 1, 1, &kalman_altitude);
  getElement(filter->x_k, 2, 1, &kalman_velocity);
  getElement(filter->x_k, 3, 1, &kalman_acceleration);

  latest_kalman.type = KALMAN;
  strncpy(latest_kalman.timestamp, ts, sizeof(latest_kalman.timestamp) - 1);
  latest_kalman.timestamp[sizeof(latest_kalman.timestamp) - 1] = '\0';
  latest_kalman.data.kalman.kalman_altitude = kalman_altitude;
  latest_kalman.data.kalman.kalman_velocity = kalman_velocity;
  latest_kalman.data.kalman.kalman_acceleration = kalman_acceleration;

  writeSensorDataToSD(latest_kalman);
  have_kalman = true;

  // Stage update
  switch (stage) {
    case 0: if ((kalman_acceleration > 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 1; break;
    case 1: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 2; break;
    case 2: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > APOGEE)) stage = 3; break;
    case 3: if ((kalman_velocity < 0) && (kalman_altitude < APOGEE)) stage = 4; break;
    case 4: if (kalman_altitude < MIN_ALT) stage = 0; break;
  }
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  delay(200);

  msg = (char*)malloc(MAX_DATA);
  if (!msg) {
    Serial.println("ERROR: malloc(MAX_DATA) failed");
    while (1) delay(100);
  }

  // SD init
  Serial.println("Initializing SD card 1 (SDIO)...");
  sd_card.begin();
  Serial.println("Initializing SD card 2 (SPI)...");
  sd_card2.begin();

  write_sd_file_headers(sd_card);
  write_sd_file_headers(sd_card2);

  // Radio init
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);

  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);

  if (!rfm96w.init()) {
    Serial.println("RFM96W initialization failed");
    while (1) delay(100);
  }

  if (!rfm96w.setFrequency(RFM96W_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) delay(100);
  }

  rfm96w.setSignalBandwidth(RFM_BW_HZ);
  rfm96w.setSpreadingFactor(RFM_SF);
  rfm96w.setTxPower(20, false);
  rfm96w.setModeRx();

  Serial.printf("Radio OK. Freq=%.3f MHz BW=%lu SF=%u\n", RFM96W_FREQ, (unsigned long)RFM_BW_HZ, (unsigned)RFM_SF);

  // I2C / sensors
  Wire1.begin();
  Wire1.setClock(100000);
  Wire1.setTimeout(500000);
  gps2.setup();

  Wire2.begin();
  Wire2.setClock(400000);
  Wire2.setTimeout(500000);
  barometer.begin();

  delay(200);
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

  WDT_timings_t config;
  config.timeout = 30000; // 30 seconds
  wdt.begin(config);

  Serial.println("Setup complete.");
}

// ------------------- Main loop -------------------
void loop() {
  wdt.feed();
 if (rfm96w.available() && !telemetry_on) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;

    if (rfm96w.recv(buf, &len)) {
        buf[len] = 0; // Null terminate to treat as string
        
        // Use your existing startsWith helper for consistency
        if (buf[0] == '1') {
            telemetry_on = true;
            Serial.println("Telemetry: ENABLED");
            rfm96w.send((uint8_t*)initial_msg, 100);
            delay(50);
            rfm96w.waitPacketSent();
        } 
    }
}

if (!telemetry_on) {
    // Instead of delay(500), use a non-blocking timer if you want to print 
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        Serial.println("System is on standby");
        lastPrint = millis();
    }
    
    // IMPORTANT: Keep feeding the watchdog even in standby!
    wdt.feed(); 
    return; // Exit loop() early to skip sensor/TX logic
} else {
 // --------- A) RX handler (commands + ACKs) ----------
  if (rfm96w.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;

    if (rfm96w.recv(buf, &len)) {
      buf[len] = 0;
      const char* s = (const char*)buf;

      // 1) Telemetry ACK
      uint8_t aType;
      uint32_t aBaySeq, aGsSeq;
      if (parseAckTlm(s, aType, aBaySeq, aGsSeq)) {
        Serial.print("[Bay] RX ");
        Serial.println(s);

        if (waitingAck && aType == inflight_type && aBaySeq == inflight_seq) {
          waitingAck = false;

          if (last_gs_seq != 0 && aGsSeq != last_gs_seq + 1) {
            Serial.printf("[Bay] NOTE: GS seq jump %lu -> %lu (ACK loss or GS restart)\n",
                          (unsigned long)last_gs_seq, (unsigned long)aGsSeq);
          }
          last_gs_seq = aGsSeq;

          Serial.printf("[Bay] ACKED type=%u bay_seq=%lu gs_seq=%lu ✅\n",
                        (unsigned)aType, (unsigned long)aBaySeq, (unsigned long)aGsSeq);

          bay_seq++; // advance only on ACK
          pauseTelemetry(5);
        } else {
          Serial.println("[Bay] ACKTLM ignored (not matching inflight).");
        }
        // Do not treat ACK as command
      }
      else {
        // Any command triggers a quiet window
        pauseTelemetry(QUIET_AFTER_CMD_MS);

        uint32_t seq;
        float fNew;

        if (parseFreqCmd(s, seq, fNew)) {
          Serial.print("[Bay] RX ");
          Serial.println(s);

          // ACK on old freq
          char ack[64];
          snprintf(ack, sizeof(ack), "ACKFREQ,%lu,%.3f", (unsigned long)seq, fNew);
          Serial.print("[Bay] TX ");
          Serial.println(ack);
          sendAscii(ack);

          if (seq == lastFreqSeq && fNew == lastFreqNew) {
            Serial.println("[Bay] Duplicate FREQ seq; ACK re-sent, not switching again.");
          } else {
            lastFreqSeq = seq;
            lastFreqNew = fNew;

            delay(50);
            Serial.printf("[Bay] Switching to %.3f\n", fNew);
            setFreq(fNew);
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
          pauseTelemetry(100);
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
        } else if (buf[0] == '0') {
            telemetry_on = false;
            Serial.println("Telemetry: DISABLED");
        } else {
          Serial.print("[Bay] RX unknown: ");
          Serial.println(s);
        }
      }
    }
  }

  // --------- B) If waiting for ACK, handle timeout (NO resend) ----------
  if (waitingAck) {
    if ((int32_t)(millis() - ack_deadline_ms) > 0) {
      Serial.printf("[Bay] ACK TIMEOUT type=%u bay_seq=%lu (no resend)\n",
                    (unsigned)inflight_type, (unsigned long)inflight_seq);

      // Mark as lost and move on (prevents deadlock)
      waitingAck = false;
      bay_seq++;
      pauseTelemetry(2);
    } else {
      // Keep loop responsive while waiting
      delay(1);
      return;
    }
  }

  // --------- C) Respect quiet window ----------
  if ((int32_t)(millis() - quietUntilMs) < 0) {
    delay(1);
    return;
  }

  // --------- D) Update dt and timestamp ----------
  t_now = micros();
  dt = (double)(t_now - t_prev) * 1e-6;
  if (dt <= 0 || dt > 1.0) dt = 0.02;
  t_prev = t_now;

  char ts[20];
  getTimestamp(ts, sizeof(ts));

  // --------- E) Scheduled sensor updates (non-blocking pacing) ----------
  uint32_t nowMs = millis();

  if ((uint32_t)(nowMs - lastGpsMs) >= GPS_PERIOD_MS) {
    lastGpsMs = nowMs;
    updateGPS(ts);
  }

  if ((uint32_t)(nowMs - lastAltMs) >= ALT_PERIOD_MS) {
    lastAltMs = nowMs;
    updateALT(ts);
  }

  if ((uint32_t)(nowMs - lastImuMs) >= IMU_PERIOD_MS) {
    lastImuMs = nowMs;
    updateIMU(ts);
  }

  if ((uint32_t)(nowMs - lastKalmanMs) >= KALMAN_PERIOD_MS) {
    lastKalmanMs = nowMs;
    updateKalman(ts);
  }

  // --------- F) Transmit one packet (round-robin), then wait for ACK ----------
  if ((uint32_t)(nowMs - lastTxMs) < TX_GAP_MS) {
    // keep loop light
    delay(1);
    return;
  }

  // Choose next type that we actually have data for
  for (int i = 0; i < 4; i++) {
    uint8_t t = next_type_to_send;
    bool ok = false;

    switch (t) {
      case 0: ok = have_imu; break;
      case 1: ok = have_alt; break;
      case 2: ok = have_gps; break;
      case 3: ok = have_kalman; break;
    }

    // advance for next time regardless
    next_type_to_send = (uint8_t)((next_type_to_send + 1) & 0x03);

    if (!ok) continue;

    // Build payload using your existing serializer
    switch (t) {
      case 0: dataToString(latest_imu, msg); break;
      case 1: dataToString(latest_alt, msg); break;
      case 2: dataToString(latest_gps, msg); break;
      case 3: dataToString(latest_kalman, msg); break;
    }

    // Send + start waiting
    startTelemetryTxn(t, msg);
    break;
  }

  // lightweight yield
  delay(1);
  }

 
}
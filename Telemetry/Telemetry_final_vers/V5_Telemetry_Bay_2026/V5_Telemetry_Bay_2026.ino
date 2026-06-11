/*
 * V5_Telemetry_Bay_2026.ino
 *
 * Integrated Telemetry System for Teensy 4.1
 * Reads data from IMU (BNO085), Barometer (BMP581), and GPS sensors
 * Integrated Kalman filter
 * Logs all data to SD card in CSV format
 *
 * RTOS Architecture: TeensyThreads
 *
 * FIXES APPLIED:
 *  1. thread_imu: imu.update() moved inside while(1) and called unconditionally
 *     so the BNO085 is always polled — prevents internal buffer overflow/reset
 *     during standby.
 *  2. thread_imu: Debug imu.update() and Serial.printf that were orphaned outside
 *     the while(1) loop have been removed.
 *  3. SPI bus arbitration: sdMutex and radioMutex merged into a single spiMutex.
 *     sd_card2 (SPI, CS=10) and rfm96w (SPI) shared a physical bus but used
 *     separate mutexes, allowing concurrent access and bus corruption.
 */


#include <SPI.h>
#include <RH_RF95.h>
#include <Watchdog_t4.h>
#include <TeensyThreads.h>


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
#include <queue> // Or use a fixed-size ring buffer to avoid heap fragmentation


#define MAX_LOG_QUEUE 500
SensorData logQueue[MAX_LOG_QUEUE];
volatile int head = 0;
volatile int tail = 0;
volatile int count = 0;
Threads::Mutex queueMutex;


// ------------------- Hardware / modules -------------------
WDT_T4<WDT3> wdt;
SOAR_SD_CARD sd_card(254, true);   // Built-in, use SDIO
SOAR_SD_CARD sd_card2(10, false);  // External, use SPI
SOAR_RTC rtc;
SOAR_RTOS_GPS gps2;                // Wire1
Adafruit_GPS gps_hw(&Wire1);
BMP581Sensor barometer;            // Wire2
SOAR_BNO085 imu;                   // Wire
RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);


// ------------------- Radio params -------------------
static float RFM96W_FREQ = 421.62f;
static const uint32_t RFM_BW_HZ = 62500;
static const uint8_t  RFM_SF    = 7;
bool telemetry_on = false;
const char initial_msg[100] = "Callsign: KR4IJA | Team: 24\n";


// ------------------- Buffers / misc -------------------
// ------------------- Buffers / misc -------------------
char msg[MAX_DATA];    // Statically allocated buffer
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
const double sigma_j = 0.2;        /* process StdDev: TUNED */
const double sigma_s = 0.1666667;  /* altitude reading StdDev */
const double sigma_a = 0.179;      /* acceleration reading StdDev */
const int states = 3;
const int observations = 2;
const double MIN_ALT = 10;          /* trust sensors below this altitude [m] */
const double APOGEE = 3048;        // [m] 10k ft
kalmanFilter *filter = NULL;


// ------------------- Telemetry pause control -------------------
static uint32_t quietUntilMs = 0;
static const uint32_t QUIET_AFTER_CMD_MS    = 1200;
static const uint32_t QUIET_AFTER_SWITCH_MS = 1200;


// ------------------- Duplicate protection for FREQ -------------------
static uint32_t lastFreqSeq = 0;
static float    lastFreqNew = 0.0f;


// ------------------- Telemetry ACK protocol state -------------------
static uint32_t bay_seq = 1;  // Bay telemetry sequence (independent)
static const uint32_t TELEMETRY_PERIOD_MS = 20;


static uint8_t  inflight_type = 0;
static uint32_t inflight_seq  = 0;
static char inflight_payload[RH_RF95_MAX_MESSAGE_LEN + 1];
uint32_t lastcallsign = 0;


// Transmit order: 0=IMU, 1=ALT, 2=GPS, 3=KALMAN
static uint8_t next_type_to_send = 2;


// ------------------- Scheduling Intervals -------------------
static const uint32_t GPS_PERIOD_MS    = 100;      // 10 Hz
static float ALT_PERIOD_MS    = 1000.0f/240.0f;        // 240 Hz
static float IMU_PERIOD_MS    = 1000.0f/400.0f;        // 400 Hz
static const uint32_t KALMAN_PERIOD_MS = 50;       // 20 Hz
static uint32_t lastGpsMs = 0;
static uint32_t lastAltMs = 0;
static uint32_t lastImuMs = 0;
static uint32_t lastKalmanMs = 0;
static uint32_t lastTxMs = 0;


// ------------------- Thread Protection (Mutexes) -------------------
// FIX #3: sdMutex and radioMutex merged into a single spiMutex.
// sd_card2 (SPI, CS=10) and rfm96w both use the same physical SPI bus.
// Using separate mutexes previously allowed concurrent access → bus corruption.
Threads::Mutex dataMutex;  // Protects latest_* sensor snapshots
Threads::Mutex spiMutexSD;   // Protects ALL SPI bus access (sd_card2 + RFM96W)
Threads::Mutex spiMutexRF;   // Protects ALL SPI bus access (sd_card2 + RFM96W)
Threads::Mutex i2cMutexIMU; // IMU
Threads::Mutex i2cMutexGPS; // GPS
Threads::Mutex i2cMutexALT; // ALT


static SensorData latest_gps{};
static SensorData latest_alt{};
static SensorData latest_imu{};
static SensorData latest_kalman{};
static bool have_gps = false, have_alt = false, have_imu = false, have_kalman = false;


// ---------------- Helpers ----------------
// Pushes data into the queue
void pushToQueue(SensorData data) {
    queueMutex.lock();
    logQueue[head] = data;
    head = (head + 1) % MAX_LOG_QUEUE;
    if (count < MAX_LOG_QUEUE) {
        count++;
    } else {
        tail = (tail + 1) % MAX_LOG_QUEUE;
    }


    static uint32_t pushes = 0;
pushes++;


if ((pushes % 100) == 0) {
    Serial.printf("[QUEUE] pushes=%lu count=%d\n",
                  pushes, count);
}
    queueMutex.unlock();
}

static void getTimestamp(char* ts, size_t n) {
  rtc.getTimestamp(ts, n, true);
}

static bool startsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}


static bool parseFreqCmd(const char* s, uint32_t& seqOut, float& fOut) {
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
  if (!startsWith(s, "PING,")) return false;
  seqOut = (uint32_t)strtoul(s + 5, nullptr, 10);
  return true;
}


static bool parseRebootCmd(const char* s, uint32_t& seqOut) {
  if (!startsWith(s, "REBOOT,")) return false;
  seqOut = (uint32_t)strtoul(s + 7, nullptr, 10);
  return true;
}


static bool parsePowerCmd(const char* s, uint32_t& seqOut, int& powerOut) {
  if (!startsWith(s, "POWER,")) return false;
  const char* p = s + 6;
  char* end1 = nullptr;
  unsigned long seq = strtoul(p, &end1, 10);
  if (!end1 || *end1 != ',') return false;
  int pow = (int)strtod(end1 + 1, nullptr);
  seqOut = (uint32_t)seq;
  powerOut = pow;
  return true;
}


static void sendAscii(const char* m) {
  spiMutexRF.lock();  // FIX #3: was radioMutex
  rfm96w.setModeIdle();
  rfm96w.send((uint8_t*)m, (uint8_t)strlen(m));
  rfm96w.waitPacketSent();
  rfm96w.setModeRx();
  spiMutexRF.unlock();
}


static void setFreq(float mhz) {
  spiMutexRF.lock();  // FIX #3: was radioMutex
  rfm96w.setModeIdle();
  if (rfm96w.setFrequency(mhz)) {
    RFM96W_FREQ = mhz;
  }
  rfm96w.setModeRx();
  spiMutexRF.unlock();
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


static void buildTlmPacketTrunc(uint8_t type, uint32_t seq, const char* payload, char* out, size_t outSize) {
  if (!payload) payload = "";
  int hn = snprintf(out, outSize, "Callsign: KR4IJA | TLM,%u,%lu,", (unsigned)type, (unsigned long)seq);
  if (hn <= 0 || (size_t)hn >= outSize) {
    out[0] = '\0';
    return;
  }
  size_t rem = outSize - (size_t)hn - 1;
  strncpy(out + hn, payload, rem);
  out[hn + rem] = '\0';
}


static void startTelemetryTxn(uint8_t type, const char* payload) {
  inflight_type = type;
  inflight_seq  = bay_seq;


  strncpy(inflight_payload, payload ? payload : "", sizeof(inflight_payload) - 1);
  inflight_payload[sizeof(inflight_payload) - 1] = '\0';


  static char pkt[RH_RF95_MAX_MESSAGE_LEN + 1];
  buildTlmPacketTrunc(inflight_type, inflight_seq, inflight_payload, pkt, sizeof(pkt));


  if (pkt[0] == '\0') {
    Serial.println("[Bay] ERROR: could not build TLM packet.");
    return;
  }


  Serial.printf("[Bay] TX %s\n", pkt);
  // uint8_t transmitpacket[sizeof(pkt)];
  // memcpy(transmitpacket, &pkt, sizeof(pkt));


  // spiMutex.lock();  // FIX #3: was radioMutex
  // rfm96w.setModeIdle();
  // rfm96w.send(*transmitpacket, (uint8_t)strlen(transmitpacket));
  // rfm96w.waitPacketSent();
  // rfm96w.setModeRx();
  // spiMutex.unlock();


  // sendAscii(pkt);
}
static void updateGPS() {
     // i2cMutexGPS.lock();
    char nmea_tmp[100];
    gps2.GET_NMEA(nmea_tmp);
    if (telemetry_on) {


      SensorData local_gps;
      local_gps.bay_seq = bay_seq;
      local_gps.type = GPS;
      getTimestamp(local_gps.timestamp, sizeof(local_gps.timestamp));
      strncpy(local_gps.data.gps.nmea, nmea_tmp, sizeof(local_gps.data.gps.nmea) - 1);
      local_gps.data.gps.nmea[sizeof(local_gps.data.gps.nmea) - 1] = '\0';


      // dataMutex.lock();
      latest_gps = local_gps;
      have_gps = true;
      // dataMutex.unlock();


      pushToQueue(local_gps);
    }
    // i2cMutexGPS.unlock();
    // threads.delay(GPS_PERIOD_MS);
//   }
// }
}

static void updateALT() {
// void thread_alt() {
//   while (1) {
    // i2cMutexALT.lock();
    float cur_alt   = barometer.get_altitude() - i_altitude;
    float cur_press = barometer.get_pressure();
    float cur_temp  = barometer.get_temperature();


    if (telemetry_on) {


      // if (cur_alt == 0) {
      //   barometer.begin();
      // }


      SensorData local_alt;
      local_alt.bay_seq = bay_seq;
      local_alt.type = ALTIMETER;
      getTimestamp(local_alt.timestamp, sizeof(local_alt.timestamp));
      local_alt.data.alt.altitude = cur_alt;
      local_alt.data.alt.temp     = cur_temp;
      local_alt.data.alt.pressure = cur_press;


      // dataMutex.lock();
      altitude    = cur_alt;
      pressure    = cur_press;
      temperature = cur_temp;
      latest_alt  = local_alt;
      have_alt    = true;
      // dataMutex.unlock();


      pushToQueue(local_alt);
    }
    // i2cMutexALT.unlock();
    // threads.delay(ALT_PERIOD_MS);
//   }
// }
}

static void updateIMU() {
// void thread_imu() {
//   while(1) {
    // i2cMutexIMU.lock();
    // Always drain the BNO085 regardless of telemetry state
    imu.update();


    if (telemetry_on) {


      SensorData local_imu;
      local_imu.bay_seq = bay_seq;
      local_imu.type = IMU;
      getTimestamp(local_imu.timestamp, sizeof(local_imu.timestamp));


      local_imu.data.imu.accel[0]   = imu.sensorData.acceleration.x;
      local_imu.data.imu.accel[1]   = imu.sensorData.acceleration.y;
      local_imu.data.imu.accel[2]   = imu.sensorData.acceleration.z;
      local_imu.data.imu.linear[0]  = imu.sensorData.linearAcceleration.x;
      local_imu.data.imu.linear[1]  = imu.sensorData.linearAcceleration.y;
      local_imu.data.imu.linear[2]  = imu.sensorData.linearAcceleration.z;
      local_imu.data.imu.gravity[0] = imu.sensorData.gravity.x;
      local_imu.data.imu.gravity[1] = imu.sensorData.gravity.y;
      local_imu.data.imu.gravity[2] = imu.sensorData.gravity.z;
      local_imu.data.imu.quat[0]    = imu.sensorData.orientation.w;
      local_imu.data.imu.quat[1]    = imu.sensorData.orientation.x;
      local_imu.data.imu.quat[2]    = imu.sensorData.orientation.y;
      local_imu.data.imu.quat[3]    = imu.sensorData.orientation.z;
      local_imu.data.imu.gyro[0]    = imu.sensorData.gyroscope.x;
      local_imu.data.imu.gyro[1]    = imu.sensorData.gyroscope.y;
      local_imu.data.imu.gyro[2]    = imu.sensorData.gyroscope.z;


      // dataMutex.lock();
      latest_imu = local_imu;
      have_imu   = true;
      // dataMutex.unlock();
      pushToQueue(local_imu);
    }
    // i2cMutexIMU.unlock();
    // threads.delay(IMU_PERIOD_MS);
//   }
// }
}

static void updateKalman() {
// void thread_kalman() {
//   while (1) {
    if (telemetry_on) {
      uint32_t thread_t_prev = micros();
      // dataMutex.lock();
      bool can_run = have_alt && have_imu;
      float k_alt      = altitude;
      // dataMutex.unlock();
      // dataMutex.lock();
      float k_quat[4]  = { latest_imu.data.imu.quat[0],   latest_imu.data.imu.quat[1],
                            latest_imu.data.imu.quat[2],   latest_imu.data.imu.quat[3] };
      // dataMutex.unlock();
      // dataMutex.lock();
      float k_linear[3]= { latest_imu.data.imu.linear[0], latest_imu.data.imu.linear[1],
                            latest_imu.data.imu.linear[2] };
      // dataMutex.unlock();


      if (can_run) {
        uint32_t t_now = micros();
        double dt = (double)(t_now - thread_t_prev) * 1e-6;
        if (dt <= 0 || dt > 1.0) dt = 0.05;
        thread_t_prev = t_now;


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
        setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) * 1.0      * pow(dt, 2));


        if (k_alt < MIN_ALT) {
          setElement(filter->P_k_prev, 2, 2, 100);
          setElement(filter->P_k_prev, 3, 3, 100);
        }


        setElement(quat, 1, 1, k_quat[0]);
        setElement(quat, 2, 1, k_quat[1]);
        setElement(quat, 3, 1, k_quat[2]);
        setElement(quat, 4, 1, k_quat[3]);


        setElement(acc, 1, 1, k_linear[0]);
        setElement(acc, 2, 1, k_linear[1]);
        setElement(acc, 3, 1, k_linear[2]);


        vectorComponent(acc, quat, dir, &magnitude);


        setElement(filter->z_k, 1, 1, k_alt);
        setElement(filter->z_k, 2, 1, magnitude);


        kalmanFilterPredict(filter);
        kalmanFilterUpdate(filter);


        getElement(filter->x_k, 1, 1, &kalman_altitude);
        getElement(filter->x_k, 2, 1, &kalman_velocity);
        getElement(filter->x_k, 3, 1, &kalman_acceleration);


        SensorData local_kalman;
        local_kalman.bay_seq = bay_seq;
        local_kalman.type = KALMAN;
        getTimestamp(local_kalman.timestamp, sizeof(local_kalman.timestamp));
        local_kalman.data.kalman.kalman_altitude     = kalman_altitude;
        local_kalman.data.kalman.kalman_velocity     = kalman_velocity;
        local_kalman.data.kalman.kalman_acceleration = kalman_acceleration;
        local_kalman.data.kalman.kalman_state        = stage;


        // dataMutex.lock();
        latest_kalman = local_kalman;
        have_kalman   = true;
        // dataMutex.unlock();


        pushToQueue(local_kalman);


        // Stage update
        switch (stage) {
          case 0: if ((kalman_acceleration > 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 1; break;
          case 1: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 2; break;
          case 2: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > APOGEE))  stage = 3; break;
          case 3: if ((kalman_velocity < 0)     && (kalman_altitude < APOGEE))                           stage = 4; break;
          case 4: if (kalman_altitude < MIN_ALT)                                                         stage = 0; break;
        }

}
    }
}

// ------------------- SD helpers -------------------
void write_sd_file_headers(SOAR_SD_CARD& sd) {
  sd.appendFile(IMU_FILEPATH,       "\n\nInitializing\n\ntime_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.appendFile(ALTIMETER_FILEPATH, "\n\nInitializing\n\ntime_stamp,altitude,temperature,pressure\n");
  sd.appendFile(GPS_FILEPATH,       "\n\nInitializing\n\ntime_stamp,gps_data\n");
  sd.appendFile(KALMAN_FILEPATH,    "\n\nInitializing\n\ntime_stamp, state, altitude,velocity,acceleration\n");
}


void writeToBothCards(const char* filename, const char* data) {
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
        sensor_data.data.imu.accel[0],   sensor_data.data.imu.accel[1],   sensor_data.data.imu.accel[2],
        sensor_data.data.imu.linear[0],  sensor_data.data.imu.linear[1],  sensor_data.data.imu.linear[2],
        sensor_data.data.imu.gravity[0], sensor_data.data.imu.gravity[1], sensor_data.data.imu.gravity[2],
        sensor_data.data.imu.quat[0],    sensor_data.data.imu.quat[1],    sensor_data.data.imu.quat[2],    sensor_data.data.imu.quat[3],
        sensor_data.data.imu.gyro[0],    sensor_data.data.imu.gyro[1],    sensor_data.data.imu.gyro[2]);
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
      len = snprintf(dataBuffer, sizeof(dataBuffer), "%s, %d, %.6f, %.6f, %.6f\n",
        sensor_data.timestamp,
        sensor_data.data.kalman.kalman_state,
        sensor_data.data.kalman.kalman_altitude,
        sensor_data.data.kalman.kalman_velocity,
        sensor_data.data.kalman.kalman_acceleration);
      break;
  }


  if (filename && len > 0 && (size_t)len < sizeof(dataBuffer)) {
    writeToBothCards(filename, dataBuffer);
  }
}

// ------------------- Threads -------------------


// void thread_readData() {
//   while (1) {
//     i2cMutexGPS.lock();
//     char nmea_tmp[100];
//     gps2.GET_NMEA(nmea_tmp);
//     if (telemetry_on) {


//       SensorData local_gps;
//       local_gps.bay_seq = bay_seq;
//       local_gps.type = GPS;
//       getTimestamp(local_gps.timestamp, sizeof(local_gps.timestamp));
//       strncpy(local_gps.data.gps.nmea, nmea_tmp, sizeof(local_gps.data.gps.nmea) - 1);
//       local_gps.data.gps.nmea[sizeof(local_gps.data.gps.nmea) - 1] = '\0';


//       dataMutex.lock();
//       latest_gps = local_gps;
//       have_gps = true;
//       dataMutex.unlock();


//       pushToQueue(local_gps);
//     }
//     i2cMutexGPS.unlock();
//     // threads.delay(GPS_PERIOD_MS);
// //   }
// // }


// // void thread_alt() {
// //   while (1) {
//     i2cMutexALT.lock();
//     float cur_alt   = barometer.get_altitude() - i_altitude;
//     float cur_press = barometer.get_pressure();
//     float cur_temp  = barometer.get_temperature();


//     if (telemetry_on) {


//       // if (cur_alt == 0) {
//       //   barometer.begin();
//       // }


//       SensorData local_alt;
//       local_alt.bay_seq = bay_seq;
//       local_alt.type = ALTIMETER;
//       getTimestamp(local_alt.timestamp, sizeof(local_alt.timestamp));
//       local_alt.data.alt.altitude = cur_alt;
//       local_alt.data.alt.temp     = cur_temp;
//       local_alt.data.alt.pressure = cur_press;


//       dataMutex.lock();
//       altitude    = cur_alt;
//       pressure    = cur_press;
//       temperature = cur_temp;
//       latest_alt  = local_alt;
//       have_alt    = true;
//       dataMutex.unlock();


//       pushToQueue(local_alt);
//     }
//     i2cMutexALT.unlock();
//     // threads.delay(ALT_PERIOD_MS);
// //   }
// // }


// // void thread_imu() {
// //   while(1) {
//     i2cMutexIMU.lock();
//     // Always drain the BNO085 regardless of telemetry state
//     imu.update();


//     if (telemetry_on) {


//       SensorData local_imu;
//       local_imu.bay_seq = bay_seq;
//       local_imu.type = IMU;
//       getTimestamp(local_imu.timestamp, sizeof(local_imu.timestamp));


//       local_imu.data.imu.accel[0]   = imu.sensorData.acceleration.x;
//       local_imu.data.imu.accel[1]   = imu.sensorData.acceleration.y;
//       local_imu.data.imu.accel[2]   = imu.sensorData.acceleration.z;
//       local_imu.data.imu.linear[0]  = imu.sensorData.linearAcceleration.x;
//       local_imu.data.imu.linear[1]  = imu.sensorData.linearAcceleration.y;
//       local_imu.data.imu.linear[2]  = imu.sensorData.linearAcceleration.z;
//       local_imu.data.imu.gravity[0] = imu.sensorData.gravity.x;
//       local_imu.data.imu.gravity[1] = imu.sensorData.gravity.y;
//       local_imu.data.imu.gravity[2] = imu.sensorData.gravity.z;
//       local_imu.data.imu.quat[0]    = imu.sensorData.orientation.w;
//       local_imu.data.imu.quat[1]    = imu.sensorData.orientation.x;
//       local_imu.data.imu.quat[2]    = imu.sensorData.orientation.y;
//       local_imu.data.imu.quat[3]    = imu.sensorData.orientation.z;
//       local_imu.data.imu.gyro[0]    = imu.sensorData.gyroscope.x;
//       local_imu.data.imu.gyro[1]    = imu.sensorData.gyroscope.y;
//       local_imu.data.imu.gyro[2]    = imu.sensorData.gyroscope.z;


//       dataMutex.lock();
//       latest_imu = local_imu;
//       have_imu   = true;
//       dataMutex.unlock();
//       pushToQueue(local_imu);
//     }
//     i2cMutexIMU.unlock();
//     // threads.delay(IMU_PERIOD_MS);
// //   }
// // }


// // void thread_kalman() {
// //   while (1) {
//     if (telemetry_on) {
//       uint32_t thread_t_prev = micros();
//       dataMutex.lock();
//       bool can_run = have_alt && have_imu;
//       float k_alt      = altitude;
//       // dataMutex.unlock();
//       // dataMutex.lock();
//       float k_quat[4]  = { latest_imu.data.imu.quat[0],   latest_imu.data.imu.quat[1],
//                             latest_imu.data.imu.quat[2],   latest_imu.data.imu.quat[3] };
//       // dataMutex.unlock();
//       // dataMutex.lock();
//       float k_linear[3]= { latest_imu.data.imu.linear[0], latest_imu.data.imu.linear[1],
//                             latest_imu.data.imu.linear[2] };
//       dataMutex.unlock();


//       if (can_run) {
//         uint32_t t_now = micros();
//         double dt = (double)(t_now - thread_t_prev) * 1e-6;
//         if (dt <= 0 || dt > 1.0) dt = 0.05;
//         thread_t_prev = t_now;


//         setElement(filter->F_k, 1, 2, dt);
//         setElement(filter->F_k, 1, 3, 0.5 * dt * dt);
//         setElement(filter->F_k, 2, 2, 1);
//         setElement(filter->F_k, 2, 3, dt);


//         setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
//         setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
//         setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
//         setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
//         setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
//         setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
//         setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
//         setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
//         setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) * 1.0      * pow(dt, 2));


//         // if (k_alt < MIN_ALT) {
//         //   setElement(filter->P_k_prev, 2, 2, 100);
//         //   setElement(filter->P_k_prev, 3, 3, 100);
//         // }


//         setElement(quat, 1, 1, k_quat[0]);
//         setElement(quat, 2, 1, k_quat[1]);
//         setElement(quat, 3, 1, k_quat[2]);
//         setElement(quat, 4, 1, k_quat[3]);


//         setElement(acc, 1, 1, k_linear[0]);
//         setElement(acc, 2, 1, k_linear[1]);
//         setElement(acc, 3, 1, k_linear[2]);


//         vectorComponent(acc, quat, dir, &magnitude);


//         setElement(filter->z_k, 1, 1, k_alt);
//         setElement(filter->z_k, 2, 1, magnitude);


//         kalmanFilterPredict(filter);
//         kalmanFilterUpdate(filter);


//         getElement(filter->x_k, 1, 1, &kalman_altitude);
//         getElement(filter->x_k, 2, 1, &kalman_velocity);
//         getElement(filter->x_k, 3, 1, &kalman_acceleration);


//         SensorData local_kalman;
//         local_kalman.bay_seq = bay_seq;
//         local_kalman.type = KALMAN;
//         getTimestamp(local_kalman.timestamp, sizeof(local_kalman.timestamp));
//         local_kalman.data.kalman.kalman_altitude     = kalman_altitude;
//         local_kalman.data.kalman.kalman_velocity     = kalman_velocity;
//         local_kalman.data.kalman.kalman_acceleration = kalman_acceleration;
//         local_kalman.data.kalman.kalman_state        = stage;


//         dataMutex.lock();
//         latest_kalman = local_kalman;
//         have_kalman   = true;
//         dataMutex.unlock();


//         pushToQueue(local_kalman);


//         // Stage update
//         switch (stage) {
//           case 0: if ((kalman_acceleration > 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 1; break;
//           case 1: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > MIN_ALT)) stage = 2; break;
//           case 2: if ((kalman_acceleration < 0) && (kalman_velocity > 0) && (kalman_altitude > APOGEE))  stage = 3; break;
//           case 3: if ((kalman_velocity < 0)     && (kalman_altitude < APOGEE))                           stage = 4; break;
//           case 4: if (kalman_altitude < MIN_ALT)                                                         stage = 0; break;
//         }
//       }
//     }
//     threads.delay(KALMAN_PERIOD_MS);
//   }
// }


void thread_radio_tx() {
  while (1) {
    if (telemetry_on) {
      uint32_t nowMs = millis();


      if (nowMs - lastcallsign >= 600000) {
        spiMutexRF.lock();
        rfm96w.setModeIdle();
        rfm96w.send((uint8_t*)initial_msg, strlen(initial_msg));
        rfm96w.waitPacketSent();
        rfm96w.setModeRx();
        spiMutexRF.unlock();
        Serial.println("[Bay TX] Sent callsign");


        lastcallsign = millis();
      }
      // Check if we are past the quiet window enforced by RX commands
      if ((int32_t)(nowMs - quietUntilMs) >= 0) {


        uint8_t t  = next_type_to_send;
        bool    ok = false;


        dataMutex.lock();
        SensorData txPayload;
        switch (t) {
          case 0:
            if (have_imu) {
              txPayload = latest_imu;
              ok = true;
            }
            break;
          case 1:
            if (have_alt) {
              txPayload = latest_alt;
              ok = true;
            }
            break;
          case 2:
            if (have_gps) {
              txPayload = latest_gps;
              ok = true;
            }
            break;
          case 3:
            if (have_kalman) {
              txPayload = latest_kalman;
              ok = true;
            }
            break;
        }
        dataMutex.unlock();


        next_type_to_send = (uint8_t)((next_type_to_send + 1) & 0x03);


        if (ok) {
          // Send binary SensorData packet directly
          size_t packetSize = sizeof(SensorData);
          const char* typeStr = "?";
          switch (t) {
            case 0: typeStr = "IMU"; break;
            case 1: typeStr = "ALT"; break;
            case 2: typeStr = "GPS"; break;
            case 3: typeStr = "KAL"; break;
          }
        uint8_t packet[128];
        uint16_t idx = 0;

        // Header
        memcpy(packet + idx, &txPayload.type, sizeof(txPayload.type));
        idx += sizeof(txPayload.type);

        memcpy(packet + idx, &txPayload.bay_seq, sizeof(txPayload.bay_seq));
        idx += sizeof(txPayload.bay_seq);
        
        memcpy(packet + idx, txPayload.timestamp, sizeof(txPayload.timestamp));
        idx += sizeof(txPayload.timestamp);
        // Payload
        switch (txPayload.type)
        {
            case IMU:
                memcpy(packet + idx,
                      &txPayload.data.imu,
                      sizeof(imu_packet));
                idx += sizeof(imu_packet);
                break;

            case ALTIMETER:
                memcpy(packet + idx,
                      &txPayload.data.alt,
                      sizeof(altimeter_packet));
                idx += sizeof(altimeter_packet);
                break;

            case GPS:
                memcpy(packet + idx,
                      &txPayload.data.gps,
                      sizeof(gps_packet));
                idx += sizeof(gps_packet);
                break;

            case KALMAN:
                memcpy(packet + idx,
                      &txPayload.data.kalman,
                      sizeof(kalman_packet));
                idx += sizeof(kalman_packet);
                break;
        }

        spiMutexRF.lock();
        rfm96w.setModeIdle();
        rfm96w.send(packet, idx);
        rfm96w.waitPacketSent();
        rfm96w.setModeRx();
        spiMutexRF.unlock();

        Serial.printf("[Bay TX] Type=%u Size=%u\n",
                      txPayload.type,
                      idx);

      
          bay_seq++;
        }
      }
    }
    threads.delay(TELEMETRY_PERIOD_MS);
  }
}


// ------------------- Setup -------------------
void setup() {
  Serial.begin(2000000);
  while (!Serial && millis() < 3000) {}
  delay(200);


  // Print struct size for debugging
  Serial.printf("[Bay SETUP] sizeof(SensorData) = %u bytes\n", (unsigned)sizeof(SensorData));




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


  Serial.printf("Radio OK. Freq=%.3f MHz BW=%lu SF=%u\n",
    RFM96W_FREQ, (unsigned long)RFM_BW_HZ, (unsigned)RFM_SF);


  // I2C / sensors
  Wire.begin();
  Wire.setClock(400000);
  Wire.setTimeout(500000);


  Wire1.begin();
  Wire1.setClock(400000);
  Wire1.setTimeout(500000);
  gps2.setup();


  Wire2.begin();
  Wire2.setClock(100000);
  Wire2.setTimeout(500000);
  barometer.begin();


  delay(200);
  i_altitude = barometer.get_altitude();


  quat = matrixCreate(4, 1);
  dir  = matrixCreate(3, 1);
  acc  = matrixCreate(3, 1);
  setElement(dir, Z_UP, 1, 1);


  filter = kalmanFilterCreate(states, observations);


  /* F_k for constant acceleration assumption */
  setElement(filter->F_k, 1, 1, 1);
  setElement(filter->F_k, 3, 3, 1);
  setElement(filter->H_k, 1, 1, 1);
  setElement(filter->H_k, 2, 3, 1);


  /* reading covariance */
  setElement(filter->R_k, 1, 1, sigma_s * sigma_s);
  setElement(filter->R_k, 2, 2, sigma_a * sigma_a);


  /* uncertainty of initial velocity and acceleration */
  setElement(filter->P_k_prev, 2, 2, 100);
  setElement(filter->P_k_prev, 3, 3, 100);


  WDT_timings_t config;
  config.timeout = 5000; // 5 s
  wdt.begin(config);


  // --- Start Threads ---
  // threads.addThread(thread_gps, nullptr, 4096);
  // threads.addThread(thread_alt, nullptr, 4096);
  // threads.addThread(thread_imu, nullptr, 4096);
  // threads.addThread(thread_kalman, nullptr, 4096);
  // threads.addThread(thread_readData, nullptr, 8912);
  threads.addThread(thread_radio_tx, nullptr, 8912);
  threads.addThread(thread_sd_store, nullptr, 8912);


  // Set execution slice to 1ms to ensure 240Hz and 400Hz threads hit execution targets.
  threads.setSliceMicros(1000);


  Serial.println("Setup complete. Threads running.");
}


void thread_sd_store() {
    while (1) {
      // if (telemetry_on) {
        bool hasData = false;
        SensorData dataToLog;


      queueMutex.lock();

      if (count > 0) {
          dataToLog = logQueue[tail];
          tail = (tail + 1) % MAX_LOG_QUEUE;
          count--;
          hasData = true;
      }

      queueMutex.unlock();

      if (hasData) {
          spiMutexSD.lock();
          writeSensorDataToSD(dataToLog);
          spiMutexSD.unlock();
      }
      threads.delay(10);
    }
}


// ------------------- Main loop (Watchdog & RX Handler) -------------------
void loop() {
  wdt.feed();


  spiMutexRF.lock();  // FIX #3: was radioMutex
  bool has_radio_data = rfm96w.available();
  spiMutexRF.unlock();


  if (has_radio_data) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;


    spiMutexRF.lock();  // FIX #3: was radioMutex
    bool received = rfm96w.recv(buf, &len);
    spiMutexRF.unlock();


    if (received) {
      buf[len] = 0; // Null terminate
      const char* s = (const char*)buf;


      uint32_t seq;
      float    fNew;
      int      powerLevel;


      if (buf[0] == '1' && !telemetry_on) {
        telemetry_on = true;
        Serial.println("Telemetry: ENABLED");


        spiMutexRF.lock();  // FIX #3: was radioMutex
        rfm96w.setModeIdle();
        rfm96w.send((uint8_t*)initial_msg, strlen(initial_msg));
        rfm96w.waitPacketSent();
        rfm96w.setModeRx();
        spiMutexRF.unlock();


        lastcallsign = millis();
      }
      else if (buf[0] == '0' && telemetry_on) {
        telemetry_on = false;
        Serial.println("Telemetry: DISABLED");
        char* offmsg = "Telemetry off";
        sendAscii(offmsg);
      }
      else if (parseFreqCmd(s, seq, fNew)) {
        Serial.printf("[Bay] RX %s\n", s);
        char ack[64];
        snprintf(ack, sizeof(ack), "ACKFREQ,%lu,%.3f", (unsigned long)seq, fNew);
        sendAscii(ack);  // acquires spiMutex internally


        if (seq != lastFreqSeq || fNew != lastFreqNew) {
          lastFreqSeq = seq;
          lastFreqNew = fNew;
          delay(50);
          Serial.printf("[Bay] Switching to %.3f\n", fNew);
          setFreq(fNew);  // acquires spiMutex internally
          pauseTelemetry(QUIET_AFTER_SWITCH_MS);
        }
      }
      else if (parsePowerCmd(s, seq, powerLevel)) {
        Serial.printf("[Bay] RX %s\n", s);
        char ack[64];
        snprintf(ack, sizeof(ack), "ACKPOWER,%lu,%d", (unsigned long)seq, powerLevel);
        sendAscii(ack);  // acquires spiMutex internally
        Serial.printf("[Bay] Sent ACKPOWER seq=%lu power=%d dBm ⚡\n", (unsigned long)seq, powerLevel);


        delay(50);
        Serial.printf("[Bay] Setting power to %d\n", powerLevel);


        spiMutexRF.lock();  // FIX #3: was radioMutex
        rfm96w.setTxPower(powerLevel, false);
        spiMutexRF.unlock();


        pauseTelemetry(QUIET_AFTER_SWITCH_MS);
      }
      else if (parsePingCmd(s, seq)) {
        Serial.printf("[Bay] RX %s\n", s);
        char ack[32];
        snprintf(ack, sizeof(ack), "ACKPING,%lu", (unsigned long)seq);
        sendAscii(ack);  // acquires spiMutex internally
        Serial.printf("[Bay] Sent ACKPING seq=%lu 🏓\n", (unsigned long)seq);
        pauseTelemetry(100);
      }
      else if (parseRebootCmd(s, seq)) {
        Serial.printf("[Bay] RX %s\n", s);
        char ack[40];
        snprintf(ack, sizeof(ack), "ACKREBOOT,%lu", (unsigned long)seq);
        sendAscii(ack);  // acquires spiMutex internally
        Serial.printf("[Bay] Sent ACKREBOOT seq=%lu 🔄\n", (unsigned long)seq);
        delay(200);
        Serial.println("[Bay] Rebooting...");
        teensyReboot();
      }
    }
  }


  if (!telemetry_on) {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
      Serial.println("System is on standby");
      lastPrint = millis();
    }
  }
         char ts[20];
  getTimestamp(ts, sizeof(ts));

  // --------- E) Scheduled sensor updates (non-blocking pacing) ----------
  uint32_t nowMs = millis();

  if ((uint32_t)(nowMs - lastGpsMs) >= GPS_PERIOD_MS) {
    lastGpsMs = nowMs;
    updateGPS();
  }

  if ((uint32_t)(nowMs - lastAltMs) >= ALT_PERIOD_MS) {
    lastAltMs = nowMs;
    updateALT();
  }

  if ((uint32_t)(nowMs - lastImuMs) >= IMU_PERIOD_MS) {
    lastImuMs = nowMs;
    updateIMU();
  }

  if ((uint32_t)(nowMs - lastKalmanMs) >= KALMAN_PERIOD_MS) {
    lastKalmanMs = nowMs;
    updateKalman();
  }
 
  // Yield the rest of the 10ms boundary to background threads
  threads.delay(10);
}





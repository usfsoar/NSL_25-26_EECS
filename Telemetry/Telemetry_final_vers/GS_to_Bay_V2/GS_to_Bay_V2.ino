  // GS sending to Bay


#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <string.h>
#include <stdlib.h>  // strtoul, strtod
#include "sensor_data_types.h"


#include "SOAR_GS_SD_CARD.h"
#include "_config.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


#define RFM96W_SCK   D8
#define RFM96W_MISO  D9
#define RFM96W_MOSI  D10


#define RFM96W_CS   D0
#define RFM96W_RST  D2
#define RFM96W_INT  D3


// ---------- Radio settings (MUST match Bay) ----------
static float currentFreqMHz = 421.62f;
static const uint32_t RFM_BW_HZ = 62500;
static const uint8_t  RFM_SF    = 7;


// ---------- Sequences ----------
static uint32_t g_seq = 1;     // command transaction seq (FREQ/PING/REBOOT)
static uint32_t gs_seq = 1;    // ACKTLM sequence (independent from Bay)
static uint32_t last_ping_seq = 0;  // Track seq of last standalone PING sent
static uint32_t last_power_seq = 0; // Track seq of last POWER command
static uint32_t last_reboot_seq = 0; // Track seq of last REBOOT command


RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);
SOAR_SD_CARD sd(D1, false);


// ---------- RTOS queues ----------
enum CmdType : uint8_t { CMD_SET_FREQ, CMD_SEND_COMM };


struct RadioCmd {
  CmdType type;
  float freq_mhz;
  uint8_t data[80];  // slightly larger to fit ACK strings comfortably
  uint8_t len;
};


struct RadioRx {
  uint8_t data[RH_RF95_MAX_MESSAGE_LEN + 1];
  uint8_t len;
};


QueueHandle_t cmdQueue;
QueueHandle_t rxQueue;
TaskHandle_t radioTaskHandle = nullptr;


// ---------------- Freq txn state machine ----------------
enum TxnState : uint8_t {
  TXN_IDLE,
  TXN_WAIT_ACKFREQ,
  TXN_SWITCH_TO_NEW,
  TXN_SWITCH_DELAY,
  TXN_WAIT_ACKPING,
  TXN_DONE,
  TXN_FAIL
};


struct FreqTxn {
  TxnState state = TXN_IDLE;
  uint32_t seq = 0;
  float f_old = 0.0f;
  float f_new = 0.0f;


  uint32_t t_deadline_ms = 0;
  uint8_t retries = 0;


  uint32_t ack_timeout_ms  = 800;
  uint32_t ping_timeout_ms = 800;
  uint8_t  max_retries     = 3;
};


static FreqTxn freqTxn;


// ---------- Helpers ----------
static bool startsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}


static bool validFreq433(float f) {
  return (f >= 420.6f && f <= 438.0f);
}


static void radioSendAscii(const char* msg) {
  RadioCmd c{};
  c.type = CMD_SEND_COMM;
  c.len = (uint8_t)min((int)strlen(msg), (int)sizeof(c.data));
  memcpy(c.data, msg, c.len);
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
}


static void radioSendAsciiNow(const char* msg) {
  rfm96w.setModeIdle();
  rfm96w.send((uint8_t*)msg, (uint8_t)strlen(msg));
  rfm96w.waitPacketSent();
  rfm96w.setModeRx();
}


static void radioSetFreq(float freq) {
  RadioCmd c{};
  c.type = CMD_SET_FREQ;
  c.freq_mhz = freq;
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
}


static bool parseAckFreq(const char* s, uint32_t& seqOut, float& fOut) {
  // "ACKFREQ,<seq>,<mhz>"
  if (!startsWith(s, "ACKFREQ,")) return false;


  const char* p = s + 8; // after "ACKFREQ,"
  char* end1 = nullptr;
  unsigned long seq = strtoul(p, &end1, 10);
  if (!end1 || *end1 != ',') return false;


  float f = (float)strtod(end1 + 1, nullptr);
  seqOut = (uint32_t)seq;
  fOut = f;
  return true;
}


static bool parseAckPing(const char* s, uint32_t& seqOut) {
  // "ACKPING,<seq>"
  if (!startsWith(s, "ACKPING,")) return false;
  seqOut = (uint32_t)strtoul(s + 8, nullptr, 10);
  return true;
}


static bool parseAckPower(const char* s, uint32_t& seqOut, int& powerOut) {
  // "ACKPOWER,<seq>,<power>"
  if (!startsWith(s, "ACKPOWER,")) return false;
  const char* p = s + 9;
  char* end1 = nullptr;
  unsigned long seq = strtoul(p, &end1, 10);
  if (!end1 || *end1 != ',') return false;
  int power = (int)strtol(end1 + 1, nullptr, 10);
  seqOut = (uint32_t)seq;
  powerOut = power;
  return true;
}


static bool parseAckReboot(const char* s, uint32_t& seqOut) {
  // "ACKREBOOT,<seq>"
  if (!startsWith(s, "ACKREBOOT,")) return false;
  seqOut = (uint32_t)strtoul(s + 10, nullptr, 10);
  return true;
}


// Parse telemetry header: "TLM,<type>,<bay_seq>,..."
static bool parseTlmHeader(const char* s, uint8_t& typeOut, uint32_t& baySeqOut, const char*& payloadOut) {
  if (!startsWith(s, "Callsign: KR4IJA | TLM,")) return false;


  const char* p = s + 23; // after "TLM,"
  char* end = nullptr;


  unsigned long t = strtoul(p, &end, 10);
  if (!end || *end != ',') return false;


  unsigned long bseq = strtoul(end + 1, &end, 10);
  if (!end || *end != ',') return false;


  typeOut = (uint8_t)t;
  baySeqOut = (uint32_t)bseq;
  payloadOut = end + 1; // rest of string after the comma
  return true;
}


static void sendAckTlm(uint8_t type, uint32_t baySeq) {
  char ack[64];
  // "ACKTLM,<type>,<bay_seq>,<gs_seq>"
  snprintf(ack, sizeof(ack), "ACKTLM,%u,%lu,%lu",
           (unsigned)type,
           (unsigned long)baySeq,
           (unsigned long)gs_seq++);
  radioSendAsciiNow(ack);
}


// CLI commands from USB serial
static void enqueueRadioCommand(String line) {
  line.trim();
  if (line.length() == 0) return;


  if (line.startsWith("freq ")) {
    float f = line.substring(5).toFloat();
    if (!validFreq433(f)) {
      Serial.println("Invalid freq. Try 420.6-438.0.");
      return;
    }
    if (freqTxn.state != TXN_IDLE && freqTxn.state != TXN_DONE && freqTxn.state != TXN_FAIL) {
      Serial.println("Freq change already in progress.");
      return;
    }


    freqTxn = FreqTxn{};
    freqTxn.seq = g_seq++;
    freqTxn.f_old = currentFreqMHz;
    freqTxn.f_new = f;
    freqTxn.state = TXN_WAIT_ACKFREQ;
    freqTxn.t_deadline_ms = millis() + freqTxn.ack_timeout_ms;


    char msg[64];
    snprintf(msg, sizeof(msg), "FREQ,%lu,%.3f", (unsigned long)freqTxn.seq, freqTxn.f_new);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s on %.3f MHz\n", msg, freqTxn.f_old);
    return;
  }


  if (line == "ping") {
    last_ping_seq = g_seq++;
    char msg[32];
    snprintf(msg, sizeof(msg), "PING,%lu", (unsigned long)last_ping_seq);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s\n", msg);
    return;
  }


  if (line == "reboot") {
    last_reboot_seq = g_seq++;
    char msg[32];
    snprintf(msg, sizeof(msg), "REBOOT,%lu", (unsigned long)last_reboot_seq);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s\n", msg);
    return;
  }


  if (line.startsWith("power ")) {
    int power = line.substring(6).toInt();
    last_power_seq = g_seq++;
    char msg[32];
    snprintf(msg, sizeof(msg), "POWER,%lu,%d", (unsigned long)last_power_seq, power);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s\n", msg);
    return;
  }


  // raw send
  RadioCmd c{};
  c.type = CMD_SEND_COMM;
  c.len = (uint8_t)min((int)line.length(), (int)sizeof(c.data));
  memcpy(c.data, line.c_str(), c.len);
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
  Serial.println("Queued: raw send");
}


// ---------- RadioTask: RX + immediate ACK + SD logging + TX queue ----------
void RadioTask(void *pv) {
  Serial.printf("[RadioTask] core=%d\n", xPortGetCoreID());


  for (;;) {
    // RX
    if (rfm96w.available()) {
      RadioRx pkt{};
      pkt.len = RH_RF95_MAX_MESSAGE_LEN;


      if (rfm96w.recv(pkt.data, &pkt.len)) {
        SensorData* rxData = (SensorData*)pkt.data;
        int8_t rssi = rfm96w.lastRssi(); // Get the RSSI
       
        // Validate packet size for binary telemetry packets
        size_t expectedSize = sizeof(SensorData);
       
        // Check if this is a binary telemetry packet (fixed size) or ASCII command (variable size)
        // if (pkt.len == expectedSize) {
          // Binary SensorData telemetry packet
          Serial.printf("RSSI=%d\n", rssi);
         
          const char* typeStr = "?";
          if (pkt.len == 85 && rxData->type == 0) {
              typeStr = "IMU";
          } else if (pkt.len == 33 && rxData->type == 1) {
              typeStr = "ALT";
          } else if (pkt.len == 46 && rxData->type == 2) {
              typeStr = "GPS";
          } else if (pkt.len == 49 && rxData->type == 3) {
              typeStr = "KAL";
          } else {
              typeStr = "UNKNOWN"; // Always handle the default case!
          }
                  
          pkt.data[pkt.len] = 0;


          // Log telemetry data to SD card


        // Telemetry parsing + logging + immediate ACK
        // IMPORTANT: pkt.data contains BINARY data (SensorData struct), NOT a C string!
        // Do NOT memcpy into a string buffer or use strlen()


        uint8_t type;
        uint32_t baySeq;
        const char* payload;


        // if (parseTlmHeader((const char*)pkt.data, type, baySeq, payload)) {
        //   // ACK ASAP (don’t wait on SD writes)
        //   // sendAckTlm(type, baySeq);


        //   // Log payload (not the TLM header)
        //   // You can choose whether to store full s or just payload.
        //   // Storing just payload keeps CSV clean.
        //   switch (type) {
        //     case 0:
        //       Serial.printf("[TLM IMU] bay_seq=%lu gs_seq=%lu Data: %s\n", (unsigned long)baySeq, (unsigned long)(gs_seq - 1), s);
        //       sd.appendBytes(IMU_FILEPATH, (const uint8_t*)payload, (uint32_t)strlen(payload));
        //       sd.appendFile(IMU_FILEPATH, "\n");
        //       break;


        //     case 1:
        //       Serial.printf("[TLM ALT] bay_seq=%lu gs_seq=%lu Data: %s\n", (unsigned long)baySeq, (unsigned long)(gs_seq - 1), s);
        //       sd.appendBytes(ALTIMETER_FILEPATH, (const uint8_t*)payload, (uint32_t)strlen(payload));
        //       sd.appendFile(ALTIMETER_FILEPATH, "\n");
        //       break;


        //     case 2:
        //       Serial.printf("[TLM GPS] bay_seq=%lu gs_seq=%lu Data: %s\n", (unsigned long)baySeq, (unsigned long)(gs_seq - 1), s);
        //       sd.appendBytes(GPS_FILEPATH, (const uint8_t*)payload, (uint32_t)strlen(payload));
        //       sd.appendFile(GPS_FILEPATH, "\n");
        //       break;


        //     case 3:
        //       Serial.printf("[TLM KAL] bay_seq=%lu gs_seq=%lu Data: %s\n", (unsigned long)baySeq, (unsigned long)(gs_seq - 1), s);
        //       sd.appendBytes(KALMAN_FILEPATH, (const uint8_t*)payload, (uint32_t)strlen(payload));
        //       sd.appendFile(KALMAN_FILEPATH, "\n");
        //       break;


        //     default:
        //       Serial.printf("[TLM ?] type=%u bay_seq=%lu\n", (unsigned)type, (unsigned long)baySeq);
        //       sd.appendBytes(TEST_FILEPATH, pkt.data, pkt.len);
        //       sd.appendFile(TEST_FILEPATH, "\n");
        //       break;
        //   }
        // } else {
        //   // Not telemetry; log unknown or leave it to AppTask
        //   // (Optional) Uncomment if you want to see non-TLM packets:
        //   Serial.printf("[RadioTask] RX: %s\n", s);
        // }


        char csvLine[256]; // Buffer to format the binary data back into a readable CSV string


    switch (rxData->type) {
        case 0: // IMU
            // Extract binary data and format to CSV string
            snprintf(csvLine, sizeof(csvLine), "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                     rxData->timestamp,
                     rxData->data.imu.accel[0],
                     rxData->data.imu.accel[1],
                     rxData->data.imu.accel[2],
                     rxData->data.imu.linear[0],
                     rxData->data.imu.linear[1],
                     rxData->data.imu.linear[2],
                     rxData->data.imu.gravity[0],
                     rxData->data.imu.gravity[1],
                     rxData->data.imu.gravity[2],
                     rxData->data.imu.quat[0],
                     rxData->data.imu.quat[1],
                     rxData->data.imu.quat[2],
                     rxData->data.imu.quat[3],
                     rxData->data.imu.gyro[0],
                     rxData->data.imu.gyro[1],
                     rxData->data.imu.gyro[2]);
                     
            Serial.printf("[GS RX IMU] bay_seq=%lu gs_seq=%lu ts=%s accel[%.3f,%.3f,%.3f] linear[%.3f,%.3f,%.3f] gravity[%.3f,%.3f,%.3f] quat[%.3f,%.3f,%.3f,%.3f] gyro[%.3f,%.3f,%.3f]\n",
                          (unsigned long)rxData->bay_seq, (unsigned long)gs_seq, rxData->timestamp,
                          rxData->data.imu.accel[0], rxData->data.imu.accel[1], rxData->data.imu.accel[2],
                          rxData->data.imu.linear[0], rxData->data.imu.linear[1], rxData->data.imu.linear[2],
                          rxData->data.imu.gravity[0], rxData->data.imu.gravity[1], rxData->data.imu.gravity[2],
                          rxData->data.imu.quat[0], rxData->data.imu.quat[1], rxData->data.imu.quat[2], rxData->data.imu.quat[3],
                          rxData->data.imu.gyro[0], rxData->data.imu.gyro[1], rxData->data.imu.gyro[2]);
            sd.appendBytes(IMU_FILEPATH, (const uint8_t*)csvLine, strlen(csvLine));
            break;
        case 1: // Altimeter
            snprintf(csvLine, sizeof(csvLine), "%s,%.6f,%.6f,%.6f\n",
                     rxData->timestamp,
                     rxData->data.alt.altitude,
                     rxData->data.alt.temp,
                     rxData->data.alt.pressure);
            Serial.printf("[GS RX ALT] bay_seq=%lu gs_seq=%lu ts=%s altitude=%.6f temp=%.6f pressure=%.6f\n",
                          (unsigned long)rxData->bay_seq, (unsigned long)gs_seq, rxData->timestamp, rxData->data.alt.altitude, rxData->data.alt.temp, rxData->data.alt.pressure);
            sd.appendBytes(ALTIMETER_FILEPATH, (const uint8_t*)csvLine, strlen(csvLine));
            break;
        case 2: // GPS
            snprintf(csvLine, sizeof(csvLine), "%s,%s\n",
                     rxData->timestamp,
                     rxData->data.gps.nmea);
            Serial.printf("[GS RX GPS] bay_seq=%lu gs_seq=%lu ts=%s nmea=%s\n",
                          (unsigned long)rxData->bay_seq, (unsigned long)gs_seq, rxData->timestamp, rxData->data.gps.nmea);
            sd.appendBytes(GPS_FILEPATH, (const uint8_t*)csvLine, strlen(csvLine));
            break;
        case 3: // KALMAN
            snprintf(csvLine, sizeof(csvLine), "%s,%d,%.6f,%.6f,%.6f\n",
                     rxData->timestamp,
                     rxData->data.kalman.kalman_state,
                     rxData->data.kalman.kalman_altitude,
                     rxData->data.kalman.kalman_velocity,
                     rxData->data.kalman.kalman_acceleration);


            Serial.printf("[GS RX KAL] bay_seq=%lu gs_seq=%lu ts=%s state=%d altitude=%.6f velocity=%.6f acceleration=%.6f\n",
                          (unsigned long)rxData->bay_seq, (unsigned long)gs_seq, rxData->timestamp,
                          rxData->data.kalman.kalman_state,
                          rxData->data.kalman.kalman_altitude,
                          rxData->data.kalman.kalman_velocity,
                          rxData->data.kalman.kalman_acceleration);


            sd.appendBytes(KALMAN_FILEPATH, (const uint8_t*)csvLine, strlen(csvLine));
            break;
        default:
            Serial.printf("[GS RX] Unknown type=%u bay_seq=%lu gs_seq=%lu\n", (unsigned)rxData->type, (unsigned long)rxData->bay_seq, (unsigned long)gs_seq);
            break;
      }
      gs_seq++;
      rfm96w.setModeRx();
        if (typeStr == "UNKNOWN") {
          // ASCII command packet (variable length) - push to AppTask for command parsing
          pkt.data[pkt.len] = 0;
          (void)xQueueSend(rxQueue, &pkt, 0);
          rfm96w.setModeRx();
        }
      }
    }


    // TX queue
    RadioCmd cmd;
    while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) {
      switch (cmd.type) {
        case CMD_SET_FREQ:
          rfm96w.setModeIdle();
          if (rfm96w.setFrequency(cmd.freq_mhz)) {
            currentFreqMHz = cmd.freq_mhz;
          }
          rfm96w.setModeRx();
          break;


        case CMD_SEND_COMM:
          rfm96w.setModeIdle();
          rfm96w.send(cmd.data, cmd.len);
          rfm96w.waitPacketSent();
          rfm96w.setModeRx();
          break;
      }
    }


    vTaskDelay(pdMS_TO_TICKS(2));
  }
}


// ---------- AppTask: FREQ handshake state machine + CLI ----------
void AppTask(void *pv) {
  Serial.printf("[AppTask] core=%d\n", xPortGetCoreID());
  Serial.setTimeout(50);


  for (;;) {
    // Consume rxQueue to drive the FREQ state machine
    RadioRx rx;
    while (xQueueReceive(rxQueue, &rx, 0) == pdTRUE) {
      const char* s = (const char*)rx.data;


      if (freqTxn.state == TXN_WAIT_ACKFREQ) {
        uint32_t seq; float f;
        if (parseAckFreq(s, seq, f) && seq == freqTxn.seq) {
          Serial.printf("[GS] Got ACKFREQ seq=%lu f=%.3f (still on old freq)\n",
                        (unsigned long)seq, f);
          freqTxn.state = TXN_SWITCH_TO_NEW;
        }
      } else if (freqTxn.state == TXN_WAIT_ACKPING) {
        uint32_t seq;
        if (parseAckPing(s, seq) && seq == freqTxn.seq) {
          Serial.printf("[GS] Got ACKPING seq=%lu on new freq %.3f MHz ✅\n",
                        (unsigned long)seq, freqTxn.f_new);
          freqTxn.state = TXN_DONE;
        }
      } else {
        // Check for standalone responses
        uint32_t seq;
        int power;
        if (parseAckPing(s, seq)) {
          Serial.printf("[GS] Got ACKPING seq=%lu 🏓\n", (unsigned long)seq);
        } else if (parseAckPower(s, seq, power)) {
          Serial.printf("[GS] Got ACKPOWER seq=%lu power=%d dBm ⚡\n", (unsigned long)seq, power);
        } else if (parseAckReboot(s, seq)) {
          Serial.printf("[GS] Got ACKREBOOT seq=%lu 🔄\n", (unsigned long)seq);
        } else {
          Serial.printf("[GS RX ASCII] %s\n", s);
        }
      }
    }


    // Drive txn
    switch (freqTxn.state) {
      case TXN_IDLE:
      case TXN_DONE:
      case TXN_FAIL:
        break;


      case TXN_WAIT_ACKFREQ: {
        if ((int32_t)(millis() - freqTxn.t_deadline_ms) > 0) {
          if (freqTxn.retries++ < freqTxn.max_retries) {
            char msg[64];
            snprintf(msg, sizeof(msg), "FREQ,%lu,%.3f", (unsigned long)freqTxn.seq, freqTxn.f_new);
            radioSendAscii(msg);
            freqTxn.t_deadline_ms = millis() + freqTxn.ack_timeout_ms;
            Serial.printf("[GS] Retry %u: %s\n", freqTxn.retries, msg);
          } else {
            Serial.println("[GS] FAIL: no ACKFREQ");
            freqTxn.state = TXN_FAIL;
          }
        }
        break;
      }


      case TXN_SWITCH_TO_NEW: {
        radioSetFreq(freqTxn.f_new);
        freqTxn.state = TXN_SWITCH_DELAY;
        freqTxn.t_deadline_ms = millis() + 50;
        Serial.printf("[GS] Switching -> %.3f MHz...\n", freqTxn.f_new);
        break;
      }


      case TXN_SWITCH_DELAY: {
        if ((int32_t)(millis() - freqTxn.t_deadline_ms) > 0) {
          freqTxn.state = TXN_WAIT_ACKPING;
          freqTxn.retries = 0;
          freqTxn.t_deadline_ms = millis() + freqTxn.ping_timeout_ms;


          char msg[32];
          snprintf(msg, sizeof(msg), "PING,%lu", (unsigned long)freqTxn.seq);
          radioSendAscii(msg);
          Serial.printf("[GS] Now on %.3f MHz, sent %s\n", freqTxn.f_new, msg);
        }
        break;
      }


      case TXN_WAIT_ACKPING: {
        if ((int32_t)(millis() - freqTxn.t_deadline_ms) > 0) {
          if (freqTxn.retries++ < freqTxn.max_retries) {
            char msg[32];
            snprintf(msg, sizeof(msg), "PING,%lu", (unsigned long)freqTxn.seq);
            radioSendAscii(msg);
            freqTxn.t_deadline_ms = millis() + freqTxn.ping_timeout_ms;
            Serial.printf("[GS] Retry %u: %s\n", freqTxn.retries, msg);
          } else {
            Serial.println("[GS] FAIL: no ACKPING on new freq");
            radioSetFreq(freqTxn.f_old);
            Serial.printf("[GS] Fallback -> %.3f MHz\n", freqTxn.f_old);
            freqTxn.state = TXN_FAIL;
          }
        }
        break;
      }
    }


    // CLI
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      enqueueRadioCommand(line);
    }


    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


// ---------- setup ----------
void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);


  Serial.begin(921600);


  Serial.printf("[GS SETUP] sizeof(SensorData) = %u bytes\n", (unsigned)sizeof(SensorData));


  if (!sd.begin()) {
    Serial.println("SD init failed; continuing without logging.");
  }


  // Prepare files
  sd.appendFile(IMU_FILEPATH,       "\n\nInitializing\n\ntime_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.appendFile(ALTIMETER_FILEPATH, "\n\nInitializing\n\ntime_stamp,altitude,temperature,pressure\n");
  sd.appendFile(GPS_FILEPATH,       "\n\nInitializing\n\ntime_stamp,gps_data\n");
  sd.appendFile(KALMAN_FILEPATH,    "\n\nInitializing\n\ntime_stamp, state, altitude,velocity,acceleration\n");

  while (!Serial && millis() < 1000) {}


  SPI.begin(RFM96W_SCK, RFM96W_MISO, RFM96W_MOSI, RFM96W_CS);


  // Reset radio
  digitalWrite(RFM96W_RST, LOW);
  delay(10);
  digitalWrite(RFM96W_RST, HIGH);
  delay(10);


  if (!rfm96w.init()) {
    Serial.println("RFM96W initialization failed");
    while (1) delay(100);
  }


  if (!rfm96w.setFrequency(currentFreqMHz)) {
    Serial.println("setFrequency failed");
    while (1) delay(100);
  }


  rfm96w.setSignalBandwidth(RFM_BW_HZ);
  rfm96w.setSpreadingFactor(RFM_SF);
  rfm96w.setTxPower(20, false);
  rfm96w.setCodingRate4(8);


  Serial.printf("Radio OK. Freq=%.3f BW=%lu SF=%u\n",
                currentFreqMHz, (unsigned long)RFM_BW_HZ, (unsigned)RFM_SF);


  cmdQueue = xQueueCreate(20, sizeof(RadioCmd));
  rxQueue  = xQueueCreate(20, sizeof(RadioRx));
  if (!cmdQueue || !rxQueue) {
    Serial.println("Queue create failed");
    while (1) delay(100);
  }


  xTaskCreatePinnedToCore(RadioTask, "RadioTask", 8192, nullptr, 3, &radioTaskHandle, 1);
  xTaskCreatePinnedToCore(AppTask,   "AppTask",   4096, nullptr, 1, nullptr,          0);


  rfm96w.setModeRx();
}


// ---------- loop ----------
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

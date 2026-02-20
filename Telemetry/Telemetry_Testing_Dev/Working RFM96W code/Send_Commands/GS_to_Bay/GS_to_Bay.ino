  // GS sending to Bay

#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <string.h>
#include "V1_SOAR_RTOS_SD_CARD.h"
#include "_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define RFM96W_SCK   D8
#define RFM96W_MISO  D9
#define RFM96W_MOSI  D10

#define RFM96W_CS   D0
#define RFM96W_RST  D2
#define RFM96W_INT  D3

float currentFreqMHz = 430.0;
static uint32_t g_seq = 1;

RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);
SOAR_SD_CARD sd(D1, false);

enum CmdType : uint8_t {CMD_SET_FREQ, CMD_SEND_COMM};

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

  uint32_t t_deadline_ms = 0;   // timeout deadline
  uint8_t retries = 0;

  // knobs
  uint32_t ack_timeout_ms = 800;
  uint32_t ping_timeout_ms = 800;
  uint8_t max_retries = 3;
};

struct RadioCmd {
  CmdType type;
  float freq_mhz;
  uint8_t data[64];
  uint8_t len;
};

struct RadioRx {
  uint8_t data[RH_RF95_MAX_MESSAGE_LEN + 1];
  uint8_t len;
};

QueueHandle_t cmdQueue;
QueueHandle_t rxQueue;
TaskHandle_t radioTaskHandle = nullptr;
static FreqTxn freqTxn;

static bool validFreq433(float f) {
  return (f >= 428.5 && f <= 431.5) || (f >= 445.5 && f <= 449.0);
}

static bool startsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void radioSendAscii(const char* msg) {
  RadioCmd c{};
  c.type = CMD_SEND_COMM;
  c.len = (uint8_t)min((int)strlen(msg), (int)sizeof(c.data));
  memcpy(c.data, msg, c.len);
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
}

static void radioSetFreq(float freq) {
  RadioCmd c{};
  c.type = CMD_SET_FREQ;
  c.freq_mhz = freq;
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
}

static bool parseAckFreq(const char* s, uint32_t& seqOut, float& fOut) {
  if (!startsWith(s, "ACKFREQ,")) return false;

  const char* p = s + 7;   // points at start of seq (correct)
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

static void enqueueRadioCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  RadioCmd c{};
  if (line.startsWith("freq ")) {
    float f = line.substring(5).toFloat();
    if (!validFreq433(f)) {
      Serial.println("Invalid freq. For 433 band try 428.5 to 431.5 or 445.5 to 449.0.");
      return;
    }
    if (freqTxn.state != TXN_IDLE && freqTxn.state != TXN_DONE && freqTxn.state != TXN_FAIL) {
      Serial.println("Freq change already in progress.");
      return;
    }
    freqTxn = FreqTxn{}; // reset
    freqTxn.seq = g_seq++;
    freqTxn.f_old = currentFreqMHz;
    freqTxn.f_new = f;
    freqTxn.retries = 0;
    freqTxn.state = TXN_WAIT_ACKFREQ;
    freqTxn.t_deadline_ms = millis() + freqTxn.ack_timeout_ms;
    char msg[64];
    snprintf(msg, sizeof(msg), "FREQ,%lu,%.3f", (unsigned long)freqTxn.seq, freqTxn.f_new);
    radioSendAscii(msg);

    Serial.printf("[GS] Sent %s on %.3f MHz\n", msg, freqTxn.f_old);
    return;
  }

  if (line == "ping") {
    uint32_t seq = g_seq++;
    char msg[32];
    snprintf(msg, sizeof(msg), "PING,%lu", (unsigned long)seq);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s\n", msg);
    return;
  }

  if (line == "reboot") {
    uint32_t seq = g_seq++;
    char msg[32];
    snprintf(msg, sizeof(msg), "REBOOT,%lu", (unsigned long)seq);
    radioSendAscii(msg);
    Serial.printf("[GS] Sent %s\n", msg);
    return;
  }

  // default: raw send
  c.type = CMD_SEND_COMM;
  c.len = (uint8_t)min((int)line.length(), (int)sizeof(c.data));
  memcpy(c.data, line.c_str(), c.len);
  xQueueSend(cmdQueue, &c, portMAX_DELAY);
  Serial.println("Queued: raw send");
}

void RadioTask(void *pv) {
  Serial.printf("[RadioTask] core=%d\n", xPortGetCoreID());

  for (;;) {
    if (rfm96w.available()) {
      RadioRx pkt{};
      pkt.len = RH_RF95_MAX_MESSAGE_LEN;

      if (rfm96w.recv(pkt.data, &pkt.len)) {
        pkt.data[pkt.len] = 0;
        if (xQueueSend(rxQueue, &pkt, 0) != pdTRUE) {}
        if (pkt.len > 0) {
          switch (pkt.data[0]) {
            case '0':
              Serial.printf("%s\n", pkt.data);
              sd.appendBytes(IMU_FILEPATH, pkt.data, pkt.len);
              sd.appendFile(IMU_FILEPATH, "\n");
              break;
            case '1':
              Serial.printf("%s\n", pkt.data);
              sd.appendBytes(ALTIMETER_FILEPATH, pkt.data, pkt.len);
              sd.appendFile(ALTIMETER_FILEPATH, "\n");
              break;
            case '2':
              Serial.printf("%s\n", pkt.data);
              sd.appendBytes(GPS_FILEPATH, pkt.data, pkt.len);
              sd.appendFile(GPS_FILEPATH, "\n");
              break;
            case '3':
              Serial.printf("%s\n", pkt.data);
              sd.appendBytes(KALMAN_FILEPATH, pkt.data, pkt.len);
              sd.appendFile(KALMAN_FILEPATH, "\n");
              break;
            default:
              Serial.printf("[RadioTask] Received len=%u: %s\n", pkt.len, pkt.data);
              sd.appendBytes(TEST_FILEPATH, pkt.data, pkt.len);
              sd.appendFile(TEST_FILEPATH, "\n");
              break;
          }
      }
      rfm96w.setModeRx();
    }
  }

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

void AppTask(void *pv) {
  Serial.printf("[AppTask] core=%d\n", xPortGetCoreID());
  Serial.setTimeout(50);

  for (;;) {
    RadioRx rx;
    while (xQueueReceive(rxQueue, &rx, 0) == pdTRUE) {
      const char* s = (const char*)rx.data;

      if (freqTxn.state == TXN_WAIT_ACKFREQ) {
        uint32_t seq; float f;
        if (parseAckFreq(s, seq, f) && seq == freqTxn.seq) {
          Serial.printf("[GS] Got ACKFREQ seq=%lu f=%.3f (still on old freq)\n",
                        (unsigned long)seq, f);
          // Move to switch state
          freqTxn.state = TXN_SWITCH_TO_NEW;
        }
      } else if (freqTxn.state == TXN_WAIT_ACKPING) {
        uint32_t seq;
        if (parseAckPing(s, seq) && seq == freqTxn.seq) {
          Serial.printf("[GS] Got ACKPING seq=%lu on new freq %.3f MHz ✅\n",
                        (unsigned long)seq, freqTxn.f_new);
          freqTxn.state = TXN_DONE;
        }
      }
    }

    switch (freqTxn.state) {
      case TXN_IDLE:
      case TXN_DONE:
      case TXN_FAIL:
        break;

      case TXN_WAIT_ACKFREQ: {
        if ((int32_t)(millis() - freqTxn.t_deadline_ms) > 0) {
          if (freqTxn.retries++ < freqTxn.max_retries) {
            char msg[64];
            snprintf(msg, sizeof(msg), "FREQ,%lu,%.3f",
                     (unsigned long)freqTxn.seq, freqTxn.f_new);
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
        // Switch local radio to new frequency
        radioSetFreq(freqTxn.f_new);
        freqTxn.state = TXN_SWITCH_DELAY;
        freqTxn.t_deadline_ms = millis() + 50; // 50ms settle
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
            // Optional fallback: switch back to old
            radioSetFreq(freqTxn.f_old);
            Serial.printf("[GS] Fallback -> %.3f MHz\n", freqTxn.f_old);
            freqTxn.state = TXN_FAIL;
          }
        }
        break;
      }
    }

    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      enqueueRadioCommand(line);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  pinMode(RFM96W_RST, OUTPUT);
  digitalWrite(RFM96W_RST, HIGH);
  
  Serial.begin(115200);
  if (!sd.begin()) {
    Serial.println("SD init failed; continuing without logging.");
  }
  sd.deleteFile(TEST_FILEPATH);
  sd.deleteFile(IMU_FILEPATH);
  sd.deleteFile(GPS_FILEPATH);
  sd.deleteFile(ALTIMETER_FILEPATH);
  sd.deleteFile(KALMAN_FILEPATH);
  sd.writeFile(TEST_FILEPATH, "test\n");
  sd.writeFile(IMU_FILEPATH, "time_stamp,accel_x,accel_y,accel_z,linear_x,linear_y,linear_z,gravity_x,gravity_y,gravity_z,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z\n");
  sd.writeFile(ALTIMETER_FILEPATH, "time_stamp,altitude,temperature,pressure\n");
  sd.writeFile(GPS_FILEPATH, "time_stamp,gps_data\n");
  sd.writeFile(KALMAN_FILEPATH, "time_stamp,altitude,velocity,acceleration\n");
  while (!Serial && millis() < 1000) {}

  SPI.begin(RFM96W_SCK, RFM96W_MISO, RFM96W_MOSI, RFM96W_CS);

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

  rfm96w.setTxPower(20, false); // 20 dBm

  Serial.print("Frequency set to ");
  Serial.println(currentFreqMHz);

  cmdQueue = xQueueCreate(10, sizeof(RadioCmd));
  rxQueue  = xQueueCreate(10, sizeof(RadioRx));
  if (!cmdQueue || !rxQueue) {
    Serial.println("Queue create failed");
    while (1) delay(100);
  }

  xTaskCreatePinnedToCore(RadioTask, "RadioTask", 8192, nullptr, 3, &radioTaskHandle, 1);
  xTaskCreatePinnedToCore(AppTask,   "AppTask",   4096, nullptr, 1, nullptr,          0);
  rfm96w.setModeRx();
}

void loop() {
  // vTaskDelay(pdMS_TO_TICKS(1000));
}

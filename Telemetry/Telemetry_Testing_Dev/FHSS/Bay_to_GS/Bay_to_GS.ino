#include <SPI.h>
#include <RH_RF95.h>
#include <string.h>
#include <stdlib.h>   // strtoul, strtod
#include <stdint.h>

#define RFM96W_CS  36
#define RFM96W_RST 9
#define RFM96W_INT 2

uint32_t t_last;
uint32_t tx_time;

const float MIN_FREQ_1 = 428.5;
const float MAX_FREQ_1 = 431.5;
const float MIN_FREQ_2 = 445.5;
const float MAX_FREQ_2 = 449.0;

const int NUM_CHANNELS = 8;

static float currentFreqMHz = 433.0;
RH_RF95 rfm96w(RFM96W_CS, RFM96W_INT);

// ---- Telemetry pause control ----
static uint32_t quietUntilMs = 0;                  // don't send telemetry before this time
static const uint32_t QUIET_AFTER_CMD_MS = 1200;   // pause after receiving any command
static const uint32_t QUIET_AFTER_SWITCH_MS = 1200;// pause after switching freq

// ---- Duplicate protection ----
static uint32_t lastFreqSeq = 0;    // last seq for FREQ handled
static float    lastFreqNew = 0.0f; // last requested freq

// ---------------- Helpers ----------------
static bool startsWith(const char* str, const char* prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

static bool parseSeq(char* str, uint32_t& seqOut) {
    // "<seq>,..."
    char* end_ptr = nullptr;
    unsigned long seq = strtoul(str, &end, 10);
    if (!end || *end1 != ',') return false;

    seqOut = (uint32_t)seq;
    str = end_ptr;
    return true;
}

static bool parseFreqCmd(const char* str, float& fOut) {
  // "FREQ,<mhz>"
  if (!startsWith(str, "FREQ,")) return false;
  fOut = (float)strtod(str + 5, nullptr);
  return true;
}

static bool parsePingCmd(const char* str, uint32_t& seqOut) {
  // "PING"
  if (!startsWith(str, "PING,")) return false;
  seqOut = (uint32_t)strtoul(str + 5, nullptr, 10);
  return true;
}

static bool parseRebootCmd(const char* str, uint32_t& seqOut) {
  // "REBOOT"
  if (!startsWith(str, "REBOOT,")) return false;
  seqOut = (uint32_t)strtoul(str + 7, nullptr, 10);
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

  rf96w.setSignalBandwidth(100'000);
  rf96w.setSpreadingFactor(7);
  rfm96w.setTxPower(20, false);

  Serial.print("Bay freq set to ");
  Serial.println(currentFreqMHz, 3);

  rfm96w.setModeRx();
}

void loop() {
  tx_time = millis() - last_t;
  /* do stuff here maybe */
  last_t = millis();

  // --------- 1) Listen for commands ----------
  if (rfm96w.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN + 1];
    uint8_t len = RH_RF95_MAX_MESSAGE_LEN;
    uint32_t seq;

    if (rfm96w.recv(buf, &len)) {
      float fNew;
      buf[len] = 0;
      char* str1 = (char*)buf;

      // Any command triggers a quiet window to reduce airtime collisions
      pauseTelemetry(QUIET_AFTER_CMD_MS);

      /* parse seq and increment str pointer */
      parseSeq(str1, seq);
      const char* str2 = (const char*)str1;

      if (parseFreqCmd(str1, seq, fNew)) {
        Serial.print("[Bay] RX ");
        Serial.println(str1);

        // Always ACK on old freq
        char ack[64];
        snprintf(ack, sizeof(ack), "%lu,ACKFREQ,%.3f", (unsigned long)seq, fNew);
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
      else if (parsePingCmd(str1, seq)) {
        Serial.print("[Bay] RX ");
        Serial.println(str1);

        char ack[32];
        snprintf(ack, sizeof(ack), "%lu,ACKPING", (unsigned long)seq);
        Serial.print("[Bay] TX ");
        Serial.println(ack);
        sendAscii(ack);

        // Give a short quiet window after ACKPING too
        pauseTelemetry(300);
      }
      else if (parseRebootCmd(str1, seq)) {
        Serial.print("[Bay] RX ");
        Serial.println(str1);

        char ack[40];
        snprintf(ack, sizeof(ack), "%lu,ACKREBOOT", (unsigned long)seq);
        Serial.print("[Bay] TX ");
        Serial.println(ack);
        sendAscii(ack);

        delay(200);
        Serial.println("[Bay] Rebooting...");
        teensyReboot();
      }
      else {
        Serial.print("[Bay] RX unknown: ");
        Serial.println(str1);
      }
    }
  }

  // --------- 2) Telemetry sender (paused during handshake) ----------
  if ((int32_t)(millis() - quietUntilMs) >= 0) {
    const char* msg  = "0DATA";
    const char* msg2 = "1DATA";

    sendAscii(msg);
    delay(100);

    sendAscii(msg2);
    delay(100);
  } else {
    // While quiet, keep loop responsive
    delay(5);
  }
}

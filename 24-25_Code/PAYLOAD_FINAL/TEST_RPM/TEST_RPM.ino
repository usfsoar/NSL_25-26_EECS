#include <Arduino.h>
#include "SOAR_IMU.h"

// match your real‐world window:
const unsigned long RPM_LOG_INTERVAL_MS = 100;
const int RPM_WINDOW_SIZE = 1000 / RPM_LOG_INTERVAL_MS;  // 10 samples → 1s

// circular buffer and state
float rpmWindow[RPM_WINDOW_SIZE];
int rpmWindowIndex = 0;
int rpmSampleCount = 0;
float maxSustainedRPM = 0;

SOAR_IMU soar_imu;

float rpmReading[3];

void updateMaxSustainedRPM(float currentRpm) {
  rpmWindow[rpmWindowIndex] = currentRpm;
  rpmWindowIndex = (rpmWindowIndex + 1) % RPM_WINDOW_SIZE;
  if (rpmSampleCount < RPM_WINDOW_SIZE)
    rpmSampleCount++;

  if (rpmSampleCount == RPM_WINDOW_SIZE) {
    float sum = 0;
    for (int i = 0; i < RPM_WINDOW_SIZE; i++)
      sum += rpmWindow[i];
    float avg = sum / RPM_WINDOW_SIZE;
    if (avg > maxSustainedRPM)
      maxSustainedRPM = avg;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // zero‐fill buffer
  for (int i = 0; i < RPM_WINDOW_SIZE; i++)
    rpmWindow[i] = 0;

  soar_imu.BNO_SETUP();
  delay(100);

  Serial.println(F(" RPM sustained test starting:n"));
}

void loop() {
  static unsigned long lastLogTime = 0;
  unsigned long now = millis();

  if (now - lastLogTime >= RPM_LOG_INTERVAL_MS) {
    lastLogTime = now;

    soar_imu.GET_RPM(rpmReading);

    updateMaxSustainedRPM(rpmReading[0]);

    Serial.print(F("rawRPM="));
    Serial.print(rpmReading[0], 1);
    Serial.print(F("  windowFull="));
    Serial.print(rpmSampleCount == RPM_WINDOW_SIZE ? "yes" : "no ");
    Serial.print(F("  maxSustained="));
    Serial.println(maxSustainedRPM, 1);
  }
}

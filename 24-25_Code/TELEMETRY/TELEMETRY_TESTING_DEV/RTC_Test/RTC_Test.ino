#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include "V1_SOAR_RTC.h"

SOAR_RTC rtc;

void RTCTask (void *Parameters) {  
  while (1) {
    char time[40];
    rtc.getTimeString(time);
    rtc.getTimeInt(time);
    Serial.println(time);
    rtc.getDate(time);
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  rtc.adjustTime(1, 28, 2025, 15, 6, 10, 000000); // Set time (month, day, year, hour, minute, second, microsecond)

  xTaskCreatePinnedToCore(
    RTCTask,
    "RTC Task",
    4096,
    NULL,
    3,
    NULL,
    app_cpu
  );
}

void loop() {
  // Empty, tasks are handled by FreeRTOS
}

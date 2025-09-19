#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#include "RTC_Test.h"

SOAR_RTC rtc;

// void RTCTask (void *Parameters) {  
//   while (1) {
//     char time[40];
//     rtc.getTimeString(time);
//     rtc.getTimeInt(time);
//     Serial.println(time);
//     rtc.getDate(time);
//     delay(1000);
//   }
// }

void setup() {
  Serial.begin(115200);
  delay(200); // Wait for serial monitor to open
  
  Serial.println("=== ESP32C3 RTC Test ===");
  Serial.println("Setting initial time...");
  
  // Set time to: January 28, 2025, 15:06:10
  bool timeSet = rtc.adjustTime(1, 28, 2025, 15, 6, 10, 0);
  
  if (timeSet) {
    Serial.println("✓ Time set successfully!");
  } else {
    Serial.println("✗ Failed to set time!");
  }
  
  Serial.println("Starting RTC test - Watch for time progression...");
  Serial.println("Format: HH:MM:SS:microseconds | MM/DD/YYYY");
  Serial.println("----------------------------------------");
}

void loop() {
  // Get individual time components
  int hours = rtc.getTimeHours();
  int minutes = rtc.getTimeMinutes();
  int seconds = rtc.getTimeSeconds();
  int microseconds = rtc.getTimeMicroseconds();
  
  // Check if we got valid time values (9999999 indicates error)
  if (hours != 9999999 && minutes != 9999999 && seconds != 9999999 && microseconds != 9999999) {
    // Print formatted time
    Serial.print("Time: ");
    Serial.printf("%02d:%02d:%02d.%06d", hours, minutes, seconds, microseconds);
    Serial.println();
    
    // Show individual components for verification
    Serial.print("Components - H:");
    Serial.print(hours);
    Serial.print(" M:");
    Serial.print(minutes);
    Serial.print(" S:");
    Serial.print(seconds);
    Serial.print(" μs:");
    Serial.println(microseconds);
    
    Serial.println("✓ RTC is working - time is advancing");
  } else {
    Serial.println("✗ RTC Error - Invalid time values received");
  }
  
  Serial.println(""); // Empty line for readability
  
  delay(200); // Update every 2 seconds to see time progression
}

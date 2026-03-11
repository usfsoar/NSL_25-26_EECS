#include "RTC_Test.h"
#include <Arduino.h> // For micros()

// The time_t type and hour(), minute(), second() functions are part of TimeLib.h

// Global variable is not needed with TimeLib. The library handles the time tracking.
// struct timeval tv; 

SOAR_RTC::SOAR_RTC() {}

void SOAR_RTC::getTimestamp(char* out, size_t outSize, bool includeMillis) {
    if (!out || outSize == 0) return;

    uint32_t totalMillis = millis();

    // TOTAL hours since boot (T+ hours)
    uint32_t hours = totalMillis / 3600000UL;
    uint32_t minutes = (totalMillis / 60000UL) % 60UL;
    uint32_t seconds = (totalMillis / 1000UL) % 60UL;
    uint32_t ms = totalMillis % 1000UL;

    if (includeMillis) {
        snprintf(out, outSize, "%lu:%02lu:%02lu.%03lu",
                 (unsigned long)hours,
                 (unsigned long)minutes,
                 (unsigned long)seconds,
                 (unsigned long)ms);
    } else {
        snprintf(out, outSize, "%lu:%02lu:%02lu",
                 (unsigned long)hours,
                 (unsigned long)minutes,
                 (unsigned long)seconds);
    }
}
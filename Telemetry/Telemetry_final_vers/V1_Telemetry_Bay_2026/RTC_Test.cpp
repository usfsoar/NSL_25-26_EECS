#include "RTC_Test.h"
#include <Arduino.h> // For micros()

// The time_t type and hour(), minute(), second() functions are part of TimeLib.h

// Global variable is not needed with TimeLib. The library handles the time tracking.
// struct timeval tv; 

SOAR_RTC::SOAR_RTC() {}

unsigned long SOAR_RTC::getElapsedMillis() {
    return millis();
}

unsigned long SOAR_RTC::getElapsedMicros() {
    return micros();
}

unsigned long SOAR_RTC::getTimeMilliseconds() {
    return millis();
}

unsigned long SOAR_RTC::getTimeMicroseconds() {
    return micros();
}

int SOAR_RTC::getTimeHours() {
    return (millis() / 3600000) % 24;
}

int SOAR_RTC::getTimeMinutes() {
    return (millis() / 60000) % 60;
}

int SOAR_RTC::getTimeSeconds() {
    return (millis() / 1000) % 60;
}

String SOAR_RTC::getTimestamp(bool includeMillis) {
    unsigned long totalMillis = millis();
    
    int hours = (totalMillis / 3600000) % 24;
    int minutes = (totalMillis / 60000) % 60;
    int seconds = (totalMillis / 1000) % 60;
    int milliseconds = totalMillis % 1000;
    
    char buffer[16];
    if (includeMillis) {
        sprintf(buffer, "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    } else {
        sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    }
    
    return String(buffer);
}
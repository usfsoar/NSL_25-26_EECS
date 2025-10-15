#include "RTC_Test.h"
#include <Arduino.h> // For micros()

// The time_t type and hour(), minute(), second() functions are part of TimeLib.h

// Global variable is not needed with TimeLib. The library handles the time tracking.
// struct timeval tv; 

SOAR_RTC::SOAR_RTC() {

    // Start the hardware RTC (this is the key initialization step)
    Teensy3Clock.get();
    // Check if the RTC is running and set a default if not (optional)
    if (timeStatus() == timeNotSet) {
        // Serial.println("RTC time is not set. Please call adjustTime() once.");
    }
}

// NOTE: The 'while(1)' loops in the original getters are wrong for a class method.
// They would prevent the task from ever returning. They have been removed.

// --- Time Retrieval Methods ---

int SOAR_RTC::getTimeHours() {
    // TimeLib's hour() function returns the hour based on the last sync
    return hour();
}

int SOAR_RTC::getTimeMinutes() {
    return minute();
}

int SOAR_RTC::getTimeSeconds() {
    return second();
}

int SOAR_RTC::getTimeMicroseconds() {
    // Standard TimeLib/RTC only track down to the second. 
    // We use the hardware micros() function for the microsecond part, 
    // which is relative to boot, not the RTC time, but is the standard solution.
    return (int)(micros() % 1000000); 
}

// --- Time Adjustment Method (Fixes the settimeofday Error) ---

bool SOAR_RTC::adjustTime(int month, int day, int year, int hour, int minute, int second) {
    
    // 1. Fill the TimeElements structure with the target time
    TimeElements tm;
    tm.Year   = year - 1970; // TimeLib uses years since 1970
    tm.Month  = month;
    tm.Day    = day;
    tm.Hour   = hour;
    tm.Minute = minute;
    tm.Second = second;
    
    // 2. Convert TimeElements into a Unix epoch timestamp (time_t)
    time_t t = makeTime(tm);

    if (t > 0) {
        // 3. Set the hardware RTC using the epoch timestamp
        // This function replaces your settimeofday() call
        Teensy3Clock.set(t); 
        setTime(t); // Also update the software timekeeping
        return true;
    }
    
    return false; // Failed to create a valid time_t
}
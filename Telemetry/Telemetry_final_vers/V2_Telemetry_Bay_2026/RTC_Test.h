#ifndef SOAR_RTC_H
#define SOAR_RTC_H

#include <TimeLib.h> 
#include <Arduino.h>

class SOAR_RTC {
public:
    SOAR_RTC();
    
    int getTimeHours();
    int getTimeMinutes();
    int getTimeSeconds();
    unsigned long getTimeMicroseconds();
    unsigned long getTimeMilliseconds();
    
    // Get formatted timestamp string for logging
    String getTimestamp(bool includeMillis = false);
    
    unsigned long getElapsedMillis();
    unsigned long getElapsedMicros();
};

#endif // SOAR_RTC_H
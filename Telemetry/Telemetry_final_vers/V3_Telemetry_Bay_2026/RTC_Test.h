#ifndef SOAR_RTC_H
#define SOAR_RTC_H

#include <TimeLib.h> 
#include <Arduino.h>

class SOAR_RTC {
public:
    SOAR_RTC();
    
    // Get formatted timestamp string for logging
    void getTimestamp(char* out, size_t outSize, bool includeMillis);
};

#endif // SOAR_RTC_H
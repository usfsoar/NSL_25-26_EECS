#ifndef SOAR_RTC_H
#define SOAR_RTC_H


class SOAR_RTC {
public:
    SOAR_RTC();// Constructor
    // Declare methods and variables here
    char* getTimeString(char* time);
    char* getTimeInt(char* time);
    char* getDate(char* date);
    bool adjustTime(int month, int day, int year, int hour, int minute, int second, int microsecond);
};

#endif // SOAR_RTC_H
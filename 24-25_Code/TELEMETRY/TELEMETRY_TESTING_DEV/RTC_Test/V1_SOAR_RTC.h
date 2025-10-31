#ifndef SOAR_RTC_H
#define SOAR_RTC_H


class SOAR_RTC {
public:
    SOAR_RTC();// Constructor
    // Declare methods and variables here
    void getTimeString(char* time);
    int getTimeInt(char* time);
    void getDate(char* date);
    bool adjustTime(int month, int day, int year, int hour, int minute, int second, int microsecond);
};
    
#endif // SOAR_RTC_H
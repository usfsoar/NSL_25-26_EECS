#ifndef SOAR_RTC_H
#define SOAR_RTC_H
// #include <vector>
// using namespace std;
#include <TimeLib.h> 

class SOAR_RTC {
public:
    SOAR_RTC();// Constructor
    // Declare methods and variables here
    // char* getTimeString(char* time);
    // void TimeInitialize();
    int getTimeHours();
    int getTimeMinutes();
    int getTimeSeconds();
    int getTimeMicroseconds();
    // char* getDate(char* date);
    bool adjustTime(int month, int day, int year, int hour, int minute, int second);

    // std::vector<int> time;
};

#endif // SOAR_RTC_H
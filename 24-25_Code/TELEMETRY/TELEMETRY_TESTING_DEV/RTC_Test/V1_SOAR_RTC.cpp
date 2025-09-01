#include "V1_SOAR_RTC.h"
#include <sys/time.h>
#include <arduino.h>

struct timeval tv;

SOAR_RTC::SOAR_RTC() {
 // Constructor
}

void SOAR_RTC::getTimeString(char* time) {
  int result = gettimeofday(&tv, NULL);
  if (result == 0) {
    struct tm* timeinfo = localtime(&tv.tv_sec);
    char timeWithoutMicroseconds[30];
    strftime(timeWithoutMicroseconds, sizeof(timeWithoutMicroseconds), "%H:%M:%S", timeinfo);
    int microseconds = tv.tv_usec;
    snprintf(time, 40, "%s:%06d", timeWithoutMicroseconds, microseconds);
    Serial.println(time);
  } else {
    Serial.println("Failure to Retrieve time - String");
  }
}


int SOAR_RTC::getTimeInt(char* time) {
  int result = gettimeofday(&tv, NULL);
  if (result == 0) {
    struct tm* timeinfo = localtime(&tv.tv_sec);
    int hours = timeinfo->tm_hour; // Int Version
    int minutes = timeinfo->tm_min;
    int seconds = timeinfo->tm_sec;
    int microseconds = tv.tv_usec;
    snprintf(time, 40, "%02d:%02d:%02d:%06d", hours, minutes, seconds, microseconds);
    // Serial.println(time);
    return int(time);
  }
  else {
    Serial.println("Failure to Retrieve time - Int");
  }
}
void SOAR_RTC::getDate(char* date){
  int result = gettimeofday(&tv, NULL);
  if (result == 0) {
   struct tm* timeinfo = localtime(&tv.tv_sec);
   // Set the date to a known value
   int year = timeinfo->tm_year + 1900; // Year since 1900
   int month = timeinfo->tm_mon + 1; // January (0-based)
   int day = timeinfo->tm_mday; // 1st day of the month
   snprintf(date, 40, "%02d/%02d/%04d", month, day, year);
   Serial.println(date);
  } 
  else {
    Serial.println("Failure to Retrieve date");
  }
}

 bool SOAR_RTC::adjustTime(int month, int day, int year, int hour, int minute, int second, int microsecond) {
   struct timeval tv;
   struct tm timeinfo = {0};
   // Set the date to a known value
   timeinfo.tm_year = year - 1900; // Year set year
   timeinfo.tm_mon = month - 1; // set month
   timeinfo.tm_mday = day; // set day of the month

   timeinfo.tm_hour = hour;
   timeinfo.tm_min = minute;
   timeinfo.tm_sec = second;
   tv.tv_sec = mktime(&timeinfo);
   tv.tv_usec = microsecond;
   return settimeofday(&tv, NULL) == 0;
 }
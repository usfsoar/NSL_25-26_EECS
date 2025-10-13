#include "RTC_Test.h"
#include <sys/time.h>
#include "esp_timer.h"
#include <vector>
using namespace std;
// #include <stdio.h>
struct timeval tv;

SOAR_RTC::SOAR_RTC() {
 // Constructor
}

// void SOAR_RTC::TimeInitialize() {
//   esp_timer_create_args_t timer_args = {
//         .callback = [](void*) {
//             struct timeval now;
//             gettimeofday(&now, NULL);
//             // Update system time here if needed
            
//         },
//         .name = "system_time_timer"
//    };

//    esp_timer_handle_t timer;
//     esp_timer_create(&timer_args, &timer);
//     esp_timer_start_periodic(timer, 1000000); // 1-second interval
// }
// char* SOAR_RTC::getTimeString(char* time) {
//   int result = gettimeofday(&tv, NULL);
//   if (result == 0) {
//     struct tm* timeinfo = localtime(&tv.tv_sec);
//     char timeWithoutMicroseconds[30];
//     strftime(timeWithoutMicroseconds, sizeof(timeWithoutMicroseconds), "%H:%M:%S", timeinfo);
//     int microseconds = tv.tv_usec;
//     snprintf(time, 20, "%s:%06d", timeWithoutMicroseconds, microseconds);
//     // Serial.println(time);
//   } else {
//     // Serial.println("Failure to Retrieve time - String");
//   }
// }


int SOAR_RTC::getTimeHours() {
  while(1) {
  int result = gettimeofday(&tv, NULL);
    if (result == 0) {
      struct tm* timeinfo = localtime(&tv.tv_sec);
      int hours = timeinfo->tm_hour; // Int Version
      // int minutes = timeinfo->tm_min;
      // int seconds = timeinfo->tm_sec;
      // int microseconds = tv.tv_usec;
      // int time = {hours, minutes, seconds, microseconds};
      // snprintf(time, 20, "%02d:%02d:%02d:%06d", hours, minutes, seconds, microseconds);
      // Serial.println(time);
      // true_time = '\0';
      return hours;
    }
    else {
    // Serial.println("Failure to Retrieve time - Hours");
    return 9999999;
  }
  }
}

int SOAR_RTC::getTimeMinutes() {
   while(1) {
  int result = gettimeofday(&tv, NULL);
    if (result == 0) {
      struct tm* timeinfo = localtime(&tv.tv_sec);
      int minutes = timeinfo->tm_min; // Int Version
      return minutes;
    } else {
      return 9999999;
    }
}
}

int SOAR_RTC::getTimeSeconds() {
   while(1) {
  int result = gettimeofday(&tv, NULL);
    if (result == 0) {
      struct tm* timeinfo = localtime(&tv.tv_sec);
      int seconds = timeinfo->tm_sec; // Int Version
      return seconds;
    } else {
      return 9999999;
    }
}
}

int SOAR_RTC::getTimeMicroseconds() {
   while(1) {
  int result = gettimeofday(&tv, NULL);
    if (result == 0) {
      struct tm* timeinfo = localtime(&tv.tv_sec);
      int microseconds = tv.tv_usec; // Int Version
      return microseconds;
    } else {
      return 9999999;
    }
}
}

// char* SOAR_RTC::getDate(char* date){
//   while(1) {
//   int result = gettimeofday(&tv, NULL);
//   char* true_date = date;
//   if (result == 0) {
//    struct tm* timeinfo = localtime(&tv.tv_sec);
//    // Set the date to a known value
//    int year = timeinfo->tm_year + 1900; // Year since 1900
//    int month = timeinfo->tm_mon + 1; // January (0-based)
//    int day = timeinfo->tm_mday; // 1st day of the month
//    snprintf(date, 20, "%02d/%02d/%04d", month, day, year);
//   //  *true_date = '\0';
//    return true_date;
//   } 
//   else {
//     // Serial.println("Failure to Retrieve date");
//   }
//   }

// }

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
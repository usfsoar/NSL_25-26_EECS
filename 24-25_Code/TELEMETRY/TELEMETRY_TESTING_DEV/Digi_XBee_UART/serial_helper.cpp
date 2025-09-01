// serial_helper.cpp
#include <Arduino.h>

extern "C" void serial_print(const char* msg) {
    Serial.println(msg);
}

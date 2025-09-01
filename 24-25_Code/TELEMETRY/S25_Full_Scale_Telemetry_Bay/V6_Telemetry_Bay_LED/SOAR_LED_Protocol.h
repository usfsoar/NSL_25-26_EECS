#ifndef LED_PROTOCOL_H
#define LED_PROTOCOL_H
#include <Arduino.h>

class LEDProtocol {
    public:
        LEDProtocol() {}

        void LED_initialize();
        void PWR_initialize();
        void BMP_initialize();
        void PWR_Pattern();
        void GPS_initialize();
        void IMU_initialize();
        void Transmit_Pattern();

    private:
        const int LED_1 = 3; // Pin for power LED
        const int LED_2 = 2; // Pin for sensor LED
        unsigned long previousMillis = 0;  // Stores last time LED was updated
    
};


#endif // LED_PROTOCOL_H
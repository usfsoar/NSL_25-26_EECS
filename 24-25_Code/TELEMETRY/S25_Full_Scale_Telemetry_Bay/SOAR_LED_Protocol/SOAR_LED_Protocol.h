#ifndef LED_PROTOCOL_H
#define LED_PROTOCOL_H
#include <Arduino.h>

class LEDProtocol {
    public:
        LEDProtocol() {}

        void LED_initialize();
        void PWR_initialize();
        void Sensor_initialize();
        void PWR_Pattern();
        void SD_Pattern();
        void SD_Null_P();
        void Transmit_Pattern();

    private:
        const int LED_1 = 2; // Pin for power LED
        const int LED_2 = 3; // Pin for sensor LED
        unsigned long previousMillis = 0;  // Stores last time LED was updated
    
};


#endif // LED_PROTOCOL_H
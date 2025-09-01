#include "SOAR_LED_Protocol.h"

LEDProtocol led;

void setup() {
    Serial.begin(115200);
    // Initialize the LED protocol
    led.LED_initialize();
    led.PWR_initialize();
    led.Sensor_initialize();
    // led.PWR_Pattern();
    // led.SD_Pattern();
    // led.SD_Null_P();
    // led.Transmit_Pattern();
}

void loop() {
    // led.PWR_Pattern();
    led.SD_Pattern();
    led.SD_Null_P();
    led.Transmit_Pattern();
}
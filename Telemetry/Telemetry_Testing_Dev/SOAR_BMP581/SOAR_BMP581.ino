#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "SOAR_BMP581.h"
#include <math.h>

BMP581Sensor barometer;

float altitude;
float pressure;
float temperature;
float i_altitude;

void setup() {
    Serial.begin(115200);
    barometer.begin();
    vTaskDelay(1000 - portTICK_PERIOD_MS);
    i_altitude = barometer.get_altitude();
    xTaskCreate(
        BMP581Task,
        "BMP581Task",
        10000,
        NULL,
        1,
        NULL
    );
}

void BMP581Task(void *parameters) {
    while (1) {
        altitude = barometer.get_altitude() - i_altitude;
        pressure = barometer.get_pressure();
        temperature = barometer.get_temperature();
        
        Serial.println("Altitude: " + String(altitude));
        
        if (altitude == 0) {
            Serial.println("Altitude is 0, retrying...");
            barometer.begin();
        } 
        
        Serial.println("Pressure: " + String(pressure));
        Serial.println("Temperature: " + String(temperature));
        Serial.println("--------------------------------");
        delay(1000);
    }
}








void loop() {
}
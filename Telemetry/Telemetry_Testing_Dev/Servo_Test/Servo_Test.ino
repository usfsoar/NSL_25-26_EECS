#include <stdio.h>
#include <ESP32Servo.h>

#define SERVO_PIN D4
Servo antenna_servo;

double theta;

void setup(void)
{
    Serial.begin(9600);
    while (!Serial) { delay(100); }
    Serial.println("Serial Initialized");

    antenna_servo.attach(SERVO_PIN);
}

void loop() {
    for (theta = 0; theta <= 180; theta++) {
        Serial.printf("theta: %.0f\n", theta);
        antenna_servo.write(theta);
        delay(15);
    }
    for (theta = 180; theta >= 0; theta--) {
        Serial.printf("theta: %.0f\n", theta);
        antenna_servo.write(theta);
        delay(15);
    }
}


#include "SOAR_LED_Protocol.h"

// Constructor is currently commented out as it's not needed
// LEDProtocol::LEDProtocol() {
//     // Constructor implementation (if needed)
// }

// Sets up initial LED configurations
// Initializes both LEDs as outputs and ensures they start in OFF state
void LEDProtocol::LED_initialize() {
    Serial.begin(115200);  // Initialize serial communication for debugging
    pinMode(LED_1, OUTPUT); // Configure LED_1 pin for output mode
    pinMode(LED_2, OUTPUT); // Configure LED_2 pin for output mode
    digitalWrite(LED_1, LOW); // Ensure LED_1 starts in OFF state
    digitalWrite(LED_2, LOW); // Ensure LED_2 starts in OFF state
}

// Power-up initialization sequence
// Blinks LED_2 three times with 1-second intervals
void LEDProtocol::PWR_initialize() {
    digitalWrite(LED_1, LOW); // Start with LED_1 OFF
    int setup = 0;           // Counter for number of blinks
    // int LED_status = 0;
    
    while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (setup == 3) {
            // After three blinks, turn off LED and exit
            // LED_status = 0;
            digitalWrite(LED_2, LOW);
            break;
        } else if ((currentMillis - previousMillis == 1000) && setup <= 3) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_2, HIGH);
            Serial.println("LED ON");
            setup++;
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_2, LOW);
        }
    }
}

// Sensor initialization sequence
// Single blink pattern for LED_2 while keeping LED_1 on
void LEDProtocol::Sensor_initialize() {
    // digitalWrite(LED_1, HIGH);  // Keep LED_1 ON during sensor initialization
    int setup = 0;             // Counter for blink sequence   

        while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (setup == 1) {
            // After three blinks, turn off LED and exit
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, LOW);
            break;
        } else if ((currentMillis - previousMillis == 300) && setup == 0) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_1, HIGH);
            digitalWrite(LED_2, HIGH);
            Serial.println("LED ON");
            setup++;
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_1, HIGH);
            digitalWrite(LED_2, LOW);
        }
    }
}

// Continuous power indicator pattern
// Blinks LED_2 every 10 seconds while keeping LED_1 off
void LEDProtocol::PWR_Pattern() {
    digitalWrite(LED_1, LOW);  // Keep LED_1 off
    // while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (currentMillis - previousMillis == 10000) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_2, HIGH);
            Serial.println("LED ON");
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_2, LOW);
        }
    // }
}

// SD card operation indicator pattern
// Blinks LED_1 three times while keeping LED_2 off
void LEDProtocol::SD_Pattern() {
    digitalWrite(LED_2, LOW);  // Keep LED_2 off during SD operations
    int setup = 0;            // Counter for blinks

    while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (setup == 3) {
            // After three blinks, turn off LED and exit
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            break;
        } else if ((currentMillis - previousMillis == 500) && setup <= 3) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_1, HIGH);
            Serial.println("LED ON");
            setup++;
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
        }
    }
}

// Error indication pattern
// Blinks both LEDs simultaneously three times
void LEDProtocol::SD_Null_P() {
    int setup = 0;    // Counter for blink sequence
    while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (setup == 3) {
            // After three blinks, turn off LED and exit
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, LOW);
            break;
        } else if ((currentMillis - previousMillis == 500) && setup <= 3) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_1, HIGH);
            digitalWrite(LED_2, HIGH);
            Serial.println("LED ON");
            setup++;
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, LOW);
        }
    }
}

// Transmission indicator pattern
// Keeps LED_2 on and blinks LED_1 three times
void LEDProtocol::Transmit_Pattern() {
    // digitalWrite(LED_2, HIGH);  // Keep LED_2 on during transmission
    int setup = 0;             // Counter for blink sequence

    while (1) {
        unsigned long currentMillis = millis();  // Get current time
        if (setup == 3) {
            // After three blinks, turn off LED and exit
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, LOW);
            break;
        } else if ((currentMillis - previousMillis == 300) && setup <= 3) {
            // Every second, turn LED on and increment counter
            previousMillis = currentMillis;
            // LED_status = 1;
            digitalWrite(LED_1, HIGH);
            digitalWrite(LED_2, HIGH);
            Serial.println("LED ON");
            setup++;
            delay(100);
        } else {
            // In between blinks, keep LED off
            // LED_status = 0;
            digitalWrite(LED_1, LOW);
            digitalWrite(LED_2, HIGH);
        }
    }
    
}
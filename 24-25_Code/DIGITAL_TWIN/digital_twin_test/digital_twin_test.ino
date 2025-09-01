#include "DIGITAL_TWIN.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 IMU Simulation Test...");
}

void loop() {
    static unsigned long lastServoUpdate = 0; // Track last servo update time
    static int servoIndex = 0; // Track current index in the servo sequence
    static const int servoNumbers[] = {0, 4, 8, 12}; // Servo numbers

    // Read and print sensor values every loop iteration
    float altitude = getSimulatedAltitude();
    Serial.println("SIM-ALT: " + String(altitude));

    float temperature = getSimulatedTemperature();
    Serial.println("SIM-TMP: " + String(temperature));

    float pressure = getSimulatedPressure();
    Serial.println("SIM-PRS: " + String(pressure));

    float accX, accY, accZ;
    getSimulatedAcceleration(accX, accY, accZ);
    Serial.println("SIM-ACC: X=" + String(accX) + " Y=" + String(accY) + " Z=" + String(accZ));

    float linAccX, linAccY, linAccZ;
    getSimulatedLinearAcceleration(linAccX, linAccY, linAccZ);
    Serial.println("SIM-ACL: X=" + String(linAccX) + " Y=" + String(linAccY) + " Z=" + String(linAccZ));

    float gravX, gravY, gravZ;
    getSimulatedGravity(gravX, gravY, gravZ);
    Serial.println("SIM-GRV: X=" + String(gravX) + " Y=" + String(gravY) + " Z=" + String(gravZ));

    // **Update servo every 2 seconds**
    if (millis() - lastServoUpdate >= 2000) {
        int testAngle = random(0, 44); // Random angle between 0 and 44
        int currentServo = servoNumbers[servoIndex]; // Get servo number from array

        setSimulatedServo(currentServo, testAngle);
        Serial.println("SIM-SERVO: Servo " + String(currentServo) + " -> Angle " + String(testAngle));

        // Cycle through servo numbers 0, 4, 8, 12
        servoIndex = (servoIndex + 1) % 4; 

        lastServoUpdate = millis(); // Reset timer
    }

    Serial.println("----------------------------------");
    delay(50); // Keep sensor updates frequent but non-blocking
}

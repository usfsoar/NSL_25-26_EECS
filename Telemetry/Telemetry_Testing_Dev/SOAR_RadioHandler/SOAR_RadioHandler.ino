#include <Arduino.h>
#include "RadioHandler.h"

RadioHandler radio(Serial3);

void setup() {
    Serial.begin(115200); //set this baud to command
    while (!Serial) { }

    Serial.println("=== DRA818 Radio Test ===");

    // Initialize the radio UART + PTT pin
    radio.begin(9600);

    Serial.println("Commands:");
    Serial.println("  tx               - enable transmit mode");
    Serial.println("  rx               - enable receive mode");
    Serial.println("  send <text>      - send text over radio");
    Serial.println("  read             - read received buffer");
    Serial.println("  status           - show radio mode");
}

void loop() {

    // Check if radio has received data
    if (radio.available()) {
        String msg = radio.readMessage();
        Serial.print("[RADIO] ");
        Serial.println(msg);
    }

    // Check USB Serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd.equalsIgnoreCase("tx")) {
            radio.setMode(RADIO_TX_MODE);
            Serial.println("PTT → TX mode");

        } else if (cmd.equalsIgnoreCase("rx")) {
            radio.setMode(RADIO_RX_MODE);
            Serial.println("PTT → RX mode");

        } else if (cmd.startsWith("send ")) {
            String payload = cmd.substring(5);
            radio.sendMessage(payload);
            Serial.println("Sent.");

        } else if (cmd.equalsIgnoreCase("read")) {
            if (radio.available()) {
                Serial.println(radio.readMessage());
            } else {
                Serial.println("(no data)");
            }

        } else if (cmd.equalsIgnoreCase("status")) {
            Serial.print("Radio mode: ");
            Serial.println("RX/TX depends on PTT pin");

        } else {
            Serial.println("Unknown command.");
        }
    }
}

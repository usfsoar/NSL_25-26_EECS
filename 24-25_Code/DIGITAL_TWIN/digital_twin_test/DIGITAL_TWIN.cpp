#include "DIGITAL_TWIN.h"
#include "_config.h"

#if SIMULATION
float getSimFloat(byte req_byte1, byte req_byte2, byte res_byte) {
  Serial.write(req_byte1);
  Serial.write(req_byte2);

  uint32_t timeout_start = millis();
  while (millis() - timeout_start < 1000) {
    if (Serial.available()) {
      byte responseCode = Serial.read();
      
      if (responseCode == res_byte) {
        union {
          float value;
          byte bytes[4];
        } data;

        for (int i = 0; i < 4; i++) {
          if (Serial.available()) {
            data.bytes[i] = Serial.read();
          } else {
            return 44330.00;  // Default broken sensor value
          }
        }
        return data.value;
      }
    }
  }
  return 44330.00;  // Timeout default
}

void getSimVector3(byte req_byte1, byte req_byte2, byte res_byte, float &x, float &y, float &z) {
  Serial.write(req_byte1);
  Serial.write(req_byte2);

  uint32_t timeout_start = millis();
  while (millis() - timeout_start < 1000) {
    if (Serial.available()) {
      byte responseCode = Serial.read();
      
      if (responseCode == res_byte) {
        union {
          float value;
          byte bytes[4];
        } data;

        // Read X
        for (int i = 0; i < 4; i++) {
          if (Serial.available()) {
            data.bytes[i] = Serial.read();
          } else {
            x = 0;  // Default value
            return;
          }
        }
        x = data.value;

        // Read Y
        for (int i = 0; i < 4; i++) {
          if (Serial.available()) {
            data.bytes[i] = Serial.read();
          } else {
            y = 0;
            return;
          }
        }
        y = data.value;

        // Read Z
        for (int i = 0; i < 4; i++) {
          if (Serial.available()) {
            data.bytes[i] = Serial.read();
          } else {
            z = 0;
            return;
          }
        }
        z = data.value;
        return;
      }
    }
  }
  x = 0;
  y = 0;
  z = 0;
}

float getSimulatedAltitude(){
    return getSimFloat(0x03, 0x01, 0x03);
}
float getSimulatedTemperature(){
    return getSimFloat(0x03, 0x02, 0x03);
}

float getSimulatedPressure(){
    return getSimFloat(0x03, 0x03, 0x03);
}

void getSimulatedAcceleration(float &x, float &y, float &z) {
    getSimVector3(0x04, 0x01, 0x04, x, y, z);
}
void getSimulatedLinearAcceleration(float &x, float &y, float &z) {
    getSimVector3(0x04, 0x02, 0x04, x, y, z);
}
void getSimulatedGravity(float &x, float &y, float &z) {
    getSimVector3(0x04, 0x03, 0x04, x, y, z);
}
#endif

#if SIMULATION_OUT

void setSimIntInt(byte req_byte1, byte req_byte2, byte res_byte, int value1, int value2) {
    Serial.write(req_byte1);
    Serial.write(req_byte2);

    union {
        int intValue;
        byte bytes[4];
    } data1, data2;

    data1.intValue = value1; // First integer (e.g., servo number)
    data2.intValue = value2; // Second integer (e.g., servo angle)

    for (int i = 0; i < 4; i++) {
        Serial.write(data1.bytes[i]);
    }
    for (int i = 0; i < 4; i++) {
        Serial.write(data2.bytes[i]);
    }

    uint32_t timeout_start = millis();
    while (millis() - timeout_start < 1000) {
        if (Serial.available()) {
            byte responseCode = Serial.read();
            if (responseCode == res_byte) {
                Serial.println("Simulation confirmed setSimIntInt()");
                return;
            } else {
                Serial.println("Incorrect response code for setSimIntInt()");
            }
        }
    }
    Serial.println("Timeout: No response from simulation for setSimIntInt()");
}

void setSimulatedServo(int servoNumber, int angle) {
    setSimIntInt(0x05, 0x01, 0x05, servoNumber, angle);
}

#endif


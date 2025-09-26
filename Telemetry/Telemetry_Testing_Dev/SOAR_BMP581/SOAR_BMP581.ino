#include "SOAR_BMP581.h"

void loop() {
  Serial.println("Loop running...");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup running...");
}

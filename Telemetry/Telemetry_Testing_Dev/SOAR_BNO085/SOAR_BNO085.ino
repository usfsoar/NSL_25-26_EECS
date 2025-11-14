#include <Adafruit_BNO08x.h>
#include "SOAR_BNO085.h"

SOAR_BNO085 imu;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("Reading events");
  delay(1000);
}

void loop() {
    int i;
    // all delay inside loop
    for (i = 0; i < 10; i++) {
        delay(5);
        imu.update();
    }
    imu.showState();
}


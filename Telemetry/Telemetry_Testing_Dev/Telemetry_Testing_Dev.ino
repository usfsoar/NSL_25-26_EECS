#include <Adafruit_BNO08x.h>
#include "SOAR_BMP581/SOAR_BMP581.h"
#include "SOAR_BMP581/SOAR_BMP581.cpp"
#include "SOAR_BNO085/SOAR_BNO085.h"
#include "SOAR_BNO085/SOAR_BNO085.cpp"
#include "Kalman_Filter/kalman.h"
#include "Kalman_Filter/vertical_acceleration.h"


SOAR_BNO085 imu;
BMP581Sensor barometer;

matrix * quat;
matrix * dir;
matrix * acc;
double magnitude;
float altitude;
float pressure;
float temperature;
float i_altitude;
int i;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  //if (CrashReport) { Serial.print(CrashReport); }

  imu.begin();
  barometer.begin();
  delay(1000);
  i_altitude = barometer.get_altitude();

  quat = matrixCreate(4, 1);
  dir = matrixCreate(3, 1);
  acc = matrixCreate(3, 1);
  setElement(dir, 3, 1, 1);

  Serial.println("Reading events");
  delay(1000);
}

void loop() {
    delay(1000);
    altitude = barometer.get_altitude() - i_altitude;
    Serial.println("Altitude: " + String(altitude));
        
    if (altitude == 0) {
        Serial.println("Altitude is 0, retrying...");
        barometer.begin();
    } 

    // all delay inside loop
    for (i = 0; i < 10; i++) {
        delay(5);
        imu.update();
    }

    setElement(quat, 1, 1, imu.sensorData.orientation.w);
    setElement(quat, 2, 1, imu.sensorData.orientation.x);
    setElement(quat, 3, 1, imu.sensorData.orientation.y);
    setElement(quat, 4, 1, imu.sensorData.orientation.z);

    setElement(acc, 1, 1, imu.sensorData.linearAcceleration.x);
    setElement(acc, 2, 1, imu.sensorData.linearAcceleration.y);
    setElement(acc, 3, 1, imu.sensorData.linearAcceleration.z);

    vectorComponent(acc, quat, dir, &magnitude);

    /*
    Serial.printf("acc_vec (%.2f, %.2f, %.2f)\n", imu.sensorData.acceleration.x, 
                                                  imu.sensorData.acceleration.y,
                                                  imu.sensorData.acceleration.z);

    Serial.printf("quat (%.2f, %.2f, %.2f, %.2f)\n", imu.sensorData.orientation.w,
                                                     imu.sensorData.orientation.x, 
                                                     imu.sensorData.orientation.y,
                                                     imu.sensorData.orientation.z);
    */

    Serial.printf("vert acc: %.2lf\n", magnitude);

    //imu.showState();
}

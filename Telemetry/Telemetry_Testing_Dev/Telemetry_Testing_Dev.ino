#include <Adafruit_BNO08x.h>
#include "SOAR_BMP581/SOAR_BMP581.h"
#include "SOAR_BMP581/SOAR_BMP581.cpp"
#include "SOAR_BNO085/SOAR_BNO085.h"
#include "SOAR_BNO085/SOAR_BNO085.cpp"
#include "Kalman_Filter/kalman.h"
#include "Kalman_Filter/matrix.h"
#include "Kalman_Filter/extras.h"


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

double t;
const double dt = 0.05; /* must be accurate to data rate */
const double sigma_j = 0.2; /* process StdDev: TUNED */
const double sigma_s = 0.1666667; /* altitude reading StdDev */
const double sigma_a = 0.179; /* acceleration reading StdDev */
const int states = 3;
const int observations = 2;
const double MIN_ALT = 1; /* trust sensors below this altitude */
kalmanFilter *filter = NULL;

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

  filter = kalmanFilterCreate(states, observations);
  
  /* set filter matrices and vectors matrices
   * matrices are initially all 0s so no need to set them
   * initialize x_k_prev and P_k_prev not current time */
  
  /* F_k for constant acceleration assumption */
  setElement(filter->F_k, 1, 1, 1);
  setElement(filter->F_k, 1, 2, dt);
  setElement(filter->F_k, 1, 3, 0.5*dt*dt);
  setElement(filter->F_k, 2, 2, 1);
  setElement(filter->F_k, 2, 3, dt);
  setElement(filter->F_k, 3, 3, 1);
  
  setElement(filter->H_k, 1, 1, 1);
  setElement(filter->H_k, 2, 3, 1);
  
  /* process covar */
  setElement(filter->Q_k, 1, 1, pow(sigma_j, 2) * (1.0/36) * pow(dt, 6));
  setElement(filter->Q_k, 1, 2, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 1, 3, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 1, pow(sigma_j, 2) * (1.0/12) * pow(dt, 5));
  setElement(filter->Q_k, 2, 2, pow(sigma_j, 2) * (1.0/4 ) * pow(dt, 4));
  setElement(filter->Q_k, 2, 3, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 1, pow(sigma_j, 2) * (1.0/6 ) * pow(dt, 4));
  setElement(filter->Q_k, 3, 2, pow(sigma_j, 2) * (1.0/2 ) * pow(dt, 3));
  setElement(filter->Q_k, 3, 3, pow(sigma_j, 2) *  1.0     * pow(dt, 2));
  
  /* reading covar */
  setElement(filter->R_k, 1, 1, sigma_s * sigma_s);
  setElement(filter->R_k, 2, 2, sigma_a * sigma_a);
  
  /* uncertainty of initial velocity and acceleration */
  setElement(filter->P_k_prev, 2, 2, 100);
  setElement(filter->P_k_prev, 3, 3, 100);

  Serial.println("Reading events");
  delay(1000);
}

void loop() {
    // all delay inside loop
    for (i = 0; i < 10; i++) {
        delay(5);
        imu.update();
    }

    altitude = barometer.get_altitude() - i_altitude;
    Serial.println("Altitude: " + String(altitude));
        
    if (altitude == 0) {
        Serial.println("Altitude is 0, retrying...");
        barometer.begin();
    }

    if (altitude < MIN_ALT) { // fully trust sensors
        setElement(filter->P_k_prev, 2, 2, 100);
        setElement(filter->P_k_prev, 3, 3, 100);
    }

    setElement(quat, 1, 1, imu.sensorData.orientation.w);
    setElement(quat, 2, 1, imu.sensorData.orientation.x);
    setElement(quat, 3, 1, imu.sensorData.orientation.y);
    setElement(quat, 4, 1, imu.sensorData.orientation.z);

    setElement(acc, 1, 1, imu.sensorData.linearAcceleration.x);
    setElement(acc, 2, 1, imu.sensorData.linearAcceleration.y);
    setElement(acc, 3, 1, imu.sensorData.linearAcceleration.z);

    vectorComponent(acc, quat, dir, &magnitude);

    // Serial.printf("acc_vec (%.2f, %.2f, %.2f)\n", imu.sensorData.linearAcceleration.x, imu.sensorData.linearAcceleration.y, imu.sensorData.linearAcceleration.z);
    // Serial.printf("quat (%.2f, %.2f, %.2f, %.2f)\n", imu.sensorData.orientation.w, imu.sensorData.orientation.x, imu.sensorData.orientation.y, imu.sensorData.orientation.z);
    // Serial.printf("vert acc: %.2lf\n", magnitude);

    setElement(filter->z_k, 1, 1, altitude);
    setElement(filter->z_k, 2, 1, magnitude);
    kalmanFilterPredict(filter);
    kalmanFilterUpdate(filter);

    Serial.println("State: ");
    matrixPrintArduino(filter->x_k);
}

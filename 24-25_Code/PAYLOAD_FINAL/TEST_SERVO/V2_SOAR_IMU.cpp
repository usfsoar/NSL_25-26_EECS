#include "V2_SOAR_IMU.h"

SOAR_IMU::SOAR_IMU()
  : bno(55) {
  this->bno = Adafruit_BNO055(55);
}
// Implement other methods here

void SOAR_IMU::BNO_SETUP() {

  Serial.println("Orientation Sensor Test");
  Serial.println("");

  /* Initialise the sensor */
  if (!this->bno.begin()) {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
  }

  delay(1000);
  this->bno.setExtCrystalUse(true);
  Serial.print("Successful\n");
}


float *SOAR_IMU::GET_GRAVITY(void) {

  //read imu linear acceleration vector and return an array of float
  //upon failure, call setup class from this function reset the BNO055

  float *gravity = new float[3];
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

  gravity[0] = g.x();
  gravity[1] = g.y();
  gravity[2] = g.z();

  return gravity;
}
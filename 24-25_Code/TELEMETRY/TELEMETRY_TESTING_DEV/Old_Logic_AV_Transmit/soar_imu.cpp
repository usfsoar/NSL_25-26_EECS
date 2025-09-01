#include "SOAR_IMU.h"

SOAR_IMU::SOAR_IMU() {
  this->bno = Adafruit_BNO055(55);
  // Constructor implementation
  // Initialize variables if needed
  prev_accel_x = 0;
  prev_accel_y = 0;
  prev_accel_z = 0;
  loop_iterations = 0;

}
// Implement other methods here

void SOAR_IMU::BNO_SETUP() {

  Serial.println("Orientation Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!this->bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
  }
  
  delay(1000);
  this->bno.setExtCrystalUse(true);
  Serial.print("Successful\n");
}

float *SOAR_IMU::GET_ACCELERATION(void) {

//read imu linear acceleration vector and return an array of float
//upon failure, call setup class from this function reset the BNO055

float *acceleration = new float[3];

  // if(fail_count <= 50){
  //   imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  //   acceleration[0] = a.x();
  //   acceleration[1] = a.y();
  //   acceleration[2] = a.z();

  //   for(int i=0; i<3; i++){
  //     if (acceleration[i] == 0.0){
  //       fail_count ++;
  //     }
  //     else {
  //       fail_count = 0;
  //     }
  //   }
  // }
  // else if(millis()>15000){ //Sensor will output incorrect readings for the first seconds while it sets up
  //   Serial.println("fail_count > 20. Resetting...");
  //   // this->BNO_SETUP();
  //   fail_count = 0;
  // }  
  // return acceleration;


  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

  acceleration[0] = a.x();
  acceleration[1] = a.y();
  acceleration[2] = a.z();

  return acceleration;


}


float *SOAR_IMU::GET_LINEARACCEL(void) {

//read imu linear acceleration vector and return an array of float
//upon failure, call setup class from this function reset the BNO055

  float *lin_accel = new float[3];

  // if(fail_count <= 20){
  //   imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  //   lin_accel[0] = a.x();
  //   lin_accel[1] = a.y();
  //   lin_accel[2] = a.z();

  //   for(int i=0; i<3; i++){
  //     if (lin_accel[i] == 0.0){
  //       fail_count ++;
  //     }
  //     else {
  //       fail_count = 0;
  //     }
  //   }
  // }
  // else {
  //   Serial.print("fail_count > 20, resetting...");
  //   this->BNO_SETUP();
  // }  
  // return lin_accel;


  imu::Vector<3> a = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  lin_accel[0] = a.x();
  lin_accel[1] = a.y();
  lin_accel[2] = a.z();

  return lin_accel;


}

float *SOAR_IMU::GET_GRAVITY(void) {

//read imu linear acceleration vector and return an array of float
//upon failure, call setup class from this function reset the BNO055

  float *gravity = new float[3];

  // if(fail_count <= 20){
  //   imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

  //   gravity[0] = g.x();
  //   gravity[1] = g.y();
  //   gravity[2] = g.z();

  //   for(int i=0; i<3; i++){
  //     if (gravity[i] == 0.0){
  //       fail_count ++;
  //     }
  //     else {
  //       fail_count = 0;
  //     }
  //   }
  // }
  // else {
  //   Serial.print("fail_count > 20, resetting...");
  //   this->BNO_SETUP();
  // }  
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

  gravity[0] = g.x();
  gravity[1] = g.y();
  gravity[2] = g.z();

  return gravity;

}

float *SOAR_IMU::GET_GYROSCOPE(void) {

  float *gyro = new float[3];

  // if(fail_count <= 20){
  //   imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

  //   gyro[0] = g.x();
  //   gyro[1] = g.y();
  //   gyro[2] = g.z();

  //   for(int i=0; i<3; i++){
  //     if (gyro[i] == 0.0){
  //       fail_count ++;
  //     }
  //     else {
  //       fail_count = 0;
  //     }
  //   }
  // }
  // else {
  //   Serial.print("fail_count > 20, resetting...");
  //   this->BNO_SETUP();
  // }  
  
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

  gyro[0] = g.x();
  gyro[1] = g.y();
  gyro[2] = g.z();


  return gyro;

}


float *SOAR_IMU::GET_QUAT(void) {

  float *quat = new float[4];
  // if(fail_count <= 20){
  imu::Quaternion q = bno.getQuat();

    quat[0] = q.x();
    quat[1] = q.y();
    quat[2] = q.z();
    quat[3] = q.w();

  //   for(int i=0; i<3; i++){
  //     if (quat[i] == 0.0){
  //       fail_count ++;
  //     }
  //     else {
  //       fail_count = 0;
  //     }
  //   }
  // }
  // else {
  //   Serial.print("fail_count > 20, resetting...");
  //   this->BNO_SETUP();
  // }  
  return quat;


}



float *SOAR_IMU::GET_VELOCITY(void) {

  float *accel = GET_LINEARACCEL();
  float *velocity = new float[3];
  
  if(loop_iterations < 1){
    prev_accel_x = accel[0];
    prev_accel_y = accel[1];
    prev_accel_z = accel[2];
    prev_time = millis();
  }

  if(loop_iterations >= 1){
    //calculations using right hand sum rects and triangles
    uint32_t delta_time = millis() - prev_time;
    float delta_time_sec = delta_time / 1000.0;
    velocity[0] += (delta_time_sec * prev_accel_x) + (0.5 * delta_time_sec * accel[0]);
    velocity[1] += (delta_time_sec * prev_accel_y) + (0.5 * delta_time_sec * accel[1]);
    velocity[2] += (delta_time_sec * prev_accel_z) + (0.5 * delta_time_sec * accel[2]);
  }

  prev_accel_x = accel[0];
  prev_accel_y = accel[1];
  prev_accel_z = accel[2];
  prev_time = millis();

  return velocity;

}


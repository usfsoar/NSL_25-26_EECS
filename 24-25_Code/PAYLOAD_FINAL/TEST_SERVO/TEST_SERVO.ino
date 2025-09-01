#include "SOAR_PAYLOAD_SERVO.h"
#include "V2_SOAR_IMU.h"

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

SOAR_PAYLOAD_SERVO p_serv;
SOAR_IMU imu2;
//SOAR_BAROMETER baro;

void setup() {
  Serial.begin(115200);
  p_serv.initialize(pwm);
  imu2.BNO_SETUP();
  //baro.Initialize();

  p_serv.setServoAngle(pwm, 0, 0);
  p_serv.setServoAngle(pwm, 4, 0);
  p_serv.setServoAngle(pwm, 8, 0);
  p_serv.setServoAngle(pwm, 12, 0);
  delay(1000);    
}

void loop() {
  // p_serv.setServoAngle(pwm, 0, 0);
  // p_serv.setServoAngle(pwm, 4, 0);
  // p_serv.setServoAngle(pwm, 8, 0);
  // p_serv.setServoAngle(pwm, 12, 0);
  // delay(1000);    

  // p_serv.setServoAngle(pwm, 0, 30);
  // p_serv.setServoAngle(pwm, 4, 30);
  // p_serv.setServoAngle(pwm, 8, 30);
  // p_serv.setServoAngle(pwm, 12, 30);
  // delay(1000);    
  float *gravity = imu2.GET_GRAVITY();
  int servo = p_serv.servoLogic(pwm, gravity);
  Serial.println(gravity[0]);
  Serial.println(gravity[1]);
  Serial.println(gravity[2]);
  Serial.println(servo);
  Serial.println("--------------------------");
  delay(1500);
}
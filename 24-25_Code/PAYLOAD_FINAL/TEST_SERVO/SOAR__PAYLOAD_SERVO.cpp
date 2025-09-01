#include "SOAR_PAYLOAD_SERVO.h"
#include "V2_SOAR_IMU.h"


SOAR_PAYLOAD_SERVO::SOAR_PAYLOAD_SERVO() {
    // Constructor
}

void SOAR_PAYLOAD_SERVO::initialize(Adafruit_PWMServoDriver& pwm) {
    pwm.begin();
    pwm.setPWMFreq(60); // Set the PWM frequency to 60 Hz
}

void SOAR_PAYLOAD_SERVO::setServoAngle(Adafruit_PWMServoDriver& pwm, int channel, int angle) {
    // Map the angle to the pulse width
    int pulseLength = map(angle, 0, 180, SERVOMIN, SERVOMAX);
    pwm.setPWM(channel, 0, pulseLength);
}

bool SOAR_PAYLOAD_SERVO::servoLogic(Adafruit_PWMServoDriver& pwm, float *gravity) {
  if (abs(gravity[0]) < abs(gravity[2])) {
    if ( gravity[2] > 0 ) {
      setServoAngle(pwm, 0, 0);
      setServoAngle(pwm, 4, 44);
      setServoAngle(pwm, 8, 0);
      setServoAngle(pwm, 12, 0);
      return 4;
    }
    else {
      setServoAngle(pwm, 0, 0);
      setServoAngle(pwm, 4, 0);
      setServoAngle(pwm, 8, 0);
      setServoAngle(pwm, 12, 44);
      return 12;
    }
  }

  else {
    if ( gravity[0] > 0 ) {
      setServoAngle(pwm, 0, 0);
      setServoAngle(pwm, 4, 0);
      setServoAngle(pwm, 8, 44);
      setServoAngle(pwm, 12, 0);
      return 8;
    }
    else {
      setServoAngle(pwm, 0, 44);
      setServoAngle(pwm, 4, 0);
      setServoAngle(pwm, 8, 0);
      setServoAngle(pwm, 12, 0);
      return 0;
      }
  }
  return 0;
}


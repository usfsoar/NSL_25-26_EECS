#include "SOAR_PAYLOAD_SERVO.h"
#include "SOAR_IMU.h"
#include "_config.h"
#include "DIGITAL_TWIN.h"


SOAR_PAYLOAD_SERVO::SOAR_PAYLOAD_SERVO() {
    // Constructor
}

void SOAR_PAYLOAD_SERVO::initialize(Adafruit_PWMServoDriver& pwm) {
    pwm.begin();
    pwm.setPWMFreq(60); // Set the PWM frequency to 60 Hz
}

void SOAR_PAYLOAD_SERVO::setServoAngle(Adafruit_PWMServoDriver& pwm, int channel, int angle) {
#if !SIMULATION_OUT
    // Map the angle to the pulse width
    int pulseLength = map(angle, 0, 180, SERVOMIN, SERVOMAX);
    pwm.setPWM(channel, 0, pulseLength);
#else
    setSimulatedServo(channel, angle);
#endif
}

bool SOAR_PAYLOAD_SERVO::servoLogic(Adafruit_PWMServoDriver& pwm, float *velocity, float *gravity) {
    // if (velocity[0] <= 0.05 && velocity[0] >= -0.05) {
        if (gravity[1] >= 7.5 | gravity[1] <= -7.5) {
            setServoAngle(pwm, 0, 44.4);
            setServoAngle(pwm, 4, 44.4);
            setServoAngle(pwm, 8, 44.4);
            setServoAngle(pwm, 12, 44.4);
            return true;
        } else if (gravity[2] < -8 && gravity[0] > -4) {
            setServoAngle(pwm, 0, 44.4);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 0);
            return true;
        } else if (gravity[2] > -8 && gravity[2] < -5 && gravity[0] < -5) {
            setServoAngle(pwm, 0, 44.4);
            setServoAngle(pwm, 4, 44.4);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 0);
            return true;
        } else if (gravity[2] > -4 && gravity[0] <= -8) {
            setServoAngle(pwm, 0, 0);
            setServoAngle(pwm, 4, 44.4);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 0);
            return true;
        } else if (gravity[2] > 5 && gravity[0] > -8 && gravity[0] < -5) {
            setServoAngle(pwm, 0, 0);
            setServoAngle(pwm, 4, 44.4);
            setServoAngle(pwm, 8, 44.4);
            setServoAngle(pwm, 12, 0);
            return true;
        } else if (gravity[2] > 8 && gravity[0] <= -5) {
            setServoAngle(pwm, 0, 0);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 44.4);
            setServoAngle(pwm, 12, 0);
            return true;
        } else if (gravity[2] < 8 && gravity[2] > 5 && gravity[0] > 5) {
            setServoAngle(pwm, 0, 0);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 44.4);
            setServoAngle(pwm, 12, 44.4);
            return true;    
        }else if (gravity[2] < 5 && gravity[0] > 8) {
            setServoAngle(pwm, 0, 0);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 44.4);
            return true;
        } else if (gravity[2] < -5 && gravity[0] > 5 && gravity[0] < 8) {
            setServoAngle(pwm, 0, 44.4);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 44.4);
            return true;
        } else if (gravity[2] < -8 && gravity[0] > 5) {
            setServoAngle(pwm, 0, 44.4);
            setServoAngle(pwm, 4, 0);
            setServoAngle(pwm, 8, 0);
            setServoAngle(pwm, 12, 0);
            return true;
        } else {
            return false;
        }
        // } else if (gravity[0] > 0 && gravity[2] == 0) {
        //     setServoAngle(pwm, 0, 180);
        //     return true
        // }
        // else if (gyroscope[0] > 0 && gyroscope[2] > 0) {
        //     setServoAngle(pwm, 0, 180);
        //     setServoAngle(pwm, 12, 180);
        //     return true;
        // } 
        // else if (gyroscope[0] > 0 && gyroscope[2] < 0) {
        //     setServoAngle(pwm, 0, 180);
        //     setServoAngle(pwm, 8, 180);
        //     return true;
        // }
        // else if (gyroscope[0] < 0 && gyroscope[2] == 0) {
        //     setServoAngle(pwm, 5, 180);
        //     return true;
        // } 
        // else if (gyroscope[0] < 0 && gyroscope[2] > 0) {
        //     setServoAngle(pwm, 5, 180);
        //     setServoAngle(pwm, 0, 180);
        //     return true;
        // }
        // else if (gyroscope[0] < 0 && gyroscope[2] < 0) {
        //     setServoAngle(pwm, 5, 180);
        //     setServoAngle(pwm, 8, 180);
        //     return true;
        // }
        // else if (gyroscope[0] == 0 && gyroscope[2] > 0) {
        //     setServoAngle(pwm, 8, 180);
        //     return true;
        // } else if (gyroscope[0] > 0  && gyroscope[2] > 0) {
        //     setServoAngle(pwm, 8, 180);
        //     setServoAngle(pwm, 5, 180);
        //     return true;
        // }
        // else if (gyroscope[0] < 0 && gyroscope[2] > 0) {
        //     setServoAngle(pwm, 8, 180);
        //     setServoAngle(pwm, 12, 180);
        //     return true;
        // } else if (gyroscope[0] == 0 && gyroscope[2] < 0) {
        //     setServoAngle(pwm, 12, 180);
        //     return true;
        // } else if (gyroscope[0] > 0 && gyroscope[2] < 0) {
        //     setServoAngle(pwm, 12, 180);
        //     setServoAngle(pwm, 0, 180);
        //     return true;
        // } else if (gyroscope[0] < 0 && gyroscope[2] < 0) {
        //     setServoAngle(pwm, 12, 180);
        //     setServoAngle(pwm, 8, 180);
        //     return true;
        // } else {
 
        //     return false;
        // }
    // } else {
    //     return false;
    // }
}
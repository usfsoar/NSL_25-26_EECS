# Class to calculate how much the servo needs to actuate based on error
# dt and prevError need to be calculated in MAIN

import time
import math
import runge_kutta

SLEW_RATE = 3885.59

class PID:
    def __init__(self, gainP, gainI, gainD, time = 0, apogee = 3048): #constructor
        self.time = time
       
        self.gainP = gainP
        self.gainI = gainI
        self.gainD = gainD

        self.apogee = apogee
        self.minSteps = 0
        self.maxSteps = 14400

        self.dt = .01
        self.derivativeValue = 0
        self.integralValue = 0
        self.errorValue = 0
        self.prevError = 0
        self.pidOutput = 0

        self.altitude = 0
        self.velocity = 0
        self.acceleration = 0
        self.projHeight = 0

        self.rk4 = runge_kutta.RungeKutta4()

        self.currStep = 0
        self.targetStep = 0


    def update(self, time, altitude, velocity, acceleration, dt):
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity
        self.dt = max(dt, 0.01)

        maxMove = SLEW_RATE * self.dt

        if self.currStep < self.targetStep:
            self.currStep = min(self.currStep + maxMove, self.targetStep)

        elif self.currStep > self.targetStep:
            self.currStep = max(self.currStep - maxMove, self.targetStep)

    def error(self):
        self.projHeight = self.rk4.prediction(self.time, self.altitude, self.velocity, self.currStep, self.targetStep) 
        self.errorValue = self.projHeight - self.apogee
        return self.errorValue

    def proportional(self):
        self.proportionalValue = self.gainP * self.errorValue
        return self.proportionalValue
    
    def integralSum(self, rawPidOutput):
        if self.shouldIntegrate(rawPidOutput):
            self.integralValue += self.errorValue * self.dt
        return self.integralValue

    def integral(self): #past error
        return self.integralValue * self.gainI

    def derivative(self): #future error
        if self.dt <= 0:
            self.derivativeValue = 0
        else:
            self.derivativeValue = ((self.errorValue - self.prevError)/ self.dt) * self.gainD
        return self.derivativeValue
    
    def pidSum(self):
        p = self.proportional()
        d = self.derivative()

        beforeIntegral = p + d
        self.integralSum(beforeIntegral)

        i = self.integral()

        self.pidOutput = p + i + d
        self.prevError = self.errorValue
        return self.pidOutput
            
    def motorInput(self, pidOutput):
        clampedSteps = int(max(self.minSteps, min(pidOutput, self.maxSteps)))
        self.targetStep = clampedSteps
        return clampedSteps

    def shouldIntegrate(self, raw_pid_output): #anti-windup clamp
        at_max = raw_pid_output >= self.maxSteps
        at_min = raw_pid_output <= self.minSteps

        error_pushes_higher = self.errorValue > 0
        error_pushes_lower = self.errorValue < 0

        if at_max and error_pushes_higher:
            return False

        if at_min and error_pushes_lower:
            return False

        return True
    
if __name__ == '__main__':
    pass

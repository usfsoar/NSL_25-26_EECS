# Class to calculate how much the servo needs to actuate based on error
# dt and prevError need to be calculated in MAIN

import time
import math
import runge_kutta

class PID:
    def __init__(self, gainP, gainI, gainD, time, apogee, flapArea): #constructor
        self.time = time
       
        self.gainP = gainP
        self.gainI = gainI
        self.gainD = gainD

        self.apogee = apogee
        self.flapArea = flapArea
        self.minSteps = 0
        self.maxSteps = 600 #6000/100 NEEDS TO BE REVISED

        self.dt = 1
        self.derivativeValue = 0
        self.integralValue = 0
        self.errorValue = 0
        self.prevError = 0
        self.pidOutput = 0

        self.rk4 = runge_kutta.RungeKutta4()

    def update(self, time, altitude, velocity, acceleration, dt):
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity
        self.dt = dt

    def error(self):
        self.projHeight = runge_kutta.step(self.time, self.altitude, self.velocity, self.acceleration)
        self.errorValue = self.apogee - self.projHeight #projHeight from runge_kutta
        return self.errorValue

    def proportional(self):
        self.proportionalValue = self.gainP * self.errorValue
        return self.proportionalValue
    
    def integralSum(self):
        if not self.clamp():
            self.integralValue += (self.errorValue * self.dt)
        return self.integralValue
    
    def integral(self): #past error
        return self.integralValue * self.gainI

    def derivative(self): #future error
        self.derivativeValue = ((self.errorValue - self.prevError)/ self.dt) * self.gainD
        return self.derivativeValue

    def pidSum(self):
        self.integralSum()
        self.pidOutput = self.proportional() + self.integral() + self.derivative()
        return self.pidOutput
            
    def motorInput(self):
        pass # we need the conversion rate from pid output to formula

    def clamp(self): #inputSteps being the number of steps the code demands

        if (self.pidOutput >= self.minSteps and self.pidOutput <= self.maxSteps):
            return False

        if (self.errorValue > 0 and self.pidOutput > 0) or (self.errorValue < 0 and self.pidOutput < 0):
            return False

        return True

if __name__ == '__main__':
    pass


#WE WANT STEP COUNT AS OUTPUT FOR FIRST LAUNCH
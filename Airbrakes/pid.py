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
        self.maxSteps = 60 #6000/100 NEEDS TO BE REVISED

        self.dt = 0
        self.derivativeValue = 0
        self.integralValue = 0
        self.errorValue = 0
        self.prevError = 0
        self.pidOutput = 0

    def update(self, time, acceleration, altitude, velocity):
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity

    def error(self):
        self.projHeight = runge_kutta.prediction(self.time, self.altitude, self.velocity)
        self.errorValue = self.apogee - self.projHeight #projHeight from runge_kutta
        return self.errorValue

    def proportional(self):
        self.proportionalValue = self.gainP * self.errorValue
        return self.proportionalValue
    
    def integralSum(self):
        self.integralValue = self.integralValue + (self.errorValue * self.dt)
        return self.integralValue
    
    def integral(self): #past error
        return self.integralValue * self.gainI

    def derivative(self): #future error
        self.derivativeValue = ((self.errorValue - self.prevError)/ self.dt) * self.gainD
        return self.derivativeValue

    def pidSum(self):
        self.update(self, self.time, self.acceleration, self.altitude, self.velocity)
        self.pidOutput = self.proportionalValue + self.integralValue + self.derivativeValue
        return self.pidOutput
            
    def motorInput(self):
        pass # we need the conversion rate from pid output to formula

    def clamp(self, inputSteps): #inputSteps being the number of steps the code demands
        resetIntegral = False

        if (inputSteps <= self.minSteps or inputSteps >= self.maxSteps):
            resetIntegral = True

        if (self.errorValue > 0 and self.pidOutput < 0) or (self.errorValue < 0 and self.pidOutput > 0):
            resetIntegral = True

        if resetIntegral == True:
            self.integralValue = 0
            return self.integralValue

if __name__ == '__main__':
    pass


#WE WANT STEP COUNT AS OUTPUT FOR FIRST LAUNCH
import rk4
import math

class PID:
    
    
    def __init__(self,target_height, time, acceleration, altitude, velocity, kp=0.029077419025980388, ki=0.004654030147673228, kd=0.012026157097838181):
        self.target_height = target_height
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity
        self.first_pass = True
        self.pid_max_output = 0.0
        self.integral = 0
        self.derivative = 0.0
        self.dt = 0.1
        self.prev_error = 0.0
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.flaps_area = 0.0
        self.flaps_length = 0.0
        self.discriminant = 0.0
        self.new_angle = 3
    def update(self, time, acceleration, altitude, velocity):
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity

    def run_PID(self):
        #print("entering rk4")
        self.proj_height = rk4.prediction(self.time, self.altitude, self.velocity, self.dt, self.flaps_length)
        error = self.proj_height - self.target_height
        #print("calculating PID")
        self.integral = self.integral + (error * self.dt)
        proportion = error * self.kp
        derivative = self.kd*((error - self.prev_error) / self.dt)
        integral = self.ki * self.integral
        self.prev_error = error
        self.derivative = derivative
        self.pid_output = proportion + integral + derivative
        if(self.pid_output<=0):
            return self.new_angle
        if(self.first_pass):
            self.pid_max_output = self.pid_output
            self.first_pass = False
        if (self.pid_output > self.pid_max_output):
            self.pid_max_ouput = self.pid_output 
        #print("Calculating Flap length")
        self.flaps_area = (((self.pid_output) / (self.pid_max_output)) * 0.0043432172)
        self.flaps_length = (self.flaps_area / (4*0.05588)) * 39.37 #Flap length in inches
        #print("Converting to angle")
        angle = int(max(3,min(self.convert_Flap_Length_to_Angle(),39)))
        #if config.TELEM and self.flaps_length > 0.0:
            #print("sending data")
            #self.xbee.send_milestone(f"Flaps out:   Flap Length: {self.flaps_length}in, Servo Angle: {angle} degrees, PAYLOAD")
        return angle
        #self.flaps_length = 0.765
        #return 39.63
    def convert_Flap_Length_to_Angle(self):
        #in inches
        #y = -0.0002x^2 + 0.0309x + -0.0877, x is servo rotation, y is drag plate extended distance
        a = -0.0002
        b = 0.0309
        c = -(0.0877 + self.flaps_length)
        #print("calculating angle")
        self.discriminant = b**2 - 4*a*c
        self.discriminant = max(0,self.discriminant)
        
        self.angle1 = round((-b + math.sqrt(self.discriminant)) / (2 * a), 1)
        self.angle2 = round((-b - math.sqrt(self.discriminant)) / (2 * a), 1)
        if 2.9 <= self.angle1 <= 39.63:
            self.new_angle = self.angle1
        else:
            self.new_angle = self.angle2
        return self.new_angle




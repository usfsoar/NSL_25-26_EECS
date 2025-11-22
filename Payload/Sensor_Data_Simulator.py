import matplotlib.pyplot as plt
import numpy as np
import math

#TO DO:  
# [DONE] take drag calculations from last year's rk4 and pid code (see github)
# [DONE] make class called Sensor_Data_Simulator, and there should be callable functions get_velocity()
# data output: 
    # acceleration will be (x,y,z)
    #for height, xyz array (x and y can be constant 0)
    #research datatypes that the sensors output
    # return only acceleration?

#NOTES:
# calculations from last year predicted apogee at 4,444 feet (1,354.53 m).  Calculations from this program (neglecting flap length 
# in the drag calculations) result in apogee at 1407.2577990273785 m


GRAVITY = 9.81 #m s^-2

rocketMass=18.302   #initial rocket mass:  kg (40.35 lbs)
fuelMass=2.013      #initial fuel mass:  kg (extrapolated from landing mass, CDR pg 15)
burnRate=0.5591     #burn rate:  kg s^-1 (extrapolated from fuel mass and burn time)
burnTime=3.6        #burn time:  s (CDR pg 94)

class Sensor_Data_Simulator:

    def getAccel(time, velocity):
        #calculate the acceleration using the thrust, drag, and mass
        #not a function of height because we are ignoring the variation of gravity due to height
        F=Sensor_Data_Simulator.getThrust(time)
        D=Sensor_Data_Simulator.getDrag(velocity)
        #D=0
        M=Sensor_Data_Simulator.getMass(time)

        acceleration=(F-(M*GRAVITY)-D)/M
        return acceleration
    
    def getThrust(time):
        #Assume thrust is constant and defined by the rocket motor parameters.  
        #Thrust will be some constant value for the duration of the burn time (using 100 for now)
        if time <= burnTime:
            thrust=996.5 #N (Avg Thrust of the motor, CDR pg 94)
        else:
            thrust=0
        return thrust
    
    def getDrag(velocity):
        #New drag calculation from last year's rk4.py
        flaps_length=0 #ignoring flap length
        drag = -0.7207 + 0.05836*velocity + 0.2469*flaps_length + 0.00494*(velocity**2) + -0.3308*velocity*flaps_length + 17.65*(flaps_length**2) + -9.656e-07*(velocity**3) + 0.002397*(velocity**2)*flaps_length + 0.373*velocity*(flaps_length**2) + -22.71*(flaps_length**3)
        if drag<0:
            drag=0
        return drag
    
    def getMass(time):
        #Mass changes wrt time due to fuel burn and loss
        if time<=burnTime:
            mass=rocketMass-(fuelMass*burnRate*time) #actual mass is the total inital mass minus the amount of fuel burned
        else:
            mass=rocketMass-fuelMass
        return mass

    def runSim():
        #INITIAL PARAMETERS
        v=0             #initial velocity
        h=0             #initial height
        dt=0.05         #time step increments
        k=0             #increment/step counter
        tRange=np.arange(0, 40, dt) #test start, stop, and step times

        A=[]            #list for acceleration values
        V=[]            #list for velocity values
        H=[]            #list for height values
        T=[]            #list for time values
        
        for t in tRange:        #loop to execute in increments of dt for given runtime 
            a=Sensor_Data_Simulator.getAccel(t, v)    #obtain acceleration
            #record values for acceleration, velocity, height, and time
            A.insert(k, a)
            V.insert(k, v)
            H.insert(k, h)
            T.insert(k, t)

            #update values for height and velocity
            h=h+v*dt
            v=v+a*dt
            k+=1
            if h<0:
                break
        
        # Comment/Uncomment to toggle plot for results
        # Sensor_Data_Simulator.plotSim(H, V, A, T)
    
    def plotSim(height, velocity, acceleration, time):            
        # print(str(max(H))) #to get apogee
        figure, axis=plt.subplots(2,2)

        axis[0,0].plot(time, acceleration)
        axis[0,0].set_title('Acceleration vs time')
        axis[0,0].set_xlabel('time (s)')
        axis[0,0].set_ylabel('Acceleration (m/s^2)')

        axis[0,1].plot(time, velocity)
        axis[0,1].set_title('Velocity vs time')
        axis[0,1].set_xlabel('time (s)')
        axis[0,1].set_ylabel('Velocity (m/s)')

        axis[1,0].plot(time, height)
        axis[1,0].set_title('Height vs time')
        axis[1,0].set_xlabel('time (s)')
        axis[1,0].set_ylabel('Height (m)')
        axis[0,0].grid(True)
        axis[0,1].grid(True)
        axis[1,0].grid(True)

        plt.show()
        print('Apogee: ', max(height))

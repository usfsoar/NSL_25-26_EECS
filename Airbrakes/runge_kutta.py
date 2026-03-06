import numpy as np
import math
# Function to predict the rocket's apogee

def f(t, a, v):
            accel = -9.8
            accel = d / 20 - 9.8 #20 is the assumed mass
            return np.array([v, accel])
def prediction(time, altitude, velocity, acceleration):
    dt = 0.1
    
    while velocity > 0:
        
        
        k1 = f(time, altitude, velocity)
        k2 = f(time + dt/2, altitude + k1[0] * dt/2, velocity + k1[1] * dt/2)
        k3 = f(time + dt/2, altitude + k2[0] * dt/2, velocity + k2[1] * dt/2)
        k4 = f(time + dt, altitude + k3[0] * dt, velocity + k3[1] * dt)

        altitude += (dt/6) * (k1[0] + 2*k2[0] + 2*k3[0] + k4[0])
        velocity += (dt/6) * (k1[1] + 2*k2[1] + 2*k3[1] + k4[1])
        time += dt
    return altitude



if __name__ == '__main__':
    pass
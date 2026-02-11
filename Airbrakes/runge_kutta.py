import numpy as np
import math
# Function to predict the rocket's apogee
def prediction(time, altitude, velocity):
    dt = 0.1
    rho = 1.225 * math.exp(-altitude/8400)
    area = 0.0182
    Cd = 0.5

    d = -0.5 * rho * (velocity*velocity) * area * Cd
    while velocity > 0:
        def f(t, a, v):
            accel = -9.8
            accel = d / 20 - 9.8 #20 is the assumed mass
            return np.array([v, accel])
        
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
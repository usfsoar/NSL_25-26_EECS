import pid
import numpy as np
rho =1.225

def prediction(time, altitude, velocity, dt, flaps_length):
    while(velocity > 0):
        def f(t,h,v):
            #drag equation
            drag = -0.7207 + 0.05836*v + 0.2469*flaps_length + 0.00494*(v**2) + -0.3308*v*flaps_length + 17.65*(flaps_length**2) + -9.656e-07*(v**3) + 0.002397*(v**2)*flaps_length + 0.373*v*(flaps_length**2) + -22.71*(flaps_length**3) 
            if drag < 0:
                drag = 0
            F_gravity = 18.1 * 9.81
            dv_dt = -(drag + F_gravity) / 18.1
            dh_dt = v
            return np.array([dh_dt, dv_dt])
        k1 = f(time, altitude, velocity)
        k2 = f(time + dt / 2, altitude + (k1[0] * dt )/ 2, velocity + k1[1] * dt / 2)
        k3 = f(time + dt / 2, altitude + (k2[0] * dt) / 2, velocity + k2[1] * dt / 2)
        k4 = f(time + dt, altitude + k3[0] * dt, velocity + k3[1] * dt)

        altitude = altitude + (dt / 6) * (k1[0] + (2 * k2[0]) + (2 * k3[0]) + k4[0])
        velocity = velocity + (dt / 6) * (k1[1] + (2 * k2[1]) + (2 * k3[1]) + k4[1])
        time += dt
    return altitude
import numpy as np
from numba import njit

max_iterations = 100000

ROCKET_MASS = 25.09 # mass of rocket in kg (55.32lb)
SLEW_RATE = 3885.59 # steps/s


steps = np.array([0, 1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0, 7000.0, 8000.0, 9000.0, 10000.0, 11000.0])
angles = np.array([0.0, 8.0, 15.0, 20.0, 24.0, 30.0, 33.0, 36.5, 41.0, 44.0, 47.0, 51.5])

'''
alternatively, the 6 degree polynomial function is: 
f = -0.0227 + 9.08E-03x + -9.19E-07x^2 + -8.33E-12x^3 + 2.49E-14x^4 + -3.12E-18x^5 + 1.19E-22x^6 
R^2 = 0.999
x=steps
'''


@njit
def find_interval(axis, value):
    idx = np.searchsorted(axis, value) - 1
    if idx < 0:
        return 0
    if idx >= len(axis) - 1:
        return len(axis) - 2
    return idx

@njit
def interpolate_angle(s):
    i = find_interval(steps,s)

    s0, s1 = steps[i], steps[i+1]
    a0, a1 = angles[i], angles[i+1]

    ds = (s - s0) / (s1 - s0)

    angle = a0 * (1 - ds) + (a1 * ds)

    return angle

@njit
def step_to_angle(step):
    return interpolate_angle(step)

@njit
def alt_to_tempK(altitude):
    tempK = 288.19 - (0.00649 * altitude)
    return tempK

@njit
def velocity_to_mach(velocity, alt):
    tempK = alt_to_tempK(alt)
    g = 1.4
    R = 287.05

    a = np.sqrt(g * R * tempK)
    mach = abs(velocity)/a

    return mach

@njit
def subsonic_drag(velocity, angle, alt):
    mach = velocity_to_mach(velocity, alt)
    f = 12.4021 \
        + 429.7658 * (mach**2) \
        + 9.2073 * (mach**2 * angle) \
        + 0.0925 * (mach**2 * angle**2)
    return f

@njit
def transonic_drag(velocity, angle, alt):
    #This is the same as the subsonic_drag function but preferably we get a unique transonic drag function
    mach = velocity_to_mach(velocity, alt)
    f = 12.4021 \
        + 429.7658 * (mach**2) \
        + 9.2073 * (mach**2 * angle) \
        + 0.0925 * (mach**2 * angle**2)
    return f

@njit
def get_drag(velocity, angle, alt):
        mach = velocity_to_mach(velocity, alt)
        if mach < 0.8:
            return subsonic_drag(velocity, angle, alt)
        elif mach >= 0.8:
            return transonic_drag(velocity, angle, alt)
        
@njit
def get_accel(velocity , angle, alt):

    drag = get_drag(velocity, angle, alt)
    a = -(drag / ROCKET_MASS) - 9.81
    return a

@njit
def update(dt, altitude, velocity, step, target):
    curr_step = step
    iterations = 0

    while velocity > 0 and iterations < max_iterations:
        curr_angle = step_to_angle(curr_step)

        v1 = velocity
        a1 = get_accel(velocity, curr_angle, altitude)

        v2 = velocity + a1 * dt/2
        a2 = get_accel(v2, curr_angle, altitude + v1*dt/2)

        v3 = velocity + a2 * dt/2 
        a3 = get_accel(v3, curr_angle, altitude + v2*dt/2)

        v4 = velocity + a3 * dt 
        a4 = get_accel(v4, curr_angle, altitude + v3*dt)

        altitude += dt/6 * (v1 + 2*v2 + 2*v3 + v4)
        velocity += dt/6 * (a1 + 2*a2 + 2*a3 + a4)

        if curr_step < target:
            curr_step = min(curr_step + dt*SLEW_RATE, target)
        elif curr_step > target:
            curr_step = max(curr_step - dt*SLEW_RATE, target)

        iterations += 1

    return altitude


class RungeKutta4:
    def prediction(self, dt, altitude, velocity, step, target):
        return update(dt, altitude, velocity,step, target)

if __name__ == '__main__':
    print(get_drag(210, 22.5, 1000))
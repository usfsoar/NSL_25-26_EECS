import numpy as np
from numba import njit


ROCKET_MASS = 25.09 # mass of rocket in kg (55.32lb)
SLEW_RATE = 62.5 # steps/s

pressures = np.array([0.81, 0.85, 0.9, 0.95, 1.0, 1.05, 1.1, 1.15, 1.2])
velocities = np.array([
    85.75, 102.9, 120.05, 137.2, 154.35, 171.5, 188.65, 205.8, 
    222.95, 240.1, 257.25, 274.4, 291.55, 308.7, 325.85, 343.0
])
angles = np.array([
    0.0, 3.0, 6.0, 9.0, 12.0, 15.0, 18.0, 21.0, 
    24.0, 27.0, 30.0, 33.0, 36.0, 39.0, 42.0, 45.0
])

drag_table = np.load('drag_forces.npy')

@njit
def find_interval(axis, value):
    idx = np.searchsorted(axis, value)
    if idx < 0:
        return 0
    if idx >= len(axis) - 1:
        return len(axis) - 2
    return idx

@njit
def interpolate_drag(p,v,f):
    i = find_interval(pressures,p)
    j = find_interval(velocities, v)
    k = find_interval(angles, f)

    p0, p1 = pressures[i], pressures[i+1]
    v0, v1 = velocities[j], velocities[j+1]
    f0, f1 = angles[k], angles[k+1]

    dp = (p - p0) / (p1 - p0)
    dv = (v - v0) / (v1 - v0)
    df = (f - f0) / (f1 - f0)

    c00 = drag_table[i,j,k] * (1 - dp) + drag_table[i+1,j,k] * dp
    c01 = drag_table[i,j,k+1] * (1 - dp) + drag_table[i+1,j,k+1] * dp
    c10 = drag_table[i,j+1,k] * (1 - dp) + drag_table[i+1,j+1,k] * dp
    c11 = drag_table[i,j+1,k+1] * (1 - dp) + drag_table[i+1,j+1,k+1] * dp

    c0 = c00 * (1-dv) + c10 * dv
    c1 = c01 * (1-dv) + c11 * dv

    drag_force = c0 * (1-df) + c1 * df
    return drag_force

@njit
def step_to_angle(step):
    pass


@njit
def get_drag(altitude, velocity, angle, air_density):
        return interpolate_drag(air_density, velocity, angle)
@njit
def get_accel(a, v, angle, air_density):
    drag = get_drag(a,v,angle, air_density)
    a = -(drag / ROCKET_MASS) - 9.81
    return a

@njit
def update(time,dt, altitude, velocity, step, target, air_density):
    curr_step = step
    while velocity > 0:
        curr_angle = step_to_angle(curr_step)
        v1, a1 = velocity, get_accel(altitude, velocity, curr_angle, air_density)
        v2, a2 = velocity + a1 * dt/2, get_accel(altitude + v1*dt/2, v1, curr_angle, air_density)
        v3, a3 = velocity + a2 * dt/2, get_accel(altitude + v2*dt/2, v2, curr_angle, air_density)
        v4, a4 = velocity + a3 * dt, get_accel(altitude + v3*dt, v3, curr_angle, air_density)

        altitude += dt/6 * (v1 + 2*v2 + 2*v3 + v4)
        velocity += dt/6 * (a1 + 2*a2 + 2*a3 + a4)
        if curr_step < target:
            curr_step = min(curr_step + dt*SLEW_RATE, target)
        elif curr_step > target:
            curr_step = max(curr_step - dt*SLEW_RATE, target)
        time += dt

    return altitude



class RungeKutta4:
    def __init__(self):
        self.dt = 0.1

    def prediction(self, time, altitude, velocity, step, target, air_density):
        return update(time, self.dt, altitude, velocity,step,target, air_density)

if __name__ == '__main__':
    print(interpolate_drag(0.975, 210, 22.5))
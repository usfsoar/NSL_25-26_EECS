import numpy as np
from numba import njit


ROCKET_MASS = 25.09 # mass of rocket in kg (55.32lb)

pressures = np.array([1,2,3])
velocities = np.array([2,4,6])
angles = np.array([0,15,30,45])

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

class RungeKutta4:
    def __init__(self):
        self.dt = 0.1
        self.launchP = None
        self.launchT = None
        self.lapseRateFactor = 0.0065 / self.launchT

    def get_drag(self, altitude, velocity, angle):
        curr_P = self.launchP * (1 - self.lapseRateFactor * altitude)**5.255
        return interpolate_drag(curr_P, velocity, angle)
    
    def get_accel(self,a, v, angle):
        drag = self.get_drag(a,v,angle)
        a = -(drag / ROCKET_MASS) - 9.81
        return a



    @njit
    def step(self, time, altitude, velocity, angle):
        vel = velocity
        dt = self.dt
        while velocity > 0:
            v1, a1 = velocity, self.get_accel(altitude, velocity, angle)
            v2, a2 = velocity + a1 * dt/2, self.get_accel(altitude + v1*dt/2, v1, angle)
            v3, a3 = velocity + a2 * dt/2, self.get_accel(altitude + v2*dt/2, v2, angle)
            v4, a4 = velocity + a3 * dt, self.get_accel(altitude + v3*dt, v3, angle)

            altitude += dt/6 * (v1 + 2*v2 + 2*v3 + v4)
            velocity += dt/6 * (a1 + 2*a2 + 2*a3 + a4)
            time += dt

        return altitude
    
if __name__ == '__main__':
    pass
class RungeKutta4:
    def __init__(self):
        self.dt = 0.1

    def step(self, time, altitude, velocity, acceleration):
        vel = velocity
        dt = self.dt
        while velocity > 0:
            v1, a1 = velocity, acceleration
            v2, a2 = velocity + a1 * dt/2, acceleration
            v3, a3 = velocity + a2 * dt/2, acceleration
            v4, a4 = velocity + a3 * dt, acceleration

            altitude += dt/6 * (v1 + 2*v2 + 2*v3 + v4)
            velocity += dt/6 * (a1 + 2*a2 + 2*a3 + a4)
            time += dt

        return altitude
    
if __name__ == '__main__':
    pass
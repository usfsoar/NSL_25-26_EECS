'''
D = 1/2 * rho * v^2 * Cd * pi * r^2
rho = P / (R * T)
a = -(g + D/m)

TODO:
-update R and T using altitude
-see if cd equation is what we will use
'''
class RungeKutta4_p2:
    def __init__(self):
        self.dt = 0.01
        self.air_gas_constant = 287.05 # specific gas constant of dry ari J/(kg*K)
        self.g = 9.81
        self.A = .1524**2 * 3.14 #cross sectional area of the airbrakes in meters
        self.mass = 4.454 # mass of the rocket in kg

    def get_rho(self, pressure, temperature):
        #need to model future pressure and temperature to get accurate density
        return pressure / (self.air_gas_constant * temperature)
    
    def get_Cd(self):
        #get cd equation
        return 1
    
    def get_drag(self, v, Cd, rho):
        return 0.5 * rho * v**2 * Cd * self.A

    def get_acceleration(self, v, pressure, temperature):
        rho = self.get_rho(pressure, temperature)
        Cd = self.get_Cd()
        D = self.get_drag(v, Cd, rho)
        return -(self.g + D/self.mass)
    
    def f(self, velocity, pressure, temperature):
        #need to model future pressure and temperature to get accurate acceleration
        return self.get_acceleration(velocity, pressure, temperature)

    def prediction(self, time, altitude, velocity, acceleration, pressure, temperature):
        dt = self.dt
        while velocity > 0:
            v1 = velocity
            a1 = self.f(v1, pressure, temperature)

            v2 = velocity + (a1 * dt/2)
            a2 = self.f(v2, pressure, temperature)

            v3 = velocity + (a2 * dt/2)
            a3 = self.f(v3, pressure, temperature)

            v4 = velocity + (a3 * dt)
            a4 = self.f(v4, pressure, temperature)

            altitude += dt/6 * (v1 + 2*v2 + 2*v3 + v4)
            velocity += dt/6 * (a1 + 2*a2 + 2*a3 + a4)
            time += dt

            #i would technically be updating pressure and temperature here using altitude, but i need to check
        return altitude


class RungeKutta4:
    def __init__(self):
        self.dt = 0.1

    def prediction(self, time, altitude, velocity, acceleration):
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
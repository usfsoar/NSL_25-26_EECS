class StateMachine:
    def __init__(self, launch_gforce_threshold, launch_altitude_threshold, descent_apogee_threshold, descent_altitude_threshold, landing_vel_threshold, landing_gforce_threshold, landing_altitude_threshold):
        self.launch_gforce_threshold = launch_gforce_threshold
        self.launch_altitude_threshold = launch_altitude_threshold
        self.descent_apogee_threshold = descent_apogee_threshold
        self.descent_altitude_threshold = descent_altitude_threshold
        self.landing_vel_threshold = landing_vel_threshold
        self.landing_gforce_threshold = landing_gforce_threshold
        self.landing_altitude_threshold = landing_altitude_threshold
        
        
    
    def update(self, current_state, g_force, altitude, velocity, apogee):
        self.current_state = current_state
        self.g_force = g_force
        self.altitude = altitude
        self.velocity = velocity
        self.apogee = apogee

        self.get_state()
        return self.current_state

    def get_state(self):
        match self.current_state:
            case 0:
                if (self.g_force > self.launch_gforce_threshold) or (self.altitude > self.launch_altitude_threshold):
                    self.current_state = 1
                else:
                    self.current_state = 0
            case 1:
                if (self.apogee > self.descent_apogee_threshold) and (self.altitude < (self.apogee - self.descent_altitude_threshold)):
                    self.current_state = 2
                else:
                    self.current_state = 1
            case 2:
                if (self.velocity < self.landing_vel_threshold) and (abs(self.g_force - 1.0) < self.landing_gforce_threshold) and (self.altitude < self.landing_altitude_threshold):
                    self.current_state = 3
                else:
                    self.current_state = 2
            case 3:
                self.current_state = 3
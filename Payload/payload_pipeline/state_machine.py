class StateMachine:
    def __init__(self, launch_gforce_threshold, launch_altitude_threshold, descent_apogee_threshold, descent_altitude_threshold, landing_vel_threshold, landing_gforce_threshold, landing_altitude_threshold):
        self.launch_gforce_threshold = launch_gforce_threshold
        self.launch_altitude_threshold = launch_altitude_threshold
        self.descent_apogee_threshold = descent_apogee_threshold
        self.descent_altitude_threshold = descent_altitude_threshold
        self.landing_vel_threshold = landing_vel_threshold
        self.landing_gforce_threshold = landing_gforce_threshold
        self.landing_altitude_threshold = landing_altitude_threshold

        self.counters = 0
        self.thresholds = {"launch": 3, "descent": 3, "landing": 10}
        
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
            case "READY":
                if (self.g_force > self.launch_gforce_threshold) or (self.altitude > self.launch_altitude_threshold):
                    self.counters += 1
                else:
                    self.counters = 0

                if self.counters >= self.thresholds["launch"]:
                    self.current_state = "LAUNCHED"
                    self.counters = 0
                
            case "LAUNCHED":
                if (self.apogee > self.descent_apogee_threshold) and (self.altitude < (self.apogee - self.descent_altitude_threshold)):
                    self.counters += 1
                else:
                    self.counters = 0
                
                if self.counters >= self.thresholds["descent"]:
                    self.current_state = "DESCENT"
                    self.counters = 0

            case "DESCENT":
                if (abs(self.velocity) < self.landing_vel_threshold) and (self.g_force - 1.0 < self.landing_gforce_threshold) and (self.altitude < self.landing_altitude_threshold):
                    self.counters += 1
                else:
                    self.counters = 0

                if self.counters >= self.thresholds["landing"]:
                    self.current_state = "LANDING"
                    self.counters = 0

            case "LANDING":
                self.current_state = "LANDING"
                
        # match self.current_state:
        #     case "READY":
        #         if (self.g_force > self.launch_gforce_threshold) or (self.altitude > self.launch_altitude_threshold):
        #             self.current_state = "LAUNCHED"
        #         else:
        #             self.current_state = "READY"
        #     case "LAUNCHED":
        #         if (self.apogee > self.descent_apogee_threshold) and (self.altitude < (self.apogee - self.descent_altitude_threshold)):
        #             self.current_state = "DESCENT"
        #         else:
        #             self.current_state = "LAUNCHED"
        #     case "DESCENT":
        #         if (abs(self.velocity) < self.landing_vel_threshold) and (self.g_force - 1.0 < self.landing_gforce_threshold) and (self.altitude < self.landing_altitude_threshold):
        #             self.current_state = "LANDING"
        #         else:
        #             self.current_state = "DESCENT"
        #     case "LANDING":
        #         self.current_state = "LANDING"
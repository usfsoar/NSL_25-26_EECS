import time
from datetime import datetime

class StateMachine:
    def __init__(self, launch_gforce_threshold, launch_altitude_threshold, 
                 descent_apogee_threshold, descent_altitude_threshold, landing_vel_threshold, 
                 landing_gforce_threshold, landing_altitude_threshold,
                 stable_readings, stable_readings_for_landing, timeout):
        
        self.launch_gforce_threshold = launch_gforce_threshold
        self.launch_altitude_threshold = launch_altitude_threshold
        self.descent_apogee_threshold = descent_apogee_threshold
        self.descent_altitude_threshold = descent_altitude_threshold
        self.landing_vel_threshold = landing_vel_threshold
        self.landing_gforce_threshold = landing_gforce_threshold
        self.landing_altitude_threshold = landing_altitude_threshold
        self.stable_readings = stable_readings
        self.stable_readings_for_landing = stable_readings_for_landing
        self.timeout = timeout

        self.ready_counters = 0
        self.launched_counters = 0
        self.descent_counters = 0
        self.landing_counters = 0

        self.start_time = 0
        self.current_time = 0

    def recover(self, start_time):
        t = datetime.strptime(start_time, "%H:%M:%S.%f").time()
        self.start_time = t.hour * 3600 + t.minute * 60 + t.second + t.microsecond / 1e6
        
    def update(self, cur_time, current_state, altitude, velocity, apogee):
        t = datetime.strptime(cur_time, "%H:%M:%S.%f").time()
        self.time = t.hour * 3600 + t.minute * 60 + t.second + t.microsecond / 1e6
        self.current_state = current_state
        self.altitude = altitude
        self.velocity = velocity
        self.apogee = apogee

        self.get_state()
        return self.current_state

    def get_state(self):
        match self.current_state:
            case "READY":
                if (self.altitude > self.launch_altitude_threshold):
                    self.ready_counters += 1
                else:
                    self.ready_counters = 0

                if self.ready_counters >= self.stable_readings:
                    self.current_state = "LAUNCHED"
                    self.start_time = self.time
                    self.ready_counters = 0
                
            case "LAUNCHED":
                if (self.apogee > self.descent_apogee_threshold) and (self.altitude < (self.apogee - self.descent_altitude_threshold)):
                    self.launched_counters += 1
                else:
                    self.launched_counters = 0
                
                if self.launched_counters >= self.stable_readings:
                    self.current_state = "DESCENT"
                    self.launched_counters = 0

            case "DESCENT":
                if (abs(self.velocity) < self.landing_vel_threshold) and (self.altitude < self.landing_altitude_threshold):
                    self.landing_counters += 1
                else:
                    self.landing_counters = 0

                if self.landing_counters >= self.stable_readings_for_landing:
                    self.current_state = "LANDING"
                    self.landing_counters = 0

            case "LANDING":
                self.current_state = "LANDING"
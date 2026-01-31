class StateMachine:
    def __init__(self):
        self.launch_accel_threshold = 0
        self.launch_altitude_threshold = 0
        self.apogee

        
    
    def update(self, current_state, g_force, altitude, velocity):
        self.current_state = current_state
        self.g_force = g_force
        self.altitude = altitude
        self.velocity = velocity
        
    def get_state(self):
        match self.current_state:
            case 0:
                pass
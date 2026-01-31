class StateMachine:
    def __init__(self, state, g_force, altitude, velocity):
        self.current_state = state
        self.g_force = g_force
        self.altitude = altitude
        self.velocity = velocity
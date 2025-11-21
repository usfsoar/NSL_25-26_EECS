# Class containing the state machine
class StateMachine:
    def __init__(self):
        self.current_state = 0
        self.state1_counter = 0
        self.state1_threshold = 5
        self.state2_counter = 0
        self.state2_threshold = 5
        self.state3_counter = 0
        self.state3_threshold = 5
        self.state4_counter = 0
        self.state4_threshold = 5
        self.state5_counter = 0
        self.state5_threshold = 5
        self.state1_time = 0
        self.state1_timeout = 6.5
        self.state1_started = False
        self.elapsed_time = time.time()
        self.target_height = 1219
        self.state2_started = False
        self.state2_time = 0
        self.state2_timeout = 30
        self.state3_started = False
        


    def transition(self, altitude, velocity, acceleration, apogee):
        match self.current_state:
            case 0:
                if altitude > 10 or acceleration > 20:
                    self.state1_counter += 1
                    if self.state1_counter >= self.state1_threshold:
                        self.current_state = 1  
            case 1:
                if self.state1_started == False:
                    self.state1_started = True
                    self.state1_time = time.time()
                else: 
                    if self.elapsed_time - self.state1_time > self.state1_timeout:
                        self.current_state = 2
            case 2:
                if not self.state2_started:
                    self.state2_started = True
                    self.state2_time = time.time()
                else:
                    if self.elapsed_time - self.state2_time > self.state2_timeout:
                        self.state3_counter += 1
                        if self.state3_counter >= self.state3_threshold:
                            self.current_state = 3
                    elif velocity < 0:
                        self.state4_counter += 1
                        if self.state4_counter >= self.state4_threshold:
                            self.current_state = 4
            case 3:
                if not self.state3_started:
                    self.state3_started = True
                else:
                    if velocity < 0:
                        self.state4_counter += 1
                        if self.state4_counter >= self.state4_threshold:
                            self.current_state = 4
                    elif altitude < apogee:
                        self.state4_counter += 1
                        if self.state4_counter >= self.state4_threshold:
                            self.current_state = 4
            case 4:
                if not self.state4_started:
                    self.state4_started = True
                else:
                    if  -10 < velocity > 10:
                        self.state5_counter += 1
                        if self.state5_counter >= self.state5_threshold:
                            self.current_state = 5
            case 5:
                pass
            case _:
                print("oh no")

        return self.current_state

if __name__ == '__main__':
    pass
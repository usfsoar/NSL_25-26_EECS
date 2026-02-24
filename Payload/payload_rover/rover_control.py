import time

class RoverControl:
    def __init__(self, motors, tof, timeout, exit_timeout):
        self.motors = motors
        self.tof = tof
        self.timeout = timeout
        self.exit_timeout = exit_timeout

        self.dist = 5
        self.detect_dist = 20



    def exit_rover(self):
        #if object is detected in front of rover at start, reverse and turn 180
        self.start_time = time.time()
        while (self.detect_objects() and time.time() != self.exit_timeout):
            self.motors.reverse(300)
        
        self.motors.turn_180()
        

    def do_scan(self):
        while time.time() - self.start_time < self.timeout:
        #multithread grid pattern, and if detect objects condition. need to move and detect at the same time
            self.grid_pattern(self.calc_dist())
            self.run_detection()

    def run_detection(self):
            if (self.detect_objects()):
                if (self.classify_plant()):
                    self.get_ndvi()
                    self.send_data()
                    self.save_frame()
                self.go_around()
            self.save_frame()
            self.send_data()        

    def grid_pattern(self, distance):
        self.motors.go_straight(distance)
        self.motors.turn_right()


    def calc_dist(self):


        self.dist += 5
        return self.dist


    def detect_objects(self):
        #use tof to detect objects in front of rover
        if self.tof.get_distance() < self.detect_dist:
            return True
        else:
            return False

    def go_around(self):
        #turn right, go straight, turn left, check if object is still there
        while (self.detect_objects()):
            self.motors.turn_right()
            self.motors.go_straight(20)
            self.motors.turn_left()

    def classify_plant(self):
        # return self.ir_camera.plant_type
        pass

    def get_ndvi(self):
        #return self.ir_camera.get_ndvi()
        pass

    def send_data(self):
        pass

    def save_frame(self):
        pass
import time

class RoverControl:
    def __init__(self, motors, timeout):
        self.motors = motors
        self.timeout = timeout

        self.cubesat_length = 300

    def exit_rover(self):
        self.start_time = time.time()
        if (self.detect_objects()):
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
        self.motors.go_straigth(distance)
        self.motors.turn_right()


    def calc_dist(self):
        #use time, and increase over time
        #use velocity and time to calculate distance
        pass

    def detect_objects(self):
        pass

    def go_around(self):
        pass

    def classify_plant(self):
        pass

    def get_ndvi(self):
        pass

    def send_data(self):
        pass

    def save_frame(self):
        pass
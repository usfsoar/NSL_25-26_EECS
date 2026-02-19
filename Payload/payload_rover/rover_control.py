import time

class RoverControl:
    def __init__(self, motors, timeout):
        self.motors = motors
        self.timeout = timeout

    def exit_rover(self):
        self.start_time = time.time()
        

    def do_scan(self):
        while time.time() - self.start_time < self.timeout:
            #multithread grid pattern and detection
            self.grid_pattern(self.calc_dist())
            
            if (self.detect_objects()):
                if (self.classify_plant()):
                    self.get_ndvi()
                    self.send_data()
                    self.save_frame()
                self.go_around()
            self.save_frame()
            self.send_data()


    def grid_pattern(self, distance):
        pass

    def calc_dist(self):
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
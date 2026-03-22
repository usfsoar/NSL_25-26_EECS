import time

class MotorControl:
    def __init__(self, pins, wheel_circum, bno, bmp):
        self.pins = pins
        self.wheel_circum = wheel_circum
        self.bno = bno
        self.bmp = bmp

    def set_pwm(self, r_pwm, l_pwm):
        pass
    
    #use p control for turning
    def turn_right(self):
        #while self.bno.yaw != start + 90:
        self.set_pwm(-127, 127)

    def turn_left(self):
        #while self.bno.yaw != start - 90:
        self.set_pwm(127, -127)

    # def go_straight(self, distance):
    #     #define distance
    #     #use time, and increase over time
    #     #use velocity and time to calculate distance
    #     #pid control???
    #     #use traveled = bno.get_velocity * (time.time() - move_time)
    #     traveled = 0
    #     kp = 0.1
    #     move_time = time.time()
    #     while (traveled < distance):
    #         traveled += self.bno.get_velocity() * (time.time() - move_time)
    #         error = distance - traveled
    #         pwm = max(0, min(int(error * kp), 127))
    #         self.set_pwm(pwm, pwm)

    def reverse(self, distance):
        traveled = 0
        while (traveled < distance):
            self.set_pwm(-127, -127)

    def turn_180(self):
        #while self.bno.yaw != start + 180:
        self.set_pwm(-127, 127)
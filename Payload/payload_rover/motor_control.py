class MotorControl:
    def __init__(self, pins, wheel_circum, bno, bmp):
        self.pins = pins
        self.wheel_circum = wheel_circum
        self.bno = bno
        self.bmp = bmp

    def set_pwm(self, r_pwm, l_pwm):
        pass

    def turn_right(self):
        #while self.bno.yaw != start + 90:
        self.set_pwm(-127, 127)

    def turn_left(self):
        #while self.bno.yaw != start - 90:
        self.set_pwm(127, -127)

    def go_straight(self, distance):
        #define distance
        #use time, and increase over time
        #use velocity and time to calculate distance
        #pid control???
        traveled = 0
        acceleration = 1
        dt = 0.1
        velocity = 0
        while (traveled < distance):
            velocity += acceleration * dt
            traveled += velocity * dt
            self.set_pwm(127, 127)

    def reverse(self, distance):
        traveled = 0
        acceleration = 1
        dt = 0.1
        velocity = 0
        while (traveled < distance):
            velocity += acceleration * dt
            traveled += velocity * dt
            self.set_pwm(-127, -127)

    def turn_180(self):
        #while self.bno.yaw != start + 180:
        self.set_pwm(-127, 127)
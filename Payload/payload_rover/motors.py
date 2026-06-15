import asyncio
from gpiozero import DigitalOutputDevice
from time import sleep

class Motor:
    # TODO** Convert to pwm
    # ex, Motor(wheel_diameter=0.1 (meters), pwm_pin=0 (0 or 1), direction_pin=36, output_pin=38)
    def __init__(self, wheel_diameter, direction_pin, pwm_pin = None):
        self.wheel_diameter = wheel_diameter                        # in meters
        self.position = 0                                           # axel position
        self.pwmRight = None
        if pwm_pin is not None:
            self.pwmRight = DigitalOutputDevice(pwm_pin)          # 0 or 1 at 25 kHz
            self.pwmRight.on()
        self.direction_pin = DigitalOutputDevice(direction_pin)     # GPIO output for direction control

    # 1 for on 0 for off
    def set_speed(self, speed):
        if self.pwmRight is not None:
            if (speed == 1):
                self.pwmRight.off()
            elif (speed == 0):
                self.pwmRight.on()

    # True for forward, false for backward
    def set_direction(self, forward=True):
        if forward:
            self.direction_pin.on()  # Set direction to forward
        else:
            self.direction_pin.off()  # Set direction to backward



class DriveController:
    def __init__(self, back_left: Motor, back_right: Motor, front_left: Motor, front_right: Motor):
        self.back_left_motor = back_left
        self.back_right_motor = back_right
        self.front_left_motor = front_left
        self.front_right_motor = front_right

    def stop(self):
        self.front_left_motor.set_speed(0)
        self.front_right_motor.set_speed(0)

    def set_speed(self, left, right):
        self.front_left_motor.set_speed(left)
        self.front_right_motor.set_speed(right)

    def move_forward(self, speed=1):
        self.back_left_motor.set_direction(True)
        self.back_right_motor.set_direction(False)
        self.front_left_motor.set_direction(True)
        self.front_right_motor.set_direction(False)
        self.set_speed(left=speed, right=speed)

    def move_backward(self, speed=1):
        self.back_left_motor.set_direction(False)
        self.back_right_motor.set_direction(True)
        self.front_left_motor.set_direction(False)
        self.front_right_motor.set_direction(True)
        self.set_speed(left=speed, right=speed)

    def spin_left(self, speed=1):
        self.back_left_motor.set_direction(False)
        self.back_right_motor.set_direction(False)
        self.front_left_motor.set_direction(False)
        self.front_right_motor.set_direction(False)
        self.set_speed(left=speed, right=speed)

    def spin_right(self, speed=1):
        self.back_left_motor.set_direction(True)
        self.back_right_motor.set_direction(True)
        self.front_left_motor.set_direction(True)
        self.front_right_motor.set_direction(True)
        self.set_speed(left=speed, right=speed)
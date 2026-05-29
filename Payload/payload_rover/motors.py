import asyncio
from rpi_hardware_pwm import HardwarePWM
from gpiozero import DigitalOutputDevice
from time import sleep

class Motor:
    # ex, Motor(wheel_diameter=0.1 (meters), pwm_channel=0 (0 or 1), direction_pin=36, output_pin=38)
    def __init__(self, wheel_diameter, pwm_channel, direction_pin, output_pin):
        self.wheel_diameter = wheel_diameter                        # in meters
        self.position = 0                                           # axel position  
        self.pwmRight = HardwarePWM(pwm_channel, hz=25000)          # 0 or 1 at 25 kHz
        self.pwmRight.start(100)                                    # Start with 100% duty cycle (stopped)
        self.direction_pin = DigitalOutputDevice(direction_pin)     # GPIO output for direction control
        self.output_pin = output_pin                                # gpio for HAL PWM output for rotation detection

    # Set speed as a percentage (0-100)
    def set_speed(self, speed):
        duty_cycle = max(0, min(100, speed))  # clamp to 0-100 
        real_duty_cycle = 100 - duty_cycle
        self.pwmRight.change_duty_cycle(real_duty_cycle)

    # True for clockwise, false for counterclockwise
    def set_direction(self, clockwise=True):
        if clockwise:
            self.direction_pin.on()  # Set direction to clockwise
        else:
            self.direction_pin.off()  # Set direction to counterclockwise

    #move clockwise for time seconds
    async def move_clockwise_time(self, time, speed=100):
        self.direction_pin.on()  # Set direction to clockwise
        self.set_speed(speed)  # Set speed
        await asyncio.sleep(time)  # Move for the specified time
        self.set_speed(0)  # Stop the motor

    #move counterclockwise for time seconds
    async def move_counterclockwise_time(self, time, speed=100):
        self.direction_pin.off()  # Set direction to counterclockwise
        self.set_speed(speed)  # Set speed
        await asyncio.sleep(time)  # Move for the specified time
        self.set_speed(0)  # Stop the motor



class DriveController:
    def __init__(self, back_left: Motor, back_right: Motor, front_left: Motor, front_right: Motor):
        self.back_left_motor = back_left
        self.back_right_motor = back_right
        self.front_left_motor = front_left
        self.front_right_motor = front_right

    def stop(self):
        self.back_left_motor.set_speed(0)
        self.back_right_motor.set_speed(0)
        self.front_left_motor.set_speed(0)
        self.front_right_motor.set_speed(0)

    def set_speed(self, speed):
        self.back_left_motor.set_speed(speed)
        self.back_right_motor.set_speed(speed)
        self.front_left_motor.set_speed(speed)
        self.front_right_motor.set_speed(speed)

    def move_forward(self, speed=100):
        self.back_left_motor.set_direction(True)
        self.back_right_motor.set_direction(False)
        self.front_left_motor.set_direction(True)
        self.front_right_motor.set_direction(False)
        self.set_speed(speed)

    def move_backward(self, speed=100):
        self.back_left_motor.set_direction(False)
        self.back_right_motor.set_direction(True)
        self.front_left_motor.set_direction(False)
        self.front_right_motor.set_direction(True)
        self.set_speed(speed)

    def spin_left(self, speed=100):
        self.back_left_motor.set_direction(False)
        self.back_right_motor.set_direction(False)
        self.front_left_motor.set_direction(False)
        self.front_right_motor.set_direction(False)
        self.set_speed(speed)

    def spin_right(self, speed=100):
        self.back_left_motor.set_direction(True)
        self.back_right_motor.set_direction(True)
        self.front_left_motor.set_direction(True)
        self.front_right_motor.set_direction(True)
        self.set_speed(speed)
    
    async def move_forward_timed(self, time, speed=100):
        await asyncio.gather(
            self.back_left_motor.move_clockwise_time(time, speed),
            self.back_right_motor.move_counterclockwise_time(time, speed),
            self.front_left_motor.move_clockwise_time(time, speed),
            self.front_right_motor.move_counterclockwise_time(time, speed)
        )

    async def move_backward_timed(self, time, speed=100):
        await asyncio.gather(
            self.back_left_motor.move_counterclockwise_time(time, speed),
            self.back_right_motor.move_clockwise_time(time, speed),
            self.front_left_motor.move_counterclockwise_time(time, speed),
            self.front_right_motor.move_clockwise_time(time, speed)
        )

    async def spin_left_timed(self, time, speed=100):
        await asyncio.gather(
            self.back_left_motor.move_counterclockwise_time(time, speed),
            self.back_right_motor.move_counterclockwise_time(time, speed),
            self.front_left_motor.move_counterclockwise_time(time, speed),
            self.front_right_motor.move_counterclockwise_time(time, speed)
        )

    async def spin_right_timed(self, time, speed=100):
        await asyncio.gather(
            self.back_left_motor.move_clockwise_time(time, speed),
            self.back_right_motor.move_clockwise_time(time, speed),
            self.front_left_motor.move_clockwise_time(time, speed),
            self.front_right_motor.move_clockwise_time(time, speed)
        )

    # async def move_forward(self, distance):
    #     await asyncio.gather(
    #         self.back_left_motor.move_clockwise_time(distance),
    #         self.back_right_motor.move_counterclockwise_time(distance)
    #     )

    # async def move_backward(self, distance):
    #     await asyncio.gather(
    #         self.back_left_motor.move_counterclockwise_time(distance),
    #         self.back_right_motor.move_clockwise_time(distance)
    #     )

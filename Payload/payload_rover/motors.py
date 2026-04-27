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

    # True for forward, false for backward
    def set_direction(self, forward=True):
        if forward:
            self.direction_pin.on()  # Set direction to forward
        else:
            self.direction_pin.off()  # Set direction to backward

    #move forward for time seconds
    async def move_forward_time(self, time):
        self.direction_pin.on()  # Set direction to forward
        self.set_speed(100)  # Full speed forward
        await asyncio.sleep(time)  # Move for the specified time
        self.set_speed(0)  # Stop the motor

    #move backward for time seconds
    async def move_backward_time(self, time):
        self.direction_pin.off()  # Set direction to backward
        self.set_speed(100)  # Full speed backward
        await asyncio.sleep(time)  # Move for the specified time
        self.set_speed(0)  # Stop the motor



# class DriveController:
#     def __init__(self, left_motor, right_motor):
#         self.left_motor = left_motor
#         self.right_motor = right_motor

#     async def move_forward(self, distance):
#         await asyncio.gather(
#             self.left_motor.move_forward(distance),
#             self.right_motor.move_forward(distance)
#         )

#     async def move_backward(self, distance):
#         await asyncio.gather(
#             self.left_motor.move_backward(distance),
#             self.right_motor.move_backward(distance)
#         )

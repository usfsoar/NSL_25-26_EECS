# from gpiozero import AngularServo
# from time import sleep

# servo = AngularServo(12, min_angle=-180, max_angle=180)

# while True:
#     servo.angle = -180
#     sleep(2)
#     servo.angle = -90
#     sleep(2)
#     servo.angle = 0
#     sleep(2)
#     servo.angle = 90
#     sleep(2)
#     servo.angle = 180
#     sleep(2)


from gpiozero import Servo
from time import sleep

servo = Servo(18, min_pulse_width=0.0005, max_pulse_width=0.0025)

while True:
    print("min")
    servo.value = -1
    sleep(2)
    print("min")
    servo.value = 0
    sleep(2)
    print("min")
    servo.value = 1
    sleep(2)


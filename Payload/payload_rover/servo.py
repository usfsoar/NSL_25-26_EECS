from gpiozero import AngularServo
from time import sleep

servo = AngularServo(14, min_angle=-180, max_angle=180)

while True:
    servo.angle = -180
    sleep(2)
    servo.angle = -90
    sleep(2)
    servo.angle = 0
    sleep(2)
    servo.angle = 90
    sleep(2)
    servo.angle = 180
    sleep(2)


# from gpiozero import Servo
# from time import sleep

# servo = Servo(14)

# while True:
#     servo.min()
#     sleep(2)
#     servo.mid()
#     sleep(2)
#     servo.max()
#     sleep(2)
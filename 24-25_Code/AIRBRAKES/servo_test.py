import servo_control
import time

servo = servo_control.SERVO(16, 3, 39)
servo.intialize(12)


for i in range (10):
    time.sleep(1)
    if i%2 == 0:
        servo.set_angle(90)
    else:
        servo.set_angle(0)

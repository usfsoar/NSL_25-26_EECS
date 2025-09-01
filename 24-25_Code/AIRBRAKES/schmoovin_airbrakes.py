import servo_control
import time
import random
servo = servo_control.SERVO(16,13,39)
servo.initialize(12)
while True:
    servo.set_angle(random.randint(13, 39))
    time.sleep(0.5)
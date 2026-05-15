from gpiozero import Servo
import time

class ServoControl:
    def __init__(self):
        pass

    def initialize(self, pin: int = 18, min_angle: int = -130, max_angle: int = 130):
        self.min = min_angle/135
        self.max = max_angle/135
        self.servo = Servo(pin, min_pulse_width=0.0005, max_pulse_width=0.0025)

    def retract(self):
        self.servo.value = self.max
    
    def lock(self):
        self.servo.value = self.min

if __name__ == '__main__':
    servo = ServoControl()

    servo.initialize(18, 0, 100)
    time.sleep(2)
    print("lock")
    servo.lock()
    time.sleep(5)
    print("retract")
    servo.retract()
    time.sleep(5)

    print("lock loop")
    while True:
        servo.lock()
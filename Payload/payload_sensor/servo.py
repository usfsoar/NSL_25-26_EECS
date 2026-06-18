from gpiozero import Servo
import time

class ServoControl:
    def __init__(self):
        pass

    def initialize(self, pin: int = 16, min_angle: int = -45, max_angle: int = 45):
        self.min = min_angle/180
        self.max = max_angle/180

        for i in range(10):
            try:
                self.servo = Servo(pin, min_pulse_width=0.0005, max_pulse_width=0.0025)
                break
            except Exception as e:
                print(f"Error initializing Servo: {e}")

    def retract(self):
        for i in range(10):
            try:
                self.servo.value = self.max
                break
            except Exception as e:
                print(f"Error retracting the Servo: {e}")
    
    def lock(self):
        for i in range(10):
            try:
                self.servo.value = self.min
                break
            except Exception as e:
                print(f"Error locking the Servo: {e}")

    def test(self, val=.75):
        for i in range(10):
            try:
                self.servo.value = val
                break
            except Exception as e:
                print(f"Error testing the Servo: {e}")

if __name__ == '__main__':
    servo = ServoControl()

    servo.initialize(16, -45, 45)

    while True:
        time.sleep(1)
        servo.test(1)
        time.sleep(1)
        servo.test(-1)

    # time.sleep(2)
    # print("lock")
    # servo.lock()
    # time.sleep(5)
    # print("retract")
    # servo.retract()
    # time.sleep(5)

    # print("lock loop")
    # while True:
    #     servo.lock()

    # time.sleep(3)
    # servo.test(.99)
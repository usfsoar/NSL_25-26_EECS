from gpiozero import Servo
import time

class ServoControl:
    def __init__(self):
        pass

    def initialize(self, pin: int = 16, min_angle: int = -20, max_angle: int = 20):
        self.min = min_angle/180
        self.max = max_angle/180
        self.servo = Servo(pin, min_pulse_width=0.0005, max_pulse_width=0.0025)

    def retract(self):
        self.servo.value = self.max
    
    def lock(self):
        self.servo.value = self.min

    def test(self, val=.75):
        self.servo.value = val

if __name__ == '__main__':
    servo = ServoControl()

    servo.initialize(16)#, -45, 45)

    #servo.retract()

    #time.sleep(8)

   # servo.lock()
        
    #servo.test(-1)
    #time.sleep(3)
    #del servo
    
    #while True:
    #    pass
    while True:
        time.sleep(2)
        print("retract")
        servo.retract()
        time.sleep(2)
        print("lock")
        servo.lock()

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

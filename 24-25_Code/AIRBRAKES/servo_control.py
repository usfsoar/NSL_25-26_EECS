from adafruit_servokit import ServoKit # type: ignore

class SERVO:
    def __init__(self, channel=12, min=0, max=360):
        self.channel = channel
        self.min_angle = min
        self.max_angle = max
        
    def initialize(self, port=12):
        found_controller = False
        for i in range(3):
            try:
                self.kit = ServoKit(channels=self.channel)
                found_controller = True
                break
            except:
                if i == 2:
                    print("No servo controller found")
                    return False
                continue
                if found_controller:
                    break
        for i in range(10):
            try:
                self.servo = self.kit.servo[port]
                break
            except:
                if (i == 10):
                    print("Error initializing servo")
                    return False
                continue
        return True
    
    def set_angle(self, ang):
        try:
            ang = max(self.min_angle, min(ang, self.max_angle))
            self.servo.angle = ang
        except:
            print("Servo angle error occurred")

def main():
    import time
    servo= SERVO(16,3,39)
    initialized_servo = servo.initialize()
    if initialized_servo:
        for i in range(10):
            servo.set_angle(servo.min_angle)
            time.sleep(1)
            servo.set_angle(servo.mix_angle)
            time.sleep(1)


if __name__ == '__main__':
    main()
        
from gpiozero import DistanceSensor as GpioDistanceSensor
import time
# sudo apt install python3-gpiozero

class DistanceSensor():
    def __init__(self):
        pass


    def initialize(self, echo_pin: int = 4, trigger_pin: int = 17):
        for i in range(10):
            try:
                self.sensor = GpioDistanceSensor(echo=echo_pin, trigger=trigger_pin)
                break
            except Exception as e:
                if i == 9:
                    raise Exception(f"Error initializing SR04: {e}")
                continue


    def get_distance(self):
        return self.sensor.distance * 100


if __name__ == '__main__':
    sr04 = DistanceSensor()
    try:
        sr04.initialize()
    except Exception as e:
        print(e)

    while True:
        print(f"Distance: {sr04.get_distance():.2f} cm")
        time.sleep(1)

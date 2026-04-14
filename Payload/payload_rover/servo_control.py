# from gpiozero import Servo
# from time import sleep

# class Servo:
#     def __init__(self, pin, min_pw=500/1000000, max_pw=2500/1000000):
#         self.servo = Servo(pin, min_pulse_width=min_pw, max_pulse_width=max_pw)
#         print(f"servo on {pin}")

#     def sweep(self, cycles, delay):
#         for i in range(cycles):
#             self.servo.max()
#             sleep(delay)
                
#             self.servo.min()
#             sleep(delay)
            
#         self.servo.mid()


# # --- Example Usage ---
# if __name__ == "__main__":
#     my_servo = Servo(17)
#     my_servo.sweep(5, 2)


from gpiozero import AngularServo

servo = AngularServo(17, 0, 270)    

servo.angle = 0 
servo.angle = 90
servo.angle = 270
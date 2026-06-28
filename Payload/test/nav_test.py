from motors import Motor, DriveController
from tofvl53 import TOF
# from ina260 import INA260
import time
timeout = 400
# Simplified rover logic :(
left_back_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD10")
right_back_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD40")
left_front_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD16", pwm_pin="BOARD8")
right_front_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD18", pwm_pin="BOARD38")

motors = DriveController(left_back_motor, right_back_motor, left_front_motor, right_front_motor)

# tof = TOF()
# tof.initialize()
# ina = INA260()

time.sleep(0.1)
print("all intialized")

curr_time = time.time()
while True:
    if time.time() - curr_time > timeout:
        print("timeout")
        break

    selected = False
    # try:
    #     selected = plantQueue.get(block=False)
    # except Exception as e:
    #     pass # No message in queue
        
    # dist = sensor_data["distance"] #[0]
    # dist = tof.get_distance()
    # # print(ina.get_current_a())
    # print(dist)

    # if (selected or (dist <= 30 and dist > 7 )):
    #     #stop, delay, rotate 
    #     motors.stop()
    #     time.sleep(1)
    #     curr_time = time.time()

    #     while time.time() - curr_time < 4:
    #         print("turning")
    #         motors.turn_right(1)            
        
    # if ina.get_current_a() > 5:
    #     motors.stop()

    print("move forward")
    motors.move_forward(1)
    time.sleep(0.08) # Prevent looping too quickly
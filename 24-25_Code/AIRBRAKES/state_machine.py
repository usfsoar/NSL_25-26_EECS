import numpy as np
#from adafruit_servokit import ServoKit
import math
import pandas as pd
import random
import pid
import time
import config
import XBeeTransmitter
#need to adjust pid k values
#need to test on unity simulation


class StateMachine:
    def __init__(self, target_height, tyme, acceleration, altitude, velocity, pid_obj):
        self.time = tyme
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity
        self.apogee = 0
        self.target_height = target_height  # Target apogee height (m)
        self.current_state = 0
        self.control_loop = pid_obj
        self.started_state1 = False
        self.started_state2 = False
        self.elapsed_time = time.time()
        self.state1_timeout = 4
        self.state2_timeout = 20
        self.state1_counter = 0
        self.state1_threshold = 5
        self.state3_counter = 0
        self.state3_threshold = 5
        self.state4_counter = 0
        self.state4_threshold = 5
        self.descent_threshold = 5
    def update(self, time, acceleration, altitude, velocity, apogee):
        self.time = time
        self.acceleration = acceleration
        self.altitude = altitude
        self.velocity = velocity
        self.apogee = apogee

    #state machine function 1=thrust 2=post-burnout, check airbrakes, 3=passed apogee, 4=descent
    def states(self):
        self.elapsed_time = time.time()
        match (self.current_state):
            case 0:
                if self.acceleration > 20:
                    self.state1_counter += 1
                    if self.state1_counter >= self.state1_threshold:
                        self.current_state = 1
                        print("Entering state 1")
                    if config.TELEM:
                        self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
            case 1:
                if not self.started_state1:
                    self.started_state1 = True
                    self.state1_time = time.time()
                else:
                    if self.elapsed_time - self.state1_time > self.state1_timeout:
                        if self.altitude > self.target_height:
                            self.current_state = 3
                            self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
                        else:
                            self.current_state = 2
                            print("Entering state 2")
                            if config.TELEM:
                                self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
            case 2:
                if not self.started_state2:
                    self.started_state2 = True
                    self.state2_time = time.time()
                else:
                    if self.elapsed_time - self.state2_time > self.state2_timeout:
                        self.current_state = 3
                        print("Entering state 3")
                        if config.TELEM:
                            self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
                if (self.velocity > 0) and (self.altitude > self.target_height):
                    self.state3_counter +=1
                    self.state4_counter = 0
                    if self.state3_counter >= self.state3_threshold:
                        self.current_state = 3
                        print("Entering state 3")
                    if config.TELEM:
                        self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
                elif (self.velocity < 0) and (self.altitude < self.apogee - self.descent_threshold):
                    self.state4_counter +=1
                    self.state3_counter = 0
                    if self.state4_counter >= self.state4_threshold:
                        self.current_state = 4
                        print("Entering state 4")
                        if config.TELEM:
                            self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
                else:
                    self.state4_counter = 0
                    self.state3_counter = 0
            case 3:
                if (self.velocity < 0) and (self.altitude < self.apogee - self.descent_threshold):
                    self.state4_counter +=1
                    if self.state4_counter >= self.state4_threshold:
                        self.current_state = 4
                        print("Entering state 4")
                        if config.TELEM:
                            self.xbee.send_milestone(f"3,4,Time,{self.time},State,{self.current_state},Altitude,{self.altitude},Velocity,{self.velocity},Acceleration,{self.acceleration}")
                else:
                        self.state4_counter = 0
            case _:
                pass
        # Apply control logic based on the state
        if (self.current_state == 2):
            self.control_loop.update(self.time, self.acceleration, self.altitude, self.velocity)
            #print("updated pid")
            self.servo_angle = self.control_loop.run_PID()
            #print("finished pid")

        elif (self.current_state == 3):
            self.servo_angle = 39 # Set flaps length to max
        else:
            self.servo_angle = 3 # Retract flaps fully on descent
        return self.servo_angle
    def add_xbee(self, xbee):
        self.xbee = xbee
    def getProjectedHeight(self):
        return self.control_loop.proj_height

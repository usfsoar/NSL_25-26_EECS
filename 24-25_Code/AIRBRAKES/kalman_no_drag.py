# dt = 0.1s, pass in as param from pid? depends on where kalman is called
# or calculate dt?
# only for x-axis, assuming horizontal change is negligible

import numpy as np
import math

MODEL_WEIGHT = 0.6
SENSOR_WEIGHT = 0.4
GRAVITY = -9.81
INIT_ALT = 359.8  # m
INIT_VEL = 159.43 # m/s
INIT_ACCEL = -9.8 # m/s^2  - very important to initialize this correctly.
INIT_TIME = 3.757   # maybe should be 0

def calculate_dt(time_1, time_2):
    return time_2 - time_1

def next_step_predictions(prev_alt, prev_vel, prev_accel, dt):
    predicted_vel = prev_vel + prev_accel * dt
    predicted_alt = prev_alt + prev_vel * dt + 0.5 * prev_accel * (dt ** 2)
    return predicted_alt, predicted_vel, prev_accel # assume accel constant over time step


 # add in rolling average before hand, exponential?
def process_sensor_data(sensor_alt, sensor_vel, sensor_accel, prev_alt, prev_vel, prev_accel, dt, first_iteration=False):

    if first_iteration:
        model_alt, model_vel, model_accel = next_step_predictions(INIT_ALT, INIT_VEL, INIT_ACCEL, dt)
    else:
        model_alt, model_vel, model_accel = next_step_predictions(prev_alt, prev_vel, prev_accel, dt)

    filtered_alt, filtered_vel, filtered_accel = filter_data(sensor_alt, sensor_vel, sensor_accel, model_alt, model_vel, model_accel)

    
    return filtered_alt, filtered_vel, filtered_accel

def filter_data(sensor_alt, sensor_vel, sensor_accel, model_alt, model_vel, model_accel):
   # print(f"in filter_data, sensor alt: {sensor_alt}, sensor vel: {sensor_vel}, sensor accel: {sensor_accel}")
    filtered_alt = (MODEL_WEIGHT * model_alt + SENSOR_WEIGHT * sensor_alt) / (MODEL_WEIGHT + SENSOR_WEIGHT)
    filtered_vel = (MODEL_WEIGHT * model_vel + SENSOR_WEIGHT * sensor_vel) / (MODEL_WEIGHT + SENSOR_WEIGHT)
    filtered_accel = (MODEL_WEIGHT * model_accel + SENSOR_WEIGHT * sensor_accel) / (MODEL_WEIGHT + SENSOR_WEIGHT)
    print(f"filtered vs sensor alt: {abs(sensor_alt - filtered_alt)}")
    print(f"filtered vs sensor vel: {abs(sensor_vel - filtered_vel)}")
    print(f"filtered vs sensor accel: {abs(sensor_accel - filtered_accel)}")   
   # print(f"in filter_data, filtered alt: {filtered_alt}, filtered vel: {filtered_vel}, filtered accel: {sensor_accel}")
    return filtered_alt, filtered_vel, filtered_accel




# init alt: 359.8
# init vel: 159.43






# get_velocity(), get_altitude(), and get_acceleration()? technically don't need that?
# drag might complicate things bc have to add in transition equation, it becomes another part of the state vector?
# no drag for now??????????????????????????????? what the fuck


    





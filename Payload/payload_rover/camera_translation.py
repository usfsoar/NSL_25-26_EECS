# from aicam_lib.aicamera import AICamera

import sys
sys.path.append('../')

# Libraries
import time
import numpy as np
import math

# Soar Libraries
import aicam_lib.aicamera as ai
import payload_sensor.relative_thermal_index as thermal

# Camera constants
FOCAL_LENGTH_AI_PIXELS = (ai.RESOLUTION_WIDTH / 2) / (math.tan(math.radians(ai.FOV_HORIZONTAL / 2)))
FOCAL_LENGTH_THERMAL_PIXELS = (thermal.THERMAL_CAM_WIDTH / 2) / (math.tan(math.radians(thermal.FOV_HORIZONTAL / 2)))
DISTANCE_CAMERAS = np.array([0.1, 0, 0]) # Meters [x, y, z] NEEDS TO BE CHANGED. Measured from center



def approximate_distance(T_d0, d0, T_d1, d1, box):    
    # print(f"Distance Estimation: {T_d0}, {d0}, {T_d1}, {d1}")

    # Recover T^-1
    T_inversve = np.array([[(d0 + d1)/(T_d0[0] + T_d1[0]),                             0],
                           [0,                             (d0 + d1)/(T_d0[1] + T_d1[1])]])

    # Get T(x)
    T_x = np.array([abs(box[2] - box[0]), # Horizontal Length
                    abs(box[3] - box[1])]) # Vertical Height
    
    # Apply T^-1(T(d1)) = x
    x = np.matmul(T_inversve, T_x)

    return (x[0] + x[1]) / 2

def recoverT_d(box0, box1, vel0, vel1, elapsed):
    # Get bounding box at timestep t (Vector of height and width)
    # Retrieve targeted inference object (Something like get inference of targeted)

    # box[0], box[1] # Top left
    # box[2], box[3] # Bottom right
    size_0 = np.array([abs(box0[2] - box0[0]), # Horizontal Length
                       abs(box0[3] - box0[1])]) # Vertical Height
    
    # Get bounding box at timestep t+1
    # Get updated inference from main loop
    size_1 = np.array([abs(box1[2] - box1[0]), # Horizontal Length
                       abs(box1[3] - box1[1])]) # Vertical Height
    # Get distance traveled 
    d = elapsed * ((vel1 + vel0) / 2)

    # Linear approximation of T(d): Subtract (size at t+1) - (size at t)
    T_d = size_1 - size_0

    return (T_d, d)


def translate_pixel_to_3d_point(pixel: np.array[int, int], dist: float):
    x = (pixel[0] - ai.RESOLUTION_WIDTH  / 2) / FOCAL_LENGTH_AI_PIXELS
    y = (pixel[1] - ai.RESOLUTION_HEIGHT / 2) / FOCAL_LENGTH_AI_PIXELS
    
    return np.array([x * dist, y * dist, dist])


def project_point_to_thermal(point: np.array[int, int, int]):
    x = FOCAL_LENGTH_THERMAL_PIXELS * (point[0] / point[2]) + thermal.THERMAL_CAM_WIDTH / 2
    y = FOCAL_LENGTH_THERMAL_PIXELS * (point[1] / point[2]) + thermal.THERMAL_CAM_HEIGHT / 2

    return np.array([x, y])


def translate_box_to_thermal(box, dist):
    top_left_point     = translate_pixel_to_3d_point(box[0:1], dist)
    bottom_right_point = translate_pixel_to_3d_point(box[2:3], dist)

    shifted_top_left     = top_left_point     - DISTANCE_CAMERAS
    shifted_bottom_right = bottom_right_point - DISTANCE_CAMERAS

    proj_top_left     = project_point_to_thermal(shifted_top_left)
    proj_bottom_right = project_point_to_thermal(shifted_bottom_right)

    return tuple(int(proj_top_left[0]),     int(proj_top_left[1]),
                 int(proj_bottom_right[0]), int(proj_bottom_right[1]))



# def get_ircamera_disparity(distance):
#     # disparity = Coordinate in Left Image - Coordinate in Right Image
#     # Coordinate IR = Coordinate AI - Disparity 
    
#     disparty = (FOCAL_LENGTH_AI_PIXELS * DISTANCE_CAMERAS) / distance

#     return round(disparty)


if __name__ == "__main__":
    from generateCameraData import *

    while True:
        try:
            print("This test is no longer correct")
            # distance_estimate = target_distance_estimation()
            # distance_actual = getActualDistance()
            # print(f"Estimate of distance from target: {distance_estimate}")
            # print(f"Actual distance from target: {distance_actual}")
            # print(f"Estimate Percent Error: {100 * abs(distance_actual - distance_estimate) / distance_actual}")
            # print(f"Calculated IR camera offset: {get_ircamera_offset(distance_estimate)}\n")
            time.sleep(1)
        except:
            break
# from aicamlib.aicamera import AICamera
import time
import numpy as np

# Camera constants
FOCAL_LENGTH_AI = 0.00474 # Meters
DISTANCE_CAMERAS = 0.1 # Meters NEEDS TO BE CHANGED

def target_distance_estimation():    
    T_d0, d0 = recoverT_d()

    T_d1, d1 = recoverT_d()

    # Recover T^-1
    T_inversve = np.array([[(d0 + d1)/(T_d0[0] + T_d1[0]), 0],
                           [0, (d0 + d1)/(T_d0[1] + T_d1[1])]])

    # Get T(x)
    box = getTargetPlant().box
    T_x = np.array([abs(box[2] - box[0]), # Horizontal Length
                       abs(box[3] - box[1])]) # Vertical Height
    
    # Apply T^-1(T(d1)) = x
    x = np.matmul(T_inversve, T_x)

    return (x[0] + x[1]) / 2


def recoverT_d():
    # Get bounding box at timestep t (Vector of height and width)
    # Retrieve targeted inference object (Something like get inference of targeted)
    box = getTargetPlant().box
    # box[0], box[1] # Top left
    # box[2], box[3] # Bottom right
    size_0 = np.array([abs(box[2] - box[0]), # Horizontal Length
                       abs(box[3] - box[1])]) # Vertical Height
    vel0 = getCurrentVelocity() # Get velocity from BNO
    # Time how long we sleep
    start_time = time.perf_counter()
    time.sleep(0.01)

    # Get bounding box at timestep t+1
    # Get updated inference from main loop
    box = getTargetPlant().box
    size_1 = np.array([abs(box[2] - box[0]), # Horizontal Length
                       abs(box[3] - box[1])]) # Vertical Height
    vel1 = getCurrentVelocity() # Get Velocity from BNO
    elapsed = time.perf_counter() - start_time
    # Get distance traveled 
    d = elapsed * ((vel1 + vel0) / 2)

    # Linear approximation of T(d): Subtract (size at t+1) - (size at t)
    T_d = size_1 - size_0

    return (T_d, d)


def get_ircamera_offset(distance):
    # disparity = Coordinate in Left Image - Coordinate in Right Image
    # Coordinate IR = Coordinate AI - Disparity 
    
    disparty = distance / (FOCAL_LENGTH_AI)(DISTANCE_CAMERAS)

    return disparty
import numpy as np
import time

# Constants
TARGET_START_X = 0 # m
TARGET_START_Y = 0 # m
TARGET_START_Z = 10 # m 
TARGET_HEIGHT = 1 # m
TARGET_WIDTH = 0.5 # m

TARGET_TOP_LEFT_UNSCALED_X = TARGET_START_X
TARGET_TOP_LEFT_UNSCALED_Y = TARGET_START_Y + TARGET_HEIGHT
TARGET_BOTTOM_RIGHT_UNSCALED_X = TARGET_START_X + TARGET_WIDTH
TARGET_BOTTOM_RIGHT_UNSCALED_Y = TARGET_START_Y

ROVER_VELOCITY = 2.24 # m/s

FOCAL_LENGTH_MM = 4.74
SENSOR_SIZE = 6.287
RESOLUTION_WIDTH = 2028
RESOLUTION_HEIGHT = 1520
FRAME_RATE = 30 
FOCAL_LENGTH = (FOCAL_LENGTH_MM * RESOLUTION_WIDTH ) / SENSOR_SIZE # in pixels pixels

# Global State
last_call_time = time.perf_counter()
distance_from_target = TARGET_START_Z
actual_dist = -1

def getTargetPlant():
    global last_call_time
    global distance_from_target
    global actual_dist

    # iterate time
    cur_time = time.perf_counter()
    elapsed_time = cur_time - last_call_time
    last_call_time = cur_time

    # calculate current distance
    distance_traveled = ROVER_VELOCITY * elapsed_time
    distance_from_target -= distance_traveled

    actual_dist = distance_from_target

    # scale coordinates based on sizes
    scaling_factor = FOCAL_LENGTH / distance_from_target

    if scaling_factor < 0:
        return (0, 0, 0, 0)

    # return coordinates
    from collections import namedtuple
    returnVal = namedtuple('returnVal', ['box'])

    if scaling_factor < 0:
        returnVal.box = (0, 0, 0, 0)
    else:
        returnVal.box = (min(TARGET_TOP_LEFT_UNSCALED_X * scaling_factor, RESOLUTION_WIDTH), 
            min(TARGET_TOP_LEFT_UNSCALED_Y * scaling_factor, RESOLUTION_HEIGHT), 
            min(TARGET_BOTTOM_RIGHT_UNSCALED_X * scaling_factor, RESOLUTION_WIDTH),
            min(TARGET_BOTTOM_RIGHT_UNSCALED_Y * scaling_factor, RESOLUTION_HEIGHT)) 
    return returnVal

def getCurrentVelocity():
    return ROVER_VELOCITY


def getActualDistance():
    return actual_dist


if __name__ == '__main__':
    while True:
        print(getTargetPlant())
        time.sleep(1 / (FRAME_RATE + 1))



#!/home/matthew/.conda/envs/soar/bin/python
# CHANGE TO YOUR PATH

import multiprocessing as mp
import multiprocessing.shared_memory as shared_memory
try:
    mp.set_start_method('spawn', force=True)
except RuntimeError:
    # start method already set; ignore
    pass
import numpy as np
import sys
sys.path.append('../../../')
import payload_rover.rover_main as rover_main

def main():
    import cv2
    from controller import Robot, Camera
    from ultralytics import YOLO
    import time


    # model = YOLO("../../../payload_rover/yolo_200epoch.pt")
    TIME_STEP = 1
    robot = Robot()
    ds = []
    dsNames = ['ds_right', 'ds_left']

    for i in range(2):
        ds.append(robot.getDevice(dsNames[i]))
        ds[i].enable(TIME_STEP)
    aicamdv = Robot.getDevice(robot,'aicam')
    Camera.enable(aicamdv, 1)
    ircamdv = Robot.getDevice(robot,'ircam')
    Camera.enable(ircamdv, 1)
    wheels = []

    wheelsNames = ['wheel1', 'wheel2', 'wheel3', 'wheel4']
    for i in range(4):
        wheels.append(robot.getDevice(wheelsNames[i]))
        wheels[i].setPosition(float('inf'))
        wheels[i].setVelocity(0.0)
    avoidObstacleCounter = 0

    SHAPE = (640, 640, 3)
    shm = mp.shared_memory.SharedMemory( create=True,
        size=np.zeros(SHAPE, dtype=np.uint8).nbytes)
    lock = mp.Lock()
    frame = np.ndarray(SHAPE, dtype=np.uint8, buffer=shm.buf)
    rover_main.startWebots((shm.name, lock))

    prev_time = 0
    while robot.step(TIME_STEP) != -1:
        t = time.time()
        if t - prev_time >= (1/30):
            prev_time = t
            # Camera.saveImage(aicamdv, "testingyolo.png", 100)
            raw = Camera.getImage(aicamdv)
            img = np.frombuffer(raw, dtype=np.uint8).reshape((640, 640, 4))
            img = img[:, :, :3][:, :, ::-1]
            with lock:
                frame[:] = img[:]
            
        leftSpeed = 1.0
        rightSpeed = 1.0
        # if avoidObstacleCounter > 0:
            # avoidObstacleCounter -= 1
            
            # leftSpeed = 1.0
            # rightSpeed = -1.0
        # else:  # read sensors
            # for i in range(2):
                # if ds[i].getValue() < 950.0:
                    # avoidObstacleCounter = 100
        wheels[0].setVelocity(leftSpeed)
        wheels[1].setVelocity(rightSpeed)
        wheels[2].setVelocity(leftSpeed)
        wheels[3].setVelocity(rightSpeed)
        
    shm.close()
    shm.unlink()

   
if mp.current_process().name == "MainProcess":
    main()
    
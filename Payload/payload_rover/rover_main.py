
#-----Libraries-----
import multiprocessing as mp
import multiprocessing.shared_memory as shared_memory
import time
import cv2
import os
import math
import sys
import numpy as np
import shutil

#-----Soar Code-----
import aicam_lib.aicamera as ai
# import aicam_lib.webots_aicam as webots_ai
import aicam_lib.rendering as rendering
import payload_rover.camera_translation as translation

import payload_sensor.bno085 as BNO
import payload_sensor.bmp580 as BMP
import payload_sensor.ina260 as INA
import payload_sensor.tofvl53 as TOF
import payload_sensor.relative_thermal_index as RTI

from payload_rover.motors import Motor, DriveController


#-----Constants-----
ALPHA_BNO = 0.8
ALPHA_BMP = 0.8

FRAME_DELAY = 1 / ai.AICAM_FRAME_RATE
MODEL_PATH = "payload_rover/yolo_200epoch.pt"

MAX_DIST = 4 * 2028
BOTTOM_CORRECTION = 70 # TODO** TUNE
DISTANCE_THRESHOLD = 220 # TODO** SUBJECT TO CHANGE AND TESTING
PLANT_CLASSES = set([6, 8]) # TODO**
MAX_UNMATCHED_TIME = 30 # TODO** TUNE 

MAX_AREA = ai.RESOLUTION_WIDTH * ai.RESOLUTION_HEIGHT
MAX_OFFSET = math.sqrt((ai.RESOLUTION_WIDTH/2) ** 2 + (ai.RESOLUTION_HEIGHT/2) ** 2)

# Hyper-parameters TODO** TUNE THEM
ACCEPTABLE_LAST_SEEN = 1
CONFIDENCE_THRESHOLD = 0.1
WEIGHT_AREA = 1
WEIGHT_CONFIDENCE = 0.7
WEIGHT_OFFSET = 1

THERMAL_CAM_DELAY = 1 / RTI.THERMAL_CAM_HZ

MAX_BOX_DELAY = 1500 # TODO** Need to tune
MAX_LIN_DELAY = 1500

print(__file__, mp.current_process().name)


"""
Prints the current function prepended to debug message
"""
def printDBG(*args, **kwargs):
    caller = sys._getframe(1).f_code.co_name
    print(f"[{caller}] -", *args, **kwargs)

    with open(f"webots.log", "a") as file:
        print(f"[{caller}]", *args, **kwargs, file=file)



def startProcess(target, args: tuple, name: str):
    # Use the 'spawn' start method to avoid inheriting Webots' controller
    for _ in range(8):
        try:
            # Create process using spawn context
            p = mp.Process(
                target=target,
                args=args,
                daemon=False,
                name=name
            )

            # Start process
            p.start()

            # Verify it actually started
            if not p.is_alive():
                printDBG(
                    f"Process failed to start (exitcode={p.exitcode})"
                )

            time.sleep(0.1)

            if p.exitcode is not None:
                printDBG(
                    f"Rover process exited immediately with code {p.exitcode}"
                )

            printDBG(f"Successfully started process {name}")
            return p

        except Exception as e:
            printDBG(f"Failed to start rover process: {e}")

    return None


def __roverMain(SIM, timeout, shm_name, plantQueue: mp.Queue, roverToSensorQueue: mp.Queue):
    # Setup sensor shared memory
    sensor_shm = mp.shared_memory.SharedMemory(name=shm_name)
    sensor_data = np.ndarray(1, 
                      dtype=[('velocity', np.float64),
                             ('lin_accel', np.float64), 
                             ('temperature', np.float64),
                             ('current', np.float64),
                             ('distance', np.float64)], 
                      buffer=sensor_shm.buf)
    
    # Simplified rover logic :(
    left_back_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD10")
    right_back_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD40")
    left_front_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD16", pwm_pin="BOARD8")
    right_front_motor = Motor(wheel_diameter=0.1, direction_pin="BOARD18", pwm_pin="BOARD38")

    motors = DriveController(left_back_motor, right_back_motor, left_front_motor, right_front_motor)


    # Exit CubeSat
    # motors.set_speed(0.5,0.5)
    # time.sleep(4)
    motors.move_forward(1, 1)

    # time.sleep(timeout - motors.move_forward(1))

    while True:
        if time.time() > timeout:
            break

        try:
            selected = plantQueue.get(block=False)
        except Exception as e:
            pass # No message in queue

        if selected:
            # Stop 
            motors.set_speed(0, 0)
            if not roverToSensorQueue.full():
                roverToSensorQueue.put(True)

            time.sleep(0.07) # Sleep until process has a good chance to reach queue read
            
            # Go back to normal speed
            motors.set_speed(1,1)

        time.sleep(0.08) # Prevent looping too quickly


    # # Initialize grid pattern
    # move_dist = 0
    # kp = 0.5
    # dist_increment = 1 #meter

    # motor_stall = 0

    # while True:
    #     if time.time() > timeout:
    #         traveled = 0
    #         move_time = time.time()
    #         move_dist = move_dist + dist_increment

    #         while (traveled < move_dist):

    #             # MOVE THIS!!!!!
    #             # I can send plant id, pixel coordinates on screen, confidence, or label
    #             plantDetected = False
    #             try:
    #                 plantDetected = plantQueue.get(block=False) # Must have block=False if you don't want to wait for something to arrive in the queue
    #                 # Also have the timeout options which will block for a time given
    #             except Exception as e:
    #                 pass # Queue will throw empty exception if nothing is in there
    #             if plantDetected:
    #                 # TODO** add frame centering, then move slow
    #                 motors.stop()
    #                 time.sleep(3)
    #                 while plantDetected:
    #                     left, right = im_center_pid()
    #                     motors.set_speed(left, right)
    #                     try:
    #                         plantDetected = plantQueue.get(block=False) # Must have block=False if you don't want to wait for something to arrive in the queue
    #                         # Also have the timeout options which will block for a time given
    #                     except Exception as e:
    #                         plantDetected = False
    #                         pass
    #                     time.sleep(0.05)

    #             traveled += sensor_data['velocity'] * (time.time() - move_time)
    #             error = move_dist - traveled
    #             speed = max(25, min(int(error * kp), 100))
                
                # if ina.is_stall():
                #     motors.stop()
                #     time.sleep(3)

                #     curr = time.time()
                #     while time.time() - curr < 10:
                #         motors.move_backward(speed=100)
                        
                #     time.sleep(3)
                        
                #     while time.time() - curr < 3:
                #         motors.move_backward(speed=50)
                #         if ina.is_stall():
                #             motors.stop()
                #             break
                #         else:
                #             motors.spin_right(speed=75)
                #             continue
                    

                        
            #     motors.set_speed(speed, speed)
            #     time.sleep(0.05)
            
            # motors.spin_right(speed=75)
            
        # check if obstacle function
            # call obstacle avoidance function
            # continue

        # check if plant in queue from plant process
            # plant driving handling
            # continue

        # else drive in holding pattern for an interval


        # alternatively, wait on queue for all messages. handle obstacles or plant that way. 
        # Once handled, then go back to holding and wait again

def im_center_pid():
    #TODO get frame dimensions, tune center constraint and set target constant
    dt = 0.5
    x_left = 270
    x_right = 370

    target_degrees = (x_left + x_right) / 2
    cx = distToCenter()
    error = 320 - cx

    p = abs(error * dt)
    if error < 0:
        right_rpm = p
        left_rpm = -p    
    elif error > 0:
        right_rpm = -p
        left_rpm = p
    else:
        right_rpm = 0
        left_rpm = 0
    
    return left_rpm, right_rpm


def startRoverProcess(args: tuple):
    return startProcess(target=__roverMain, args=args, name="RoverProcess")


class Plant():
    def __init__(self, inference, last_seen: int, t):
        self.inference = inference
        self.last_seen = last_seen
        self.t = t


# Good enough for now
# TODO** Take into account the centers of the boxes
# Boxes centered on same thing are probably looking at the same thing
# Take into account time since last seen too??? More time since last seen, farther distance could've moved
def getBoxDistance(box1, box2):
    dist = 0
    for i in range(4):
        d = abs(box1[i] - box2[i])
        if i == 3:
            d -= BOTTOM_CORRECTION
        
        dist = max(d, dist)

    return dist

prevId = 0
def generateId():
    global prevId
    prevId += 1
    return prevId


def idPlants(prevPlantMap: dict[Plant], inferences):
    t = time.perf_counter()
    plantMap = dict()

    for inference in inferences:
        # Disregard any inferences that aren't plants
        # if inference.label_class not in PLANT_CLASS:
        #     continue

        bestId = -1
        minDist = MAX_DIST
        # Find closest previous label
        for id, oldPlant in prevPlantMap.items():
            dist = getBoxDistance(inference.box, oldPlant.inference.box)
            if dist < DISTANCE_THRESHOLD and dist < minDist:
                # Choose best under threshold
                bestId = id 
                minDist = dist
        
        # Check if this box has already been accounted for by a previous id
        for id, oldPlant in plantMap.items():
            dist = getBoxDistance(inference.box, oldPlant.inference.box)
            if dist < DISTANCE_THRESHOLD and dist < minDist:
                bestId = -2

        if bestId == -1:
            id = generateId()
            plantMap[id] = Plant(inference, 0, t)
        elif bestId == -2:
            continue
        else:
            # Remove item at id from old list
            p = prevPlantMap.pop(bestId)
            p.inference = inference
            p.last_seen = 0
            p.t = t
            plantMap[bestId] = p
            

    # All those in old list still will be put back into queue with - 1 timeout
    for id, unmatched in prevPlantMap.items():
        if unmatched.last_seen < MAX_UNMATCHED_TIME: # If unmatched for full wait, then remove id
            unmatched.last_seen +=1
            plantMap[id] = unmatched

    return plantMap


def __aiMain(SIM: bool, timeout, MODEL_PATH: str, queue: mp.Queue):
    # Initialize AI camera
    aicam = None
    if SIM is not None:
        shm, lock = SIM
        #aicam = webots_ai.WebotsAICamera(network=MODEL_PATH, shm_name=shm, lock=lock, size=(ai.RESOLUTION_WIDTH, ai.RESOLUTION_HEIGHT))
    else:
        aicam = ai.AICamera(network=MODEL_PATH, size=(ai.RESOLUTION_WIDTH, ai.RESOLUTION_HEIGHT))

    # make aicam directory if not already there
    if not os.path.exists("aicam"):
            os.mkdir("aicam")

    prev_start = 0
    plantMap = dict()

    frameNumber = 0

    while True:
        if time.time() > timeout:
            break

        # Ensure we don't get an old frame
        iter_time = time.time() - prev_start
        # printDBG(f"Loop executed in {iter_time}") # Executes in 0.06-0.11. Want 0.033
        if FRAME_DELAY > iter_time:
            delay_time = round(FRAME_DELAY - iter_time + 0.002, 3)
            time.sleep(delay_time)

        # get frame and list of inferences
        prev_start = time.time()
        inferences, frame = aicam.getInference()
        
        # map each plant to id's (using previous)
        plantMap = idPlants(plantMap, inferences)


        if len(inferences) >= 1:
            printDBG(f"Have the following plant inferences in frame {frameNumber}")
            for id, plant in plantMap.items():
                if plant.last_seen == 0:
                    printDBG("\t", f"ID: {id},", plant.inference.to_string()) 

        for id, plant in plantMap.items():
            if plant.last_seen > 15:
                continue
            inf = plant.inference
            color = None
            if plant.last_seen == 0:
                color = (0,255,0)
            else:
                color = (0,0,255)
            rendering.drawBox(frame, box=inf.box, color=color)
            rendering.drawText(frame, 
                               text=f"ID: {id}, Conf: {inf.confidence}", 
                               position=(inf.box[0], inf.box[1]),
                               size=1,
                               color=color,
                               thickness=2)


        # save frame to file with frame number name (remove oldest frame if > 600 images)
        # if os.path.exists(f"aicam/{(frameNumber - 600):05d}.jpg"):
        #     # delete oldest frame
        #     os.remove(f"aicam/{(frameNumber - 600):05d}.jpg")
        # TODO** Conver this to shared memory for sure. Just use a lock and shared memory array. Not too bad
        # Maybe just save all frames if space permits. Then we can reconstruct a video of what the rover saw????
        cv2.imwrite(f"aicam/{frameNumber:05d}.jpg", frame) # TODO** A lot of I/O overhead? Maybe save in shared memory
        frameNumber += 1

        if len(plantMap) == 0:
            continue

        # send id list, boxes, and frame number to plant process
        if queue.full():
            queue.get()
        
        queue.put((plantMap, frameNumber))
        

def startAIProcess(args: tuple):
    return startProcess(target=__aiMain, args=args, name="AICamProcess")





def findArea(box):
    height = box[1] - box[3]
    length = box[0] - box[2]
    return height * length


def distToCenter(box):
    # Find center of box
    center_x = box[2] + (box[0] - box[2]) / 2
    center_y = box[3] + (box[1] - box[3]) / 2

    # Return euclid distance to center
    return math.sqrt((center_x - ai.RESOLUTION_WIDTH/2)**2 + (center_y - ai.RESOLUTION_HEIGHT/2)**2)


def selectPlant(plantMap, ignore: set):
    # Want close to center (center of box to center of image? maybe offset downwards b/c rover is down low?)
    # Want high confidence
    # Want large (ie close and better readings)
    targetId = -1
    maxValue = float('-inf')

    for id, plant in plantMap.items():
        inf = plant.inference
        if id not in ignore and plant.last_seen <= ACCEPTABLE_LAST_SEEN and plant.inference.confidence >= CONFIDENCE_THRESHOLD:
            # Normalize all to < 1
            # maximize area / screen area
            area = findArea(inf.box) / MAX_AREA
            # maximize confidence
            confidence = inf.confidence
            # minimize distance from center of plant.box to center of pixels grid / corner to center
            offset = distToCenter(inf.box) / MAX_OFFSET
            
            # maximize value
            value = (WEIGHT_AREA * area) + (WEIGHT_CONFIDENCE * confidence) - (WEIGHT_OFFSET * offset)
            if value > maxValue:
                maxValue = value
                targetId = id
            
            
    return targetId


def __plantMain(thermal_shm_name, timeout, aiqueue: mp.Queue, sensor_shm_name, plantToRover):
    # Setup sensor shared memory
    sensor_shm = mp.shared_memory.SharedMemory(name=sensor_shm_name)
    sensor_data = np.ndarray(1, 
                      dtype=[('velocity', np.float64),
                             ('lin_accel', np.float64), 
                             ('temperature', np.float64),
                             ('current', np.float64),
                             ('distance', np.float64)], 
                      buffer=sensor_shm.buf)

    thermal_shm = mp.shared_memory.SharedMemory(name=thermal_shm_name)
    thermal_data = np.ndarray(RTI.THERMAL_CAM_WIDTH * RTI.THERMAL_CAM_HEIGHT, 
                      dtype=np.float64, 
                      buffer=thermal_shm.buf)

    prevPlants = {}
    prevLinTransform = {}

    # initialize an ignore id list
    ignore = set()
    if not os.path.exists("plants"):
            os.mkdir("plants")

    while True:
        if time.time() > timeout:
            break
        
        # get most recent from queue
        plantMap, frameNumber = aiqueue.get()

        # choose plant ID according to selection algorithm
        # TODO** Change to choose if good enough target exists
        targetID = selectPlant(plantMap, ignore) # Just choose one over the threshold??

        if targetID == -1:
            continue

        printDBG(f"Target ID: {targetID}")

        # send plant ID and relative position over pipe to rover control (rover should at least slow)
        # roverqueue.
        plantToRover.put(True)

        # TODO** Sleep for a small bit to let rover stop???

        vel = sensor_data['velocity'][0] # Get velocity from BNO
        temp = sensor_data['temperature'][0] # Get tmp from bmp
        thermal_frame = None # Only copy if actually needed
        printDBG(f"Velocity: {vel}")
        # Store time, vel, and size in data structure
        for id, plant in plantMap.items(): 
            if id in ignore: # Don't process plants again
                continue

            old_plant, old_vel = prevPlants.pop(id, (None, None))
            
            if old_plant == None or (plant.t - old_plant.t) > MAX_BOX_DELAY:
                prevPlants[id] = (plant, vel) # No old measurement
            else:
                T_d, d = translation.recoverT_d(old_plant.inference.box, plant.inference.box, old_vel, vel, plant.t - old_plant.t)
                
                printDBG(f"d value: {d}")

                # Try to recover inverse function if previous exists
                T_d0, d_0, t_0 = prevLinTransform.pop(id, (None, None, None))
                if t_0 == None or (plant.t - t_0) > MAX_LIN_DELAY:
                    prevLinTransform[id] = (T_d, d, plant.t)
                else:
                    dist = translation.approximate_distance(T_d0, d_0, T_d, d, plant.inference.box)
                    printDBG(f"Estimated Distance: {dist}")

                    # Calculate box in thermal frame of reference
                    thermal_box = translation.translate_box_to_thermal(plant.inference.box, dist)

                    # Copy frame if this hasn't happened yet this iteration
                    if thermal_frame is None:
                        thermal_frame = thermal_data[:]
                        cv2.imwrite(f"plants/{frameNumber:05d}.jpg", thermal_frame)

                        # Also copy AI Cam frame
                        shutil.copyfile(f"aicam/{frameNumber:05d}.jpg", f"plants/{frameNumber:05d}.jpg")

                    # Calculate health
                    actd = RTI.calculate_ATCD(thermal_frame, thermal_box, temp)

                    # Save plant health data
                    with open(f"plants/{id.txt}", "w") as f:
                        f.write(f"Plant ID: {id}\n"
                              + f"Plant ATCD: {actd}"
                              + f"Distance when reading was taken: {dist}"
                              + f"Top Left Box Thermal Pixel: ({thermal_box[0]}, {thermal_box[1]})"
                              + f"Bottom Right Box Thermal Pixel: ({thermal_box[2]}, {thermal_box[3]})"
                              + f"Time reading was taken: {time.time()}"
                              + f"Frame Number: {frameNumber}")

                    # Add ID to ignore list
                    ignore.add(id)

        

def startPlantProcess(args: tuple):
    return startProcess(target=__plantMain, args=args, name="PlantProcess")




def __SensorMain(timeout, sensor_shm_name, therm_shm_name, roverQueue: mp.Queue):
    # Init shared memory
    sensor_shm = mp.shared_memory.SharedMemory(name=sensor_shm_name)
    data = np.ndarray(1, 
                      dtype=[('velocity', np.float64),
                             ('lin_accel', np.float64), 
                             ('temperature', np.float64),
                             ('current', np.float64),
                             ('distance', np.float64)], 
                      buffer=sensor_shm.buf)
    
    local_data = np.ndarray(1, 
                            dtype=[('velocity', np.float64),
                                   ('lin_accel', np.float64), 
                                   ('temperature', np.float64),
                                   ('current', np.float64),
                                   ('distance', np.float64)])

    thermal_shm = mp.shared_memory.SharedMemory(name=therm_shm_name)
    thermal_frame = np.ndarray(RTI.THERMAL_CAM_WIDTH * RTI.THERMAL_CAM_HEIGHT, 
                      dtype=np.float64, 
                      buffer=thermal_shm.buf)

    # Create all I2C dependent sensors
    bno = BNO.BNO085()
    bno.initialize(ALPHA_BNO)

    bmp = BMP.BMP()
    bmp.initialize(ALPHA_BMP)

    ina = INA.INA()

    # tof = TOF.TOF()
    # tof.initialize()

    # mlx = RTI.MLX()
    # mlx.initialize()
    # thermal_read = 0

    # Init velocity calculation
    local_data['velocity'] = 0
    prev_lin_accel = bno.get_linear_acceleration()
    prev_time = time.perf_counter()

    while True:
        if time.time() > timeout:
            break

        # read all relevant sensor info into local variables
        read_time = time.perf_counter()
        local_data['lin_accel'] = bno.get_linear_acceleration()
        local_data['temperature'] = bmp.get_temperature()
        local_data['current'] = ina.get_current_a()
        local_data['distance'] = None #tof.get_distance()

        # attempt to read from queue for reset velocity bias to 0
        stopped = False
        try: 
            stopped = roverQueue.get(block=False)
        except Exception as e:
            pass # Not stopped since no message in queue

        # calculate current velocity
        if stopped:
            local_data['velocity'] = 0

        delta_t = read_time - prev_time
        delta_v = ((local_data['lin_accel'] + prev_lin_accel) / 2) * delta_t
        local_data['velocity'] = delta_v + local_data['velocity']

        # save all these into shared memory at once
        data[:] = local_data[:]

        # # get frame from mlx if it's ready
        # if (time.time() - thermal_read) > THERMAL_CAM_DELAY:
        #     thermal_frame[:] = mlx.getFrame()[:]
        #     thermal_read = time.time()

        prev_lin_accel = local_data['lin_accel']
        prev_time = read_time




def startSensorProcess(args):
    return startProcess(target=__SensorMain, args=args, name="SensorProcess")



def startWebots(aicamshm, thermshm, aiToPlantQueue, sensor_shm):
    # Init webots rover specifics
    global MODEL_PATH
    MODEL_PATH = "../../../payload_rover/yolo_200epoch.pt"

    timeout = time.time() + 240 # Stop after 4 minutes)

    printDBG("Before process creation")

    # Start processes
    processes = list()
    # processes.append(startRoverProcess((SIM, timeout))) # rover
    processes.append(startAIProcess((aicamshm, timeout, MODEL_PATH, aiToPlantQueue))) # ai cam
    processes.append(startPlantProcess((thermshm, timeout, aiToPlantQueue, sensor_shm))) # plant processing   
    
    printDBG("After process creation")
    
    return processes


if __name__ == "__main__":
    startWebots(None, None, mp.Queue(maxsize=1))
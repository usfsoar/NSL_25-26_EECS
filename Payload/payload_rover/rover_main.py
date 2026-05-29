
#-----Libraries-----
import multiprocessing as mp
import time
import cv2
import os
import math

#-----Soar Code-----
import aicam_lib.aicamera as ai
import aicam_lib.webots_aicam as webots_ai
import payload_rover.camera_translation as translation
import payload_sensor.bno085 as bno085

#-----Constants-----
AICAM_FRAME_RATE = 30
FRAME_DELAY = 1 / AICAM_FRAME_RATE
MODEL_PATH = "payload_rover/yolo_200epoch.pt"

MAX_DIST = 4 * 2028
DISTANCE_THRESHOLD = 30 # TODO** SUBJECT TO CHANGE AND TESTING
PLANT_CLASS = 6 # TODO** BASED ON AI CAMERA MODEL CLASSES

RESOLUTION_WIDTH = 2028
RESOLUTION_HEIGHT = 1520
MAX_AREA = RESOLUTION_WIDTH * RESOLUTION_HEIGHT
MAX_OFFSET = math.sqrt((RESOLUTION_WIDTH/2) ** 2 + (RESOLUTION_HEIGHT/2) ** 2)

# Hyper-parameters TODO** TUNE THEM
WEIGHT_AREA = 1
WEIGHT_CONFIDENCE = 0.7
WEIGHT_OFFSET = 1


import multiprocessing
print(__file__, multiprocessing.current_process().name)


def startProcess(target, args, name, ctx):
    # Use the 'spawn' start method to avoid inheriting Webots' controller
    for _ in range(8):
        try:
            # Create process using spawn context
            p = ctx.Process(
                target=target,
                args=args,
                daemon=False,
                name=name
            )

            # Start process
            p.start()

            # Verify it actually started
            if not p.is_alive():
                print(
                    f"Process failed to start (exitcode={p.exitcode})"
                )

            time.sleep(0.1)

            if p.exitcode is not None:
                print(
                    f"Rover process exited immediately with code {p.exitcode}"
                )

            return p

        except Exception as e:
            print(f"Failed to start rover process: {e}")

    return None


def __roverMain(SIM, timeout):
    # Any sensors?
    # Initialize rover

    while True:
        if time.time() > timeout:
            break

        # check if obstacle function
            # call obstacle avoidance function
            # continue

        # check if plant in queue from plant process
            # plant driving handling
            # continue

        # else drive in holding pattern for an interval


        # alternatively, wait on queue for all messages. handle obstacles or plant that way. 
        # Once handled, then go back to holding and wait again
        
        

def startRoverProcess(args, ctx):
    return startProcess(target=__roverMain, args=args, name="RoverProcess", ctx=ctx)




def getBoxDistance(box1, box2):
    dist = 0
    for i in range(4):
        dist = max(abs(box1[i] - box2[i]), dist)

    return dist

prevId = 0
def generateId():
    global prevId
    prevId += 1
    return prevId


def idPlants(prevPlantMap, inferences):
    # TODO** Make plant boxes store 2-3 frames before getting removed

    plantMap = dict()

    for inference in inferences:
        # Disregard any inferences that aren't plants
        if inference.label_class != PLANT_CLASS:
            continue

        bestId = -1
        minDist = MAX_DIST
        # Find closest previous label
        for id, oldPlant in prevPlantMap.items():
            dist = getBoxDistance(inference.box, oldPlant.box)
            if dist < DISTANCE_THRESHOLD and dist < minDist:
                # Choose best under threshold
                bestId = id 
                minDist = dist
                
        if bestId == -1:
            id = generateId()
            plantMap[id] = inference
        else:
            plantMap[bestId] = inference

    return plantMap


def __aiMain(SIM: bool, timeout, MODEL_PATH, queue: mp.Queue):
    # Initialize AI camera
    aicam = None
    if SIM is not None:
        shm, lock = SIM
        aicam = webots_ai.WebotsAICamera(network=MODEL_PATH, shm_name=shm, lock=lock, size=(RESOLUTION_WIDTH, RESOLUTION_HEIGHT))
    else:
        aicam = ai.AICamera(network=MODEL_PATH, size=(RESOLUTION_WIDTH, RESOLUTION_HEIGHT))

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
        if FRAME_DELAY > iter_time:
            delay_time = round(FRAME_DELAY - iter_time + 0.002, 3)
            time.sleep(delay_time)

        # get frame and list of inferences
        prev_start = time.time()
        inferences, frame = aicam.getInference()
        print(f"DBG - AICam inferences: {inferences}")

        # map each plant to id's (using previous)
        plantMap = idPlants(plantMap, inferences)

        # save frame to file with frame number name (remove oldest frame if > 240 images)
        if os.path.exists(f"aicam/{frameNumber - 240}.jpg"):
            # delete oldest frame
            os.remove(f"aicam/{frameNumber - 240}.jpg")
        cv2.imwrite(f"aicam/{frameNumber}.jpg", frame) # TODO** A lot of I/O overhead? Maybe save in shared memory
        frameNumber += 1

        if len(plantMap) == 0:
            continue

        # send id list, boxes, and frame number to plant process
        # if queue.full():
            # queue.get()
        
        # queue.put((plantMap, frameNumber))
        

def startAIProcess(args, ctx):
    return startProcess(target=__aiMain, args=args, name="AICamProcess", ctx=ctx)





def findArea(box):
    height = box[1] - box[3]
    length = box[0] - box[2]
    return height * length


def distToCenter(box):
    # Find center of box
    center_x = box[2] + (box[0] - box[2]) / 2
    center_y = box[3] + (box[1] - box[3]) / 2

    # Return euclid distance to center
    return math.sqrt((center_x - RESOLUTION_WIDTH/2)**2 + (center_y - RESOLUTION_HEIGHT/2)**2)


def selectPlant(plantMap, ignore: set):
    # Want close to center (center of box to center of image? maybe offset downwards b/c rover is down low?)
    # Want high confidence
    # Want large (ie close and better readings)
    targetId = -1
    maxValue = float('-inf')

    for id, plant in plantMap.items():
        if id not in ignore:
            # Normalize all to < 1
            # maximize area / screen area
            area = findArea(plant.box) / MAX_AREA
            # maximize confidence
            confidence = plant.confidence
            # minimize distance from center of plant.box to center of pixels grid / corner to center
            offset = distToCenter(plant.box) / MAX_OFFSET
            
            # maximize value
            value = (WEIGHT_AREA * area) + (WEIGHT_CONFIDENCE * confidence) - (WEIGHT_OFFSET * offset)
            if value > maxValue:
                maxValue = value
                targetId = id
            
            
    return targetId


def __plantMain(SIM, timeout, aiqueue: mp.Queue):
    # intitialize thermal camera
    thermalcam = None
    if SIM is None:
        # thermalcam = webots camera
        pass
    else:    
        #thermalcam = THERMAL CAM CLASS
        pass

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
        targetID = selectPlant(plantMap)

        # send plant ID and relative position over pipe to rover control (rover should at least slow)
        # TODO**
    
        try:
            # call distance approximation on plant id
            distance = translation.target_distance_estimation(targetID, aiqueue)
        except Exception as e:
            print(e)
            continue
    
        # calculate offset
        disparity = translation.get_ircamera_offset(distance)

        print(f"Plant detected: {targetID}, distance: {distance}")

        # Retrieve pixels from thermal 
        # TODO**

        # Save ID + health + copy picture to readings
        # TODO**
        # Copy f"{frameNumber}.jpg" will need to draw plant on it too to plant folder and rename to plant id
        # store plant id, health, location? into csv file in plants folder


        # add ID to ignore list
        # ignore.add(targetID)

        

def startPlantProcess(args):
    return startProcess(target=__plantMain, args=args, name="PlantProcess")


def startWebots(aicamshm):
    # Init webots rover specifics
    global MODEL_PATH
    MODEL_PATH = "../../../payload_rover/yolo_200epoch.pt"

    timeout = time.time() + 240 # Stop after 4 minutes
    ctx = mp.get_context('spawn')
    aiToPlant = ctx.Queue(maxsize=1)

    # Start processes
    processes = list()
    # processes.append(startRoverProcess((SIM, timeout))) # rover
    processes.append(startAIProcess((aicamshm, timeout, MODEL_PATH, aiToPlant), ctx)) # ai cam
    # processes.append(startPlantProcess((SIM, timeout, aiToPlant))) # plant processing   
    
    # wait on all 3 to finish
    # for p in processes:
        # p.join()

#-----Libraries-----
import multiprocessing as mp
import time
import cv2
import os

#-----Soar Code-----
import aicam_lib as ai
import payload_rover.camera_translation as translation

#-----Constants-----
AICAM_FRAME_RATE = 30
FRAME_DELAY = 1 / AICAM_FRAME_RATE
MODEL_PATH = "payload_rover/yolo_200epoch.pt"

MAX_DIST = 4 * 2028
DISTANCE_THRESHOLD = 30 # TODO** SUBJECT TO CHANGE AND TESTING
PLANT_CLASS = 6 # TODO** BASED ON AI CAMERA MODEL CLASSES




def startProcess(target, args, name):
    for _ in range(8):
        try:
            # Create process
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
                raise RuntimeError(
                    f"Process failed to start (exitcode={p.exitcode})"
                )

            time.sleep(0.1)

            if p.exitcode is not None:
                raise RuntimeError(
                    f"Rover process exited immediately with code {p.exitcode}"
                )

            return p

        except Exception as e:
            print(f"Failed to start rover process: {e}")
    
    return None


def __roverMain(timeout, ):
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
        
        

def startRoverProcess(args):
    return startProcess(target=__roverMain, args=args, name="RoverProcess")




def getBoxDistance(box1, box2):
    dist = 0
    for i in range(4):
        dist = max(abs(box1[i] - box2[i]), dist)

    return dist

prevId = 0
def generateId():
    prevId += 1
    return prevId


def idPlants(prevPlantMap, inferences):
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


def __aiMain(timeout, queue: mp.Queue): #queue is of size 1
    # Initialize AI camera
    aicam = ai.AICamera(MODEL_PATH)
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

        # map each plant to id's (using previous)
        plantMap = idPlants(plantMap, inferences)

        # save frame to file with frame number name (remove oldest frame if > 240 images)
        if os.path.exists(f"{frameNumber - 240}.jpg"):
            # delete oldest frame
            os.remove(f"{frameNumber - 240}.jpg")
        # frame_bgr = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR) # Might need to swap blue and red channels
        cv2.imwrite(f"aicam/{frameNumber}.jpg", frame) # A lot of I/O overhead?
        frameNumber += 1

        # send id list, boxes, and frame number to plant process
        if queue.full():
            queue.get()
        
        queue.put((plantMap, frameNumber))
        


def startAIProcess(args):
    return startProcess(target=__aiMain, args=args, name="AICamProcess")



def __plantMain(timeout, aiqueue: mp.Queue):
    # intitialize thermal camera
    #thermalcam = 
    # initialize an ignore id list
    ignore = list()
    if not os.path.exists("plants"):
            os.mkdir("plants")

    while True:
        if time.time() > timeout:
            break
        
        # get most recent from queue
        plantMap, frameNumber = aiqueue.get()

        # choose plant ID according to selection algorithm
        # TODO**
        targetID = selectPlant(plantMap)

        # send plant ID and relative position over pipe to rover control
        # TODO**
    
        try:
            # call distance approximation on plant id
            distance = translation.target_distance_estimation(targetID, aiqueue)
        except Exception as e:
            print(e)
            continue
    
        # calculate offset
        disparity = translation.get_ircamera_offset(distance)

        # Retrieve pixels from thermal 
        # TODO**

        # Save ID + health + copy picture to readings
        # TODO**

        # add ID to ignore list
        ignore.append(targetID)

        

def startPlantProcess(args):
    return startProcess(target=__plantMain, args=args, name="PlantProcess")



if __name__ == "__main__":
    # Init webots rover 
     
    # Start processes
    processes = list()
    processes.append(startRoverProcess()) # rover
    processes.append(startAIProcess()) # ai cam
    processes.append(startPlantProcess()) # plant processing   
    
    # wait on all 3 to finish
    for p in processes:
        p.join()
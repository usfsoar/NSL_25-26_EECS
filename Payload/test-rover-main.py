import payload_rover.rover_main as rover_main
import numpy as np
import multiprocessing as mp
import time
import payload_sensor.relative_thermal_index as RTI

THERMAL_SHAPE = (RTI.THERMAL_CAM_HEIGHT, RTI.THERMAL_CAM_WIDTH, 1)
ROVER_SCAN_TIMEOUT = 900

sensor_shm = mp.shared_memory.SharedMemory(create=True, 
        size=np.zeros(1, dtype=[('velocity', np.float64), ('lin_accel', np.float64), ('temperature', np.float64), ('current', np.float64), ('distance', np.float64)]).nbytes)

# Create and pass in all required args for the threads
# AI to Plant Process Message Queue
aiToPlantQueue = mp.Queue(maxsize=1)
plantToRover = mp.Queue(maxsize=1)
roverToSensorQueue = mp.Queue(maxsize=1)

# Thermal Camera Shared Memory
thermal_shm = mp.shared_memory.SharedMemory(create=True,
size=np.zeros(THERMAL_SHAPE, dtype=np.float64).nbytes)

# Start processes
timeout_time = time.time() + ROVER_SCAN_TIMEOUT
processes = list()
processes.append(rover_main.startRoverProcess((None, timeout_time, sensor_shm.name, plantToRover, roverToSensorQueue))) # rover

for _ in range(15):
    time.sleep(1)
    plantToRover.put(True)
    time.sleep(0.001)
    print(roverToSensorQueue.get())


for p in processes:
    p.join()
    
sensor_shm.close()
sensor_shm.unlink()
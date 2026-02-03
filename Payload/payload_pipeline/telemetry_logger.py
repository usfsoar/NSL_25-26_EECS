# If using multiprocessing, switch the queue type being used
from queue import Queue
# from multiprocessing import Queue

from os import path
import time
import pandas as pd

# Useage: Create a TelemetryLogger object, then pass in your data to the corresponding function
class TelemetryLogger:
    def __init__(self):
        # Get sensor data csv object
        self.sensor_data_csv = self.__SensorDataCsv() 
        
    def print_helper(self, data):
        for label, data in data.items():        
            print(str(label) + ": " + str(data), end=", ")
        print()

    def log_sensor(self, data : dict):
        self.sensor_data_csv.queue.put(data)
        self.sensor_data_csv.write()
        self.print_helper(data)

    # def log_rover():
    
    # def log_console():



    # Singleton Class for Sensor Data CSV File
    # TODO: Make this class into an abstract class so all files can use it
    class __SensorDataCsv:
        __SENSOR_LOGGING_PATH = "sensor_data" + str(time.time()) + ".csv" 

        def __new__(cls):
            if not hasattr(cls, 'inst'):
                cls.inst = super().__new__(cls)
            return cls.inst 
        
        def __init__(self):
            self.queue = Queue()
            
        def write(self):
            data = self.queue.get(block=True)
            df = pd.DataFrame([data]) 
            # TODO Keep file open or maybe try not to recreate objects on each write??
            # Batch writes in the queue to logging speed up? Very slow as is
            df.to_csv(path_or_buf=self.__SENSOR_LOGGING_PATH, mode='a', 
                      header=not path.exists(self.__SENSOR_LOGGING_PATH), index=False)


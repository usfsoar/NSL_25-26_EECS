from queue import Queue
# from multiprocessing import Queue

from os import path
from abc import ABC, abstractmethod
from datetime import datetime
import pandas as pd


# Useage: Create a TelemetryLogger object, then pass in your data to the corresponding logging function
class TelemetryLogger:
    def __init__(self):
        # Get sensor data csv object
        self.sensor_data_csv = SensorDataCsv() 
        self.LOGGING_FILE_PATH = self.sensor_data_csv.LOGGING_FILE_PATH
        
    @staticmethod
    def get_timestamp():
        return datetime.now().strftime("%H:%M:%S.%f")[:15]
    
    @staticmethod
    def print_dictionary(data: dict):
        for label, data in data.items():        
            print(str(label) + ": " + str(data), end=", ")
        print()

    def log_sensor(self, data : dict):
        self.sensor_data_csv.queue.put(data)
        self.sensor_data_csv.write()
        TelemetryLogger.print_dictionary(data) # Print to stdout too for our viewing pleasure

    # If/When we want to create more log types:
    # def log_rover():
    
    # def log_console():



# Classes for managing csv files
# TODO: Make the singleton logic better (ie metaclass) 
# TODO: Make singleton logic thread safe
class AbstractLoggingCSV(ABC):
    LOGGING_FILE_PATH = None # Subclass must override

    # Only 1 instance of this each subclass is allowed
    def __new__(cls):
        if not hasattr(cls, 'inst'):
            cls.inst = super().__new__(cls)
            cls.inst._initialized = False
        return cls.inst 
    
    def __init__(self):
        if not self._initialized:
            # TODO Create a thread/process to run the write function asynchronously??
            self.queue = Queue()
            self.inst._initialized = True
        
    def write(self):
        # TODO Keep file open or maybe try not to recreate objects on each write??
        # Batch writes in the queue to logging speed up? Very slow as is
        data = self.queue.get(block=True)
        df = pd.DataFrame([data]) 
        df.to_csv(path_or_buf=self.LOGGING_FILE_PATH, mode='a', 
                header=not path.exists(self.LOGGING_FILE_PATH), index=False)


class SensorDataCsv(AbstractLoggingCSV):
    LOGGING_FILE_PATH = "sensor_data_" + str(TelemetryLogger.get_timestamp()) + ".csv" 


class RoverDataCsv(AbstractLoggingCSV):
    LOGGING_FILE_PATH = "rover_data_" + str(TelemetryLogger.get_timestamp()) + ".csv" 
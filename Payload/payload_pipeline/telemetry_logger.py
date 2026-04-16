from queue import Queue
# from multiprocessing import Queue
import multiprocessing as mp

import os
from abc import ABC, abstractmethod
from datetime import datetime
import csv


# Useage: Create a TelemetryLogger object, then pass in your data to the corresponding logging function
class TelemetryLogger:
    def __init__(self, sensor_data: dict):
        if not os.path.exists("logs"):
            os.mkdir("logs")

        # Get sensor data csv object
        self.sensor_data_csv = SensorDataCsv(sensor_data) 
        self.LOGGING_FILE_PATH = self.sensor_data_csv.LOGGING_FILE_PATH
        
    @staticmethod
    def get_timestamp():
        return datetime.now().strftime("%H:%M:%S.%f")[:15]
    
    @staticmethod
    def string_to_sec(t):
        t = datetime.strptime(t, "%H:%M:%S.%f").time()
        return t.hour * 3600 + t.minute * 60 + t.second + t.microsecond / 1e6
    
    @staticmethod
    def sec_to_string(sec):
        hours = sec // 3600
        minutes = (sec // 60) % 60
        sec = sec - (hours * 3600) - (minutes * 60)
        return f"{hours:02.0f}:{minutes:02.0f}:{sec:02.0f}.0"

    @staticmethod
    def print_dictionary(data: dict):
        for label, data in data.items():        
            print(str(label) + ": " + str(data), end=", ")
        print()

    def log_sensor(self, data : dict):
        self.sensor_data_csv.queue.put(data)
        TelemetryLogger.print_dictionary(data) # Print to stdout too for our viewing pleasure

    def kill(self):
        self.sensor_data_csv.kill()

    # If/When we want to create more log types:
    # def log_rover():
    
    # def log_console():



# Classes for managing csv files
# TODO: Make the singleton logic better (ie metaclass) 
# TODO: Make singleton logic thread safe
class AbstractLoggingCSV(ABC):
    LOGGING_FILE_PATH = None # Subclass must override

    # Only 1 instance of this each subclass is allowed
    def __new__(cls, sensor_data: dict):
        if not hasattr(cls, 'inst'):
            cls.inst = super().__new__(cls)
            cls.inst._initialized = False
        return cls.inst 
    
    def __init__(self, sensor_data: dict):
        if not self._initialized:
            self.queue = mp.Queue()
            self.inst._initialized = True
            
            with open(self.LOGGING_FILE_PATH, 'a', newline="") as csvfile:
                # Write csv header
                self.fieldnames = list(sensor_data.keys())
                writer = csv.DictWriter(csvfile, fieldnames=self.fieldnames)
                writer.writeheader()

            self.process = mp.Process(target=AbstractLoggingCSV.write, args=(self.LOGGING_FILE_PATH, self.queue, self.fieldnames), )
            self.process.start()

    def kill(self):
        self.queue.put(None)
        self.process.join()
        self._initialized = False

        
    def write(path, queue, fieldnames):
        with open(path, 'a', newline="") as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            while True:
                data = queue.get(block=True)
                if data == None:
                    break

                writer.writerow(data)
                csvfile.flush()
                os.fsync(csvfile.fileno())


class SensorDataCsv(AbstractLoggingCSV):
    LOGGING_FILE_PATH = "logs/sensor_data_" + str(TelemetryLogger.get_timestamp()) + ".csv" 


class RoverDataCsv(AbstractLoggingCSV):
    LOGGING_FILE_PATH = "logs/rover_data_" + str(TelemetryLogger.get_timestamp()) + ".csv" 

    


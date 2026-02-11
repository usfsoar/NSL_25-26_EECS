import csv
import matplotlib
import pandas as pd
import numpy as np
from typing import List, Dict, Union

Numeric = Union[int, float]

# Class which will contain functions for storing data into csv files on an SD card
# Will also contain functions to visualize data
class RocketData():

    # Constructor
    def __init__(self, filename : str, headers : List[str]):
        self.filename = filename
        self.headers = headers
    
    # Create the csv file and write headers
    def createFile(self):
        data = {}
        for x in self.headers:
            data[x] = []
        df = pd.DataFrame(data)
        df.to_csv(self.filename, mode = "w", header = True, index = False)
    
    # write to the csv file
    # Keys of input dict should be the names of the file headers
    def writeFile(self, data: Dict[str, Numeric]):
        df = pd.DataFrame(data)
        df.to_csv(self.filename, mode = "a", header = False, index = False)
           
    # will return an array of all data points in a row
    def readRow(self):
        pass
    
    # Will save the csv file to the sd card
    def saveSD(self):
        pass

    # Will create a line graph with given x and y axis
    def createLineGraph(self, x_axis : str, y_axis : str):
        pass

    # Will create a figure with multiple sub plots 
    # and save to sd card (hopefully)
    # Examples:
    # Predicted apogee/altitude vs time
    # States/altitude/velocity vs time
    # Flap actuation/acceleration vs time
    def masterPlot(self):
        pass


if __name__ == '__main__':
    pass
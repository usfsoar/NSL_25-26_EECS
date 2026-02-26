import csv
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from typing import List, Dict, Union
import math

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
    def readColumn(self):
        df = pd.read_csv(self.filename)
        arr = df.to_numpy()
        return arr[:,0:3]
    
    # Will save the csv file to the sd card
    def saveSD(self):
        pass

    # Will create a line graph with given x and y axis
    def createLineGraph(self, x_axis : str, y_axis : str):
        plt.plot([1,2,3,4])
        plt.show()
        #x_values = np.linspace(0, landing_time, length_of_time)
        #y_values = whatever column is passed in (e.g. apogee)
        
        #plt.plot(x_values, y_values)
        #plt.axis(0, landing_time, min(passed array), max(passed array))
        #plt.show()

    # Will create a figure with multiple sub plots 
    # and save to sd card (hopefully)
    # Examples:
    # Predicted apogee/altitude vs time
    # States/altitude/velocity vs time
    # Flap actuation/acceleration vs time
    def masterPlot(self):
        fig, ax = plt.subplots(2, 2)
        plt.show()
        #ax[0, 0].plot(list(range(round(landing_time))), [0, 12500])
        #ax[0, 1].plot(list(range(round(landing_time))), [min(velocity), max(velocity)])
        #ax[1, 0].plot(list(range(round(landing_time))), [0, 6000])
        #ax[1, 1].plot(list(range(round(landing_time))), [min(acceleration), max(velocity)])
        


if __name__ == '__main__':
    fig, ax = plt.subplots(2, 2)
    #ax[0, 0].plot(list(range(round(landing_time))), [1,2,3,5,9])
    plt.show()
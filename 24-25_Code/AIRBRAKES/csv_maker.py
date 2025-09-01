import numpy as np
import pandas as pd

class CSV:

    def __init__(self,path,data,path_index=0):
        self.path_base = path
        self.path = self.path_base + str(path_index) + '.csv'
        self.data = data
    
    def make_csv(self):
        df = pd.DataFrame(self.data)
        df.to_csv(self.path, header=True, index=False)

    def update_csv(self, new_data):
        df = pd.DataFrame(new_data)
        df.to_csv(self.path, mode = 'a', header=False, index=False)

    def set_index(self, path_index):
        self.path = self.path_base + str(path_index) + '.csv'
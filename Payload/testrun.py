from Sensor_Data_Simulator import *
import pandas as pd

altVals = []
speedVals = []
accelVals = []

for x in Sensor_Data_Simulator.tRange:
    Sensor_Data_Simulator.updateValues()
    
    #Sensor_Data_Simulator.getAccel()
    #Sensor_Data_Simulator.getVelocity()

    altVals.append(Sensor_Data_Simulator.getAlt())
    speedVals.append(Sensor_Data_Simulator.getAccel())
    accelVals.append(Sensor_Data_Simulator.getAccel())

    print(Sensor_Data_Simulator.getAlt())
    if Sensor_Data_Simulator.currentHeight < 0:
        break

dict = {'Altitude':altVals, 'Velocity':speedVals, 'Acceleration':accelVals}
df = pd.DataFrame(dict)
df.to_csv('flightdata.csv')
#csv without indices or header
#df.to_csv('flightdata2.csv', header=False, index=False)
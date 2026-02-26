#Testing matplotlib + csv
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import csv

time = []
altitude = []
velocity = []
acceleration_x = []
acceleration_y = []
acceleration_z = []
state = []
steps = []
predicted_apogee = []
error = []

fig, ax = plt.subplots(2, 2, figsize = (18, 12)) # creates 2x2 grid of plots, figsize is size of overall graph

#plt will change the bottom-rightmost plot

with open('C:/Users/jojol/Downloads/flightdata3.csv') as csvfile:
    lines = csv.reader(csvfile, delimiter = ',')
    next(lines)
    for row in lines:
       # if type(row[0]) != float:
        #    continue
        
        time.append(row[0])
        altitude.append(float(row[1]))
        velocity.append(float(row[2]))
        acceleration_x.append(float(row[3]))
        acceleration_y.append(float(row[4]))
        acceleration_z.append(float(row[5]))
        state.append(float(row[6]))
        steps.append(float(row[7]))
        predicted_apogee.append(float(row[8]))
        error.append(float(row[8]) - float(row[1]))
        

fig.suptitle('Data', fontsize = 20) # fig.suptitle sets overall title

# Graph 1, Time vs Altitude
ax[0, 0].plot(time, altitude, color = 'black', linestyle = "solid")
    #label is overlaid onto the graph
# ax[0, 0].plot(time, acceleration_x, color = 'b', linestyle = 'solid')
# ax[0, 0].plot(time, acceleration_y, color = 'r', linestyle = 'solid')
# ax[0, 0].plot(time, acceleration_z, color = 'g', linestyle = 'solid')
ax[0, 0].title.set_text("Altitude") # titles top left plot
ax[0, 0].set_xlabel('Time')
ax[0, 0].set_ylabel('Altitude')
ax[0, 0].grid()
ax[0, 0].legend()
ax[0, 0].tick_params(axis = 'x', labelrotation = 45, labelsize = 7)
ax[0, 0].set_xlim(0, time[len(time) - 1])
ax[0, 0].set_ylim(min(altitude), max(altitude) * 1.1)
ax[0, 0].xaxis.set_major_locator(ticker.MultipleLocator(int(time[len(time) - 1]) / 20)) # time between ticks on x axis
ax[0, 0].annotate(
    "Max: " + str(max(altitude)),
    xy = (time[altitude.index(max(altitude))], max(altitude) * 1.01), color = 'black', fontsize = 14)

# Graph 1, Time vs Acceleration X
# ax2 = ax[0, 0].twinx() # creates second y axis on right side of graph  
# ax2.plot(time, acceleration_x, color = 'b', linestyle = 'solid')
# ax2.set_ylabel('Acceleration X', color = 'b')
# ax2.tick_params(axis = 'y', labelcolor = 'b')

# Graph 1, Time vs Acceleration Y
# ax3 = ax[0, 0].twinx()
# ax3.plot(time, acceleration_y, color = 'r', linestyle = 'solid')
# ax3.set_ylabel('Acceleration Y', color = 'r')
# ax3.tick_params(axis = 'y', labelcolor = 'r')

# Graph 1,Time vs Acceleration Z
ax4 = ax[0, 0].twinx()
ax4.plot(time, acceleration_z, color = 'g', linestyle = 'solid')
ax4.set_ylabel('Acceleration Z', color = 'g')
ax4.tick_params(axis = 'y', labelcolor = 'g')
ax4.spines.right.set_position(("axes", 1.15))


# Graph 1, Time vs Velocity

ax5 = ax[0, 0].twinx()
ax5.plot(time, velocity, color = 'm', linestyle = 'solid')
ax5.set_ylabel('Velocity', color = 'm')
ax5.tick_params(axis = 'y', labelcolor = 'm')
ax5.spines.left.set_position(("axes", -.15))
ax5.yaxis.set_label_position('left')
ax5.yaxis.tick_left()

# Graph 1, Time vs State
ax6 = ax[0, 0].twinx()
ax6.plot(time, state, color = 'c', linestyle = 'solid')
ax6.set_ylabel('State', color = 'c')
ax6.tick_params(axis = 'y', labelcolor = 'c')

# Graph 2, Time vs Error
ax[0, 1].plot(time, error, color = 'r', linestyle = 'solid')
ax[0, 1].title.set_text("Error")
ax[0, 1].set_xlabel('Time')
ax[0, 1].set_ylabel('Error')
ax[0, 1].grid()
ax[0, 1].legend()
ax[0, 1].tick_params(axis = 'x', labelrotation = 45, labelsize = 7)
ax[0, 1].set_xlim(0, time[len(time) - 1])
ax[0, 1].set_ylim(min(error), max(error) * 1.1)
ax[0, 1].xaxis.set_major_locator(ticker.MultipleLocator(int(time[len(time) - 1]) / 20))

# Graph 2, Time vs Steps
ax2_1 = ax[0, 1].twinx()
ax2_1.plot(time, steps, color = 'y', linestyle = 'solid')
ax2_1.set_ylabel('Steps', color = 'y')
ax2_1.tick_params(axis = 'y', labelcolor = 'y')

# Graph 2, Time vs PID Output / Steps(?)
# ax2_2 = ax[0, 1].twinx()
# ax2_2.plot(time, predicted_apogee, color = 'm', linestyle = 'solid')
# ax2_2.set_ylabel('Predicted Apogee', color = 'm')
# ax2_2.tick_params(axis = 'y', labelcolor = 'm')

# Graph 3, Time vs Error
ax[1, 0].plot(time, error, color = 'r', linestyle = 'solid')
ax[1, 0].title.set_text("Error")
ax[1, 0].set_xlabel('Time')
ax[1, 0].set_ylabel('Error')
ax[1, 0].grid()
ax[1, 0].legend()
ax[1, 0].tick_params(axis = 'x', labelrotation = 45, labelsize = 7)
ax[1, 0].set_xlim(0, time[len(time) - 1])
ax[1, 0].set_ylim(min(error), max(error) * 1.1) 
ax[1, 0].xaxis.set_major_locator(ticker.MultipleLocator(int(time[len(time) - 1]) / 20))

# Graph 3, Time vs Predicted Apogee
ax3_1 = ax[1, 0].twinx()
ax3_1.plot(time, predicted_apogee, color = 'g', linestyle = 'solid')
ax3_1.set_ylabel('Predicted Apogee', color = 'g')
ax3_1.tick_params(axis = 'x', labelcolor = 'g')

plt.subplots_adjust(left = 0.12, right = 0.95, top = 0.85, bottom = 0.15)
# fig.tight_layout(pad = 3.0)
plt.subplots_adjust(wspace=0.5, hspace=0.4)

plt.savefig('flightdata4.png') # saves graph to SD card")
plt.show()


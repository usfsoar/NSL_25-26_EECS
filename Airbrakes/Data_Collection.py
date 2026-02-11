#Testing matplotlib + csv
import matplotlib.pyplot as plt
import csv

time = []
altitude = []
velocity = []
acceleration_x = []
acceleration_y = []
acceleration_z = []
state = []
servo_angle = []
predicted_apogee = []

fig, ax = plt.subplots(2, 2)

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
        #acceleration_y.append(float(row[4]))
        #acceleration_z.append(float(row[5]))
        #state.append(float(row[6]))
        #servo_angle.append(float(row[7]))
        #predicted_apogee.append(float(row[8]))
    print(time)

fig.suptitle('Data', fontsize = 20) # fig.suptitle sets overall title

# Time vs Altitude
ax[0, 0].plot(time, altitude, color = 'g', linestyle = "solid")
    #label is overlaid onto the graph
ax[0, 0].title.set_text("Altitude") # titles top left plot
ax[0, 0].set_xlabel('Time')
ax[0, 0].set_ylabel('Altitude')
ax[0, 0].grid()
ax[0, 0].legend()
ax[0, 0].tick_params(axis = 'x', labelrotation = 45)
print(time)
ax[0, 0].set_xlim(0, max(time))
ax[0, 0].set_ylim(min(altitude), max(altitude))


# Time vs Velocity
ax[0, 1].plot(time, velocity, color = 'r', linestyle = 'solid')
ax[0, 1].title.set_text("Velocity")
ax[0, 1].set_xlabel('Time')
ax[0, 1].set_ylabel('Velocity')
ax[0, 1].grid()
ax[0, 1].legend()
ax[0, 1].tick_params(axis = 'x', labelrotation = 45)
ax[0, 1].set_xlim(0, max(time))

# Time vs Acceleration X, Y, Z
ax[1, 0].plot(time, acceleration_x, color = 'b', linestyle = 'solid')
#ax[1, 0].plot(time, acceleration_y, color = 'r', linestyle = 'solid')
#ax[1, 0].plot(time, acceleration_z, color = 'g', linestyle = 'solid')
ax[1, 0].title.set_text("Accelerations")
ax[1, 0].set_xlabel('Time')
ax[1, 0].set_ylabel('Accelerations')
ax[1, 0].grid()
ax[1, 0].legend()
ax[1, 0].tick_params(axis = 'x', labelrotation = 45)
ax[1, 0].set_xlim(0, max(time))

# # Time vs Servo Angle and Predicted Apogee
# #ax[1, 1].plot(time, servo_angle, color = 'y', linestyle = 'solid')
# #ax[1, 1].plot(time, predicted_apogee, color = 'm', linestyle = 'solid')
# ax[1, 1].title.set_text("Servo Angle and Predicted Apogee")
# ax[1, 1].set_xlabel('Time')
# ax[1, 1].set_ylabel('Acceleration Y')
# ax[1, 1].grid()
# ax[1, 1].legend()
# ax[1, 1].tick_params(axis = 'x', labelrotation = 45)
# ax[1, 1].set_xlim(0, max(time))

plt.show()

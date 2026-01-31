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

with open('C:/Users/jojol/Downloads/CSVTester.csv') as csvfile:
    lines = csv.reader(csvfile, delimiter = ',')
    for row in lines:
        time.append(row[0])
        altitude.append(int(row[1]))
        velocity.append(int(row[2]))
        acceleration_x.append(int(row[3]))
        acceleration_y.append(int(row[4]))
        acceleration_z.append(int(row[5]))
        state.append(int(row[6]))
        servo_angle.append(int(row[7]))
        predicted_apogee.append(int(row[8]))

fig.suptitle('Data', fontsize = 20) # fig.suptitle sets overall title

# Time vs Altitude
ax[0, 0].plot(time, altitude, color = 'g', linestyle = "solid")
    #label is overlaid onto the graph
ax[0, 0].title.set_text("Time vs Altitude") # titles top left plot
ax[0, 0].set_xlabel('Time')
ax[0, 0].set_ylabel('Altitude')
ax[0, 0].grid()
ax[0, 0].legend()
ax[0, 0].tick_params(axis = 'x', labelrotation = 45)
ax[0, 0].set_xlim(0, max(time))
ax[0, 0].set_ylim(min(altitude), max(altitude))


# Time vs Velocity
ax[0, 1].plot(time, velocity, color = 'r', linestyle = 'solid')
ax[0, 1].title.set_text("Time vs Velocity")
ax[0, 1].set_xlabel('Time')
ax[0, 1].set_ylabel('Velocity')
ax[0, 1].grid()
ax[0, 1].legend()
ax[0, 1].tick_params(axis = 'x', labelrotation = 45)
ax[0, 1].set_xlim(0, max(time))

# Time vs Acceleration X, Y, Z
ax[1, 0].plot(time, acceleration_x, color = 'b', linestyle = 'solid')
ax[1, 0].plot(time, acceleration_y, color = 'r', linestyle = 'solid')
ax[1, 0].plot(time, acceleration_z, color = 'g', linestyle = 'solid')
ax[1, 0].title.set_text("Time vs Accelerations")
ax[1, 0].set_xlabel('Time')
ax[1, 0].set_ylabel('Accelerations')
ax[1, 0].grid()
ax[1, 0].legend()
ax[1, 0].tick_params(axis = 'x', labelrotation = 45)
ax[1, 0].set_xlim(0, max(time))

# Time vs Servo Angle and Predicted Apogee
ax[1, 1].plot(time, servo_angle, color = 'y', linestyle = 'solid')
ax[1, 1].plot(time, predicted_apogee, color = 'm', linestyle = 'solid')
ax[1, 1].title.set_text("Time vs Servo Angle and Predicted Apogee")
ax[1, 1].set_xlabel('Time')
ax[1, 1].set_ylabel('Acceleration Y')
ax[1, 1].grid()
ax[1, 1].legend()
ax[1, 1].tick_params(axis = 'x', labelrotation = 45)
ax[1, 1].set_xlim(0, max(time))

# fig, ax = plt.subplots(2,2)
# ax[0, 0].plot([1, 2, 4, 8, 16], [1, 2, 3, 4, 5])
# ax[0, 0].title.set_text("plot A")
plt.show()

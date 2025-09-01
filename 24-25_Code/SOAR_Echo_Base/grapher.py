import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
df = pd.read_csv('flight_data.csv')

# Convert time from milliseconds to seconds
df['Time'] = df['Time'] / 1000

# Create a figure with four subplots
fig, ax = plt.subplots(4, 1, figsize=(10, 20))

# Plot altitude
ax[0].plot(df['Time'], df['Altitude'], label='Altitude')
ax[0].set_xlabel('Time (seconds)')
ax[0].set_ylabel('Altitude')
ax[0].legend()
ax[0].set_title('Altitude')

# Plot distance
ax[1].plot(df['Time'], df['Distance'], label='Distance')
ax[1].set_xlabel('Time (seconds)')
ax[1].set_ylabel('Distance')
ax[1].legend()
ax[1].set_title('Distance')

# Plot status with no interpolation
ax[2].step(df['Time'], df['Status'], label='Status', where='post')
ax[2].set_xlabel('Time (seconds)')
ax[2].set_ylabel('Status')
ax[2].legend()
ax[2].set_title('Status')

# Plot AltTrigger State with no interpolation
ax[3].step(df['Time'], df['AltTrigger State'], label='AltTrigger State', where='post')
ax[3].set_xlabel('Time (seconds)')
ax[3].set_ylabel('AltTrigger State')
ax[3].legend()
ax[3].set_title('AltTrigger State')

# Adjust the layout and show the plot
plt.tight_layout()
plt.show()

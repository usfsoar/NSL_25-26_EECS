import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import cumtrapz

# Load the CSV file
df = pd.read_csv('imu_data.csv', sep=',\s+', engine='python')
df['time'] = df['time']/1000


# Calculate the velocity by integrating the linear acceleration
df['velocity_x'] = cumtrapz(df['linear_x'], df['time'], initial=0)
df['velocity_y'] = cumtrapz(df['linear_y'], df['time'], initial=0)
df['velocity_z'] = cumtrapz(df['linear_z'], df['time'], initial=0)


# Calculate the speed
df['speed'] = np.sqrt(df['velocity_x']**2 + df['velocity_y']**2 + df['velocity_z']**2)

# Assume a constant mass for the rocket (e.g., 1 kg)
mass = 18.92

# Calculate the kinetic energy
df['kinetic_energy'] = 0.5 * mass * df['speed']**2

joule_to_ft_lb = 0.737562

# Convert kinetic energy to foot-pounds
df['kinetic_energy_ft_lb'] = df['kinetic_energy'] * joule_to_ft_lb

# Plot the kinetic energy in foot-pounds over time
plt.plot(df['time'], df['kinetic_energy_ft_lb'])
plt.xlabel('Time (s)')
plt.ylabel('Kinetic Energy (ft*lb)')
plt.title('Kinetic Energy of the Rocket Over Time (ft*lb)')
plt.grid(True)
plt.show()


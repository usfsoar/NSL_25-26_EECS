import pandas as pd
import matplotlib.pyplot as plt
from scipy.integrate import cumtrapz
import numpy as np

# Load the CSV file into a DataFrame
file_path = 'imu_data.csv'
df = pd.read_csv(file_path, sep=',\s+', engine='python')
df['time']  = df['time']/1000
df = df[df['time'] >= 1980]
df = df[df['time'] <= 2065]

#manually add gravity to linear_x acceleration
df['raw_accel_x'] =  df['linear_x'] + df['gravity_x']
df['raw_accel_y'] =  df['linear_y'] + df['gravity_y']
df['raw_accel_z'] =  df['linear_z'] + df['gravity_z']

df['mag_raw_accel'] = np.sqrt(df['raw_accel_x']**2 + df['raw_accel_y']**2 + df['raw_accel_z']**2)

# Get when


# Calculate the velocity by integrating the linear acceleration
df['velocity_x'] = cumtrapz(df['gravity_x'], df['time'], initial=0)
df['velocity_y'] = cumtrapz(df['gravity_y'], df['time'], initial=0)
df['velocity_z'] = cumtrapz(df['gravity_z'], df['time'], initial=0)


# Calculate the speed
df['speed'] = np.sqrt(df['velocity_x']**2 + df['velocity_y']**2 + df['velocity_z']**2)

df['mag_accel'] = np.sqrt(df['linear_x']**2 + df['linear_y']**2 + df['linear_z']**2)
df['mag_grav'] = np.sqrt(df['gravity_x']**2 + df['gravity_y']**2 + df['gravity_z']**2)

# Assume a constant mass for the rocket (e.g., 1 kg)
mass = 9.53

# Calculate the kinetic energy
df['kinetic_energy'] = 0.5 * mass * df['speed']**2

joule_to_ft_lb = 0.7375621493

# Convert kinetic energy to foot-pounds
df['kinetic_energy_ft_lb'] = df['kinetic_energy'] * joule_to_ft_lb

# Create a figure with two subplots
fig, axes = plt.subplots(nrows=5, ncols=1, figsize=(12, 12))

# Plot acceleration data in the first subplot
axes[0].plot(df['time'], df['raw_accel_x'], label='Accel X')
axes[0].plot(df['time'], df['raw_accel_y'], label='Accel Y')
axes[0].plot(df['time'], df['raw_accel_z'], label='Accel Z')
axes[0].plot(df['time'], df['mag_raw_accel'], label='Magnitude', linestyle='--')
axes[0].set_title('Raw Acceleration Data')
axes[0].set_xlabel('Time')
axes[0].set_ylabel('Acceleration (m/s^2)')
axes[0].legend()
axes[0].grid(True)

axes[1].plot(df['time'], df['linear_x'], label='Linear X')
axes[1].plot(df['time'], df['linear_y'], label='Linear Y')
axes[1].plot(df['time'], df['linear_z'], label='Linear Z')
axes[1].plot(df['time'], df['mag_accel'], label='Magnitude', linestyle='--')
axes[1].set_title('Linear Acceleration Data')
axes[1].set_xlabel('Time')
axes[2].set_ylabel('Acceleration (m/s^2)')
axes[1].legend()
axes[1].grid(True)

axes[2].plot(df['time'], df['gravity_x'], label='X')
axes[2].plot(df['time'], df['gravity_y'], label='Y')
axes[2].plot(df['time'], df['gravity_z'], label='Z')
axes[2].plot(df['time'], df['mag_grav'], label='Magnitude', linestyle='--')
axes[2].set_title('Gravity Acceleration Data')
axes[2].set_xlabel('Time')
axes[2].set_ylabel('Acceleration (m/s^2)')
axes[2].legend()
axes[2].grid(True)

# Plot quaternion and gyro data in the second subplot
axes[3].plot(df['time'], df['velocity_x'], label='X')
axes[3].plot(df['time'], df['velocity_y'], label='Y')
axes[3].plot(df['time'], df['velocity_z'], label='Z')
axes[3].plot(df['time'], df['speed'], label='Magnitude', linestyle='--')
axes[3].set_title('Linear Speed Data')
axes[3].set_xlabel('Time')
axes[3].set_ylabel('Velocity (m/s)')
axes[3].legend()
axes[3].grid(True)

axes[4].plot(df['time'], df['kinetic_energy_ft_lb'], label='KE (ft*lb)')
axes[4].set_title('KE data (ft * lb)')
axes[4].set_xlabel('Time (s)')
axes[4].set_ylabel('Value')
axes[4].legend()
axes[4].grid(True)

# Adjust the layout and show the plot
plt.tight_layout()
plt.show()

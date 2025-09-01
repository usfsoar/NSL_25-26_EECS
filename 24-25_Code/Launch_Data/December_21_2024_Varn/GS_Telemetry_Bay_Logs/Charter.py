import pandas as pd

import matplotlib.pyplot as plt

# Load the CSV file into a DataFrame
file_path = "Launch_Data/December_21_2024_Varn/GS_Telemetry_Bay_Logs/parsed_evening_Serial_Log.csv"
df = pd.read_csv(file_path)

# Drop rows with missing acceleration data
df = df.dropna(subset=["accel_x", "accel_y", "accel_z"])

# Create a 3D scatter plot
fig = plt.figure()
ax = fig.add_subplot(111, projection="3d")

# Plot the acceleration data
ax.scatter(df["accel_x"], df["accel_y"], df["accel_z"], c="r", marker="o")

# Set labels
ax.set_xlabel("Accel X")
ax.set_ylabel("Accel Y")
ax.set_zlabel("Accel Z")

# Set title
ax.set_title("3D Scatter Plot of Acceleration Data")

# Show the plot
plt.show()

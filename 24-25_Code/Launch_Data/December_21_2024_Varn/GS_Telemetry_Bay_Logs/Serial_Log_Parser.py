import re
import csv
from datetime import datetime

# Define regex patterns
timestamp_pattern = re.compile(r"(\d{2}:\d{2}:\d{2}:\d{3})")
altitude_pattern = re.compile(r"Altitude: (-?\d+\.\d+)m")
temperature_pattern = re.compile(r"(\d+\.\d+)°C")
pressure_pattern = re.compile(r"(\d+\.\d+)kPa")
accel_pattern = re.compile(r"Accel: \[X(-?\d+\.\d+),Y(-?\d+\.\d+),Z(-?\d+\.\d+)\]")
linaccel_pattern = re.compile(
    r"LinAccel: \[X(-?\d+\.\d+),Y(-?\d+\.\d+),Z(-?\d+\.\d+)\]"
)
gravity_pattern = re.compile(r"Gravity: \[X(-?\d+\.\d+),Y(-?\d+\.\d+),Z(-?\d+\.\d+)\]")
quat_pattern = re.compile(
    r"Quat: \[W(-?\d+\.\d+),X(-?\d+\.\d+),Y(-?\d+\.\d+),Z(-?\d+\.\d+)\]"
)
gyro_pattern = re.compile(r"Gyro: \[X(-?\d+\.\d+),Y(-?\d+\.\d+),Z(-?\d+\.\d+)\]")

# Open CSV file for writing
output_file = open(
    "Launch_Data/December_21_2024_Varn/GS_Telemetry_Bay_Logs/parsed_evening_Serial_Log.csv",
    "w",
    newline="",
)

# Create CSV writer
csv_writer = csv.writer(output_file)

# Write header
csv_writer.writerow(
    [
        "timestamp",
        "altitude",
        "temperature",
        "pressure",
        "accel_x",
        "accel_y",
        "accel_z",
        "linaccel_x",
        "linaccel_y",
        "linaccel_z",
        "gravity_x",
        "gravity_y",
        "gravity_z",
        "quat_w",
        "quat_x",
        "quat_y",
        "quat_z",
        "gyro_x",
        "gyro_y",
        "gyro_z",
    ]
)

# Read the log file
with open(
    "Launch_Data/December_21_2024_Varn/GS_Telemetry_Bay_Logs/EVENING_Serial_Log.txt",
    "r",
) as log_file:
    for line in log_file:
        timestamp_match = timestamp_pattern.search(line)
        if timestamp_match:
            timestamp = timestamp_match.group(1)
            timestamp = datetime.strptime(timestamp, "%H:%M:%S:%f").strftime(
                "%H:%M:%S.%f"
            )[:-3]

            row = [timestamp] + [
                None
            ] * 19  # Initialize row with timestamp and None values

            altitude_match = altitude_pattern.search(line)
            if altitude_match:
                row[1] = altitude_match.group(1)

            temperature_match = temperature_pattern.search(line)
            if temperature_match:
                row[2] = temperature_match.group(1)

            pressure_match = pressure_pattern.search(line)
            if pressure_match:
                row[3] = pressure_match.group(1)

            accel_match = accel_pattern.search(line)
            if accel_match:
                row[4] = accel_match.group(1)
                row[5] = accel_match.group(2)
                row[6] = accel_match.group(3)

            linaccel_match = linaccel_pattern.search(line)
            if linaccel_match:
                row[7] = linaccel_match.group(1)
                row[8] = linaccel_match.group(2)
                row[9] = linaccel_match.group(3)

            gravity_match = gravity_pattern.search(line)
            if gravity_match:
                row[10] = gravity_match.group(1)
                row[11] = gravity_match.group(2)
                row[12] = gravity_match.group(3)

            quat_match = quat_pattern.search(line)
            if quat_match:
                row[13] = quat_match.group(1)
                row[14] = quat_match.group(2)
                row[15] = quat_match.group(3)
                row[16] = quat_match.group(4)

            gyro_match = gyro_pattern.search(line)
            if gyro_match:
                row[17] = gyro_match.group(1)
                row[18] = gyro_match.group(2)
                row[19] = gyro_match.group(3)

            # Check if at least one data field is not None
            if any(field is not None for field in row[1:]):
                csv_writer.writerow(row)

# Close CSV file
output_file.close()

import csv
import os
from datetime import datetime
from sensor_data import SensorData  # Import SensorData class

def data_processor(data_object: SensorData, charts: dict):
    """
    Processes SensorData and updates charts.

    Args:
        data_object (SensorData): The SensorData object containing sensor values.
        charts (dict): A dictionary of charts to update, keyed by sensor name.
    """
    # Step 1: Create a .csv file with a timestamp in the name
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    filename = f"sensor_data_{timestamp}.csv"
    
    # Explicitly set the CSV file folder path
    data_folder = r"C:\Users\HP\Downloads\NSL_24-25_EECS\SOAR_ECHO_BASE_V2\ECHO_BASE_ASSEMBLY\UI\CSV file"
    os.makedirs(data_folder, exist_ok=True)

    # Save the file in the CSV folder
    filepath = os.path.join(data_folder, filename)

    # Write data to the .csv file
    with open(filepath, mode="w", newline="") as csvfile:
        csv_writer = csv.writer(csvfile)
        # Write the header row
        csv_writer.writerow([
            "Sender ID", "Receiver ID", "Time (H:M:S.MS)",
            "Acceleration X", "Acceleration Y", "Acceleration Z",
            "Linear Acceleration X", "Linear Acceleration Y", "Linear Acceleration Z",
            "Gravity X", "Gravity Y", "Gravity Z",
            "Quaternion W", "Quaternion X", "Quaternion Y", "Quaternion Z",
            "Gyro X", "Gyro Y", "Gyro Z",
            "Altitude", "Temperature", "Pressure",
            "Status", "Latitude", "N/S", "Longitude", "E/W"
        ])
        # Write the data row
        csv_writer.writerow([
            data_object.sender_id, data_object.rx_id,
            f"{data_object.hours}:{data_object.minutes}:{data_object.seconds}.{data_object.microseconds}",
            data_object.accel_x, data_object.accel_y, data_object.accel_z,
            data_object.linear_x, data_object.linear_y, data_object.linear_z,
            data_object.gravity_x, data_object.gravity_y, data_object.gravity_z,
            data_object.quat_w, data_object.quat_x, data_object.quat_y, data_object.quat_z,
            data_object.gyro_x, data_object.gyro_y, data_object.gyro_z,
            data_object.altitude, data_object.temp, data_object.pressure,
            data_object.status, data_object.lat, data_object.n_s, data_object.long, data_object.e_w
        ])

    print(f"Data saved to {filepath}")

    # Step 2: Update charts with the respective data values
    try:
        if "Acceleration" in charts:
            charts["Acceleration"].update_chart(
                [data_object.accel_x], [data_object.accel_y]
            )
        if "Temperature" in charts:
            charts["Temperature"].update_chart(
                [datetime.now().timestamp()], [data_object.temp]
            )
        if "Pressure" in charts:
            charts["Pressure"].update_chart(
                [datetime.now().timestamp()], [data_object.pressure]
            )
        if "Altitude" in charts:
            charts["Altitude"].update_chart(
                [datetime.now().timestamp()], [data_object.altitude]
            )
        print("Charts updated successfully.")
    except KeyError as e:
        print(f"Chart for {e} is not defined. Ensure chart instances are passed in the dictionary.")
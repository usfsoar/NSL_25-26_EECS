import pandas as pd
import os

class SDCard:
    def __init__(self, file_path):
        self.file_path = file_path

    def make_sd(self, data):
        """Creates a CSV file with headers from the dictionary keys and initializes it with the data dictionary."""
        try:
            # Check if the file already exists
            if not os.path.exists(self.file_path):
                # Initialize a DataFrame with empty lists
                df = pd.DataFrame(data)  # Pandas automatically uses dictionary keys as column headers
                df.to_csv(self.file_path, index=False)  # Write the DataFrame to CSV
                print(f"CSV file '{self.file_path}' created and initialized successfully.")
            else:
                print(f"CSV file '{self.file_path}' already exists.")
        except:
            print(f"Error creating CSV file")

    def update_sd(self, data):
        """Appends a new row of data to the CSV file from a dictionary."""
        try:
            # Convert the dictionary to a DataFrame
            new_data = pd.DataFrame([data])  # Wrap the data in a list to create a row in DataFrame
            
            # Append the new data to the existing CSV file
            new_data.to_csv(self.file_path, mode='a', header=False, index=False)  # Append without headers
            print(f"Data appended to '{self.file_path}' successfully.")
        except:
            print(f"Error updating CSV file")

# # Example usage of the SDCard class
def main():
    # Define the file path where the CSV file will be stored
    file_path = '/media/pi/sdcard/data.csv'
    
    # Instantiate the SDCard class
    sd_card = SDCard(file_path)

    # Define the data with empty lists for initialization
    data = {
        'Time': [],
        'Altitude': [],
        'Velocity': [],
        'Acceleration': [],
        'State': [],
        'Servo Angle': []
    }

    # Create the CSV file (if it doesn't exist) and initialize with empty data
    sd_card.make_sd(data)

    # Example data to be added (from your dictionary structure)
    new_data = {
        'Time': 120,  # Elapsed time in seconds
        'Altitude': 1500,  # Altitude in meters
        'Velocity': 35,  # Velocity in m/s
        'Acceleration': 9.8,  # Acceleration in m/s²
        'State': 'In flight',  # Current state (e.g., 'In flight', 'Landing')
        'Servo Angle': 45  # Servo angle in degrees
    }

    # Update the CSV file with the new data
    sd_card.update_sd(new_data)

# Run the main function
if __name__ == "__main__":
    main()

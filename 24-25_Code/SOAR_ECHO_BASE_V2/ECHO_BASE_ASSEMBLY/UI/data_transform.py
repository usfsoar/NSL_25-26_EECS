def process_hex_input(hex_input: str) -> str:
    """
    Processes an input string and extracts data after the FFFE42 marker.
    Decodes hex ASCII characters that follow the FFFE42 marker.
    
    Args:
        hex_input (str): The input string that may contain the FFFE42 marker.
        
    Returns:
        str: The decoded ASCII string starting after FFFE42.
    """
    try:
        # Pre-process the input to handle various hex formats:
        # - Remove '0x' prefixes
        # - Remove spaces between hex values
        cleaned_input = ""
        i = 0
        while i < len(hex_input):
            # Skip '0x' prefixes
            if i + 1 < len(hex_input) and hex_input[i:i+2].lower() == '0x':
                i += 2
                continue
            # Skip spaces and other non-hex characters
            elif hex_input[i].isspace() or hex_input[i] in ['x', 'X']:
                i += 1
                continue
            else:
                # Only add if it's a valid hex character
                if hex_input[i].lower() in '0123456789abcdef':
                    cleaned_input += hex_input[i]
                i += 1
        print(f"Cleaned input: {cleaned_input[:150]}...")
        # First try looking for FFFE42 in the cleaned input (case insensitive)
        marker = "FFFE42"
        marker_index = cleaned_input.upper().find(marker)
     
        if marker_index == -1:
            return "Error: Sequence FFFE42 not found."
        
        # Extract the data after FFFE42
        data_after_marker = cleaned_input[marker_index + 6:]
        
        # The data is in hex format representing ASCII values
        # For example "32" is ASCII for "2", "2C" is ASCII for ","
        decoded_string = ""
        i = 0
        
        # Process two characters at a time as hex values
        while i < len(data_after_marker) - 1:
            hex_char = data_after_marker[i:i+2]
            # Check if this is a valid hex character
            try:
                # Convert from hex to ASCII
                ascii_char = bytes.fromhex(hex_char).decode('ascii')
                decoded_string += ascii_char
                i += 2
            except (ValueError, UnicodeDecodeError):
                # If not valid hex, move one character forward
                i += 1
        
        return decoded_string
                
    except Exception as e:
        return f"Unexpected error: {e}"

def identify_string_type(decoded_string: str) -> str:
    """
    Identifies whether the string is IMU, GPS, or BMP data based on
    the pattern of dashes and data.
    
    Args:
        decoded_string (str): The decoded string.
        
    Returns:
        str: The type of data ('IMU', 'GPS', or 'BMP').
    """
    parts = decoded_string.split(',')
    
    # Check for GPS pattern - contains coordinates with N/W/E/S
    has_coordinates = any(('N' in part or 'S' in part or 'E' in part or 'W' in part) for part in parts)
    
    # Check for pressure/temperature pattern
    has_pressure = False
    for i in range(len(parts) - 1):
        # Look for numeric values that could be temp/pressure readings after a series of dashes
        if parts[i].startswith('-') and parts[i].strip() == '-' and i+1 < len(parts) and parts[i+1].replace('.', '', 1).isdigit():
            has_pressure = True
            break
    
    # Identify based on patterns and dash positions
    if has_coordinates:
        return "GPS"
    elif has_pressure or any("pressure" in p.lower() for p in parts) or any("temp" in p.lower() for p in parts):
        return "BMP"  # BMP contains pressure/temperature data
    else:
        # Count dashes at the end
        end_dashes = 0
        for part in reversed(parts):
            if part.strip() == '-':
                end_dashes += 1
            else:
                break
                
        # IMU typically has several dashes at the end
        if end_dashes >= 4:  # Threshold can be adjusted
            return "IMU"
        
        # If unsure, check if it looks like IMU data (many floating point values)
        float_count = sum(1 for p in parts if p.replace('.', '', 1).replace('-', '', 1).isdigit())
        
        if float_count > 10:  # Threshold can be adjusted
            return "IMU"
        else:
            return "Unknown"
        

def prepare_gps_input(decoded_string: str) -> str:
        # Split the decoded string by commas
        parts = decoded_string.split(",")
        
        # Check if this is likely GPS data
        if not any(('N' in part or 'S' in part or 'E' in part or 'W' in part) for part in parts):
            return "Not GPS data"
            
        # Find the coordinates
        lat_index = -1
        lon_index = -1
        
        for i, part in enumerate(parts):
            if 'N' in part or 'S' in part:
                lat_index = i
            if 'E' in part or 'W' in part:
                lon_index = i
                
        if lat_index == -1 or lon_index == -1:
            return "Coordinates not found in GPS data"
            
        # Extract rocket time (parts 0-3)
        rocket_time = parts[0:4]
        
        # Extract relevant GPS data
        lat = parts[lat_index-1] + "," + parts[lat_index]
        lon = parts[lon_index-1] + "," + parts[lon_index]
        speed = parts[lon_index+1] if lon_index+1 < len(parts) else ""
        
        process_lat = lat.split(",")
        lat_num = process_lat[0]
        lat_dd = process_lat[0:2]
        lat_mm = lat_num[::2]
        lat_mm = lat_dd[2:]

        lon_dd = lon[::-1].replace("E", "").replace("W", "")


        # Format as $GNRMC input with coordinates
        return f"$GNRMC,{','.join(rocket_time)},A,{lat},{lon},{speed}"
        # Example: $GNRMC,2,4,19,14,A,3446370,N,08641.1597,W,
        # rocket_time = "2,4,19,14"
        # lat = "3446.3703,N"
        # lon = "08641.1597,W"
        # speed = "0.0" (or whatever is in the string)
        # DDMM.MMMM
        # DD.MMMMMM

        # except Exception as e:
        # return f"Error preparing GPS input: {e}"


def process_bmp_data(decoded_string: str) -> dict:
    """
    Processes BMP data to extract altitude, temperature, and pressure values.
    Returns a dictionary with the extracted values.

    Args:
        decoded_string (str): The decoded string containing BMP data.

    Returns:
        dict: A dictionary with keys 'altitude', 'temperature', and 'pressure'.
    """
    try:
        parts = decoded_string.split(',')
        altitude = None
        temperature = None
        pressure = None

        # Iterate through parts to find altitude, temperature, and pressure
        for i, part in enumerate(parts):
            if part.replace('.', '', 1).replace('-', '', 1).isdigit():
                try:
                    val = float(part)
                    # Typical altitude range (in meters)
                    if 0 <= val <= 10000 and altitude is None:
                        altitude = val
                    # Typical temperature range (in Celsius)
                    elif -50 <= val <= 50 and temperature is None:
                        temperature = val
                    # Typical pressure range (in Pa)
                    elif 90000 <= val <= 120000 and pressure is None:
                        pressure = val
                except ValueError:
                    pass

        return {
            "altitude": altitude,
            "temperature": temperature,
            "pressure": pressure
        }
    except Exception as e:
        return {"error": str(e)}
    
def process_imu_data(response_ascii):
    """
    Processes IMU data from the ASCII response string.

    Args:
        response_ascii (str): The ASCII string containing IMU data.

    Returns:
        dict: A dictionary containing processed IMU data, including acceleration, gyroscope, and other metrics.
    """
    try:
        # Split the response by commas
        data_fields = response_ascii.split(",")
        
        # Initialize an empty dictionary to store all IMU data
        imu_data = {}
        
        # Index tracking to handle variable data formats
        current_idx = 0
        field_count = len(data_fields)
        
        # Process accelerometer data (usually first three values)
        if current_idx + 2 < field_count:
            try:
                imu_data["accel_x"] = float(data_fields[current_idx])
                imu_data["accel_y"] = float(data_fields[current_idx + 1])
                imu_data["accel_z"] = float(data_fields[current_idx + 2])
                current_idx += 3
            except ValueError:
                print("Error parsing accelerometer data")
        
        # Process gyroscope data (next three values)
        if current_idx + 2 < field_count:
            try:
                imu_data["gyro_x"] = float(data_fields[current_idx])
                imu_data["gyro_y"] = float(data_fields[current_idx + 1])
                imu_data["gyro_z"] = float(data_fields[current_idx + 2])
                current_idx += 3
            except ValueError:
                print("Error parsing gyroscope data")
        
        # Process linear acceleration data if available
        if current_idx + 2 < field_count:
            try:
                imu_data["linear_x"] = float(data_fields[current_idx])
                imu_data["linear_y"] = float(data_fields[current_idx + 1])
                imu_data["linear_z"] = float(data_fields[current_idx + 2])
                current_idx += 3
            except ValueError:
                print("Error parsing linear acceleration data")
        
        # Process gravity data if available
        if current_idx + 2 < field_count:
            try:
                imu_data["gravity_x"] = float(data_fields[current_idx])
                imu_data["gravity_y"] = float(data_fields[current_idx + 1])
                imu_data["gravity_z"] = float(data_fields[current_idx + 2])
                current_idx += 3
            except ValueError:
                print("Error parsing gravity data")
        
        # Process quaternion data if available
        if current_idx + 3 < field_count:
            try:
                imu_data["quat_w"] = float(data_fields[current_idx])
                imu_data["quat_x"] = float(data_fields[current_idx + 1])
                imu_data["quat_y"] = float(data_fields[current_idx + 2])
                imu_data["quat_z"] = float(data_fields[current_idx + 3])
                current_idx += 4
            except ValueError:
                print("Error parsing quaternion data")
        
        # Print debug info
        print(f"Processed IMU data with {len(imu_data)} fields: {imu_data}")
        return imu_data
    
    except Exception as e:
        print(f"Error processing IMU data: {e}")
        return {}

def main():
    """
    Main function to process input strings and identify their types.
    Allows user to input strings interactively.
    """
    print("Flight Data Decoder\n")
    print("Enter a telemetry string to decode, or 'exit' to quit")
    
    while True:
        user_input = input("\nEnter raw input: ")
        
        if user_input.lower() == 'exit' or user_input.lower() == 'quit':
            print("Exiting program. Goodbye!")
            break
            
        decoded_string = process_hex_input(user_input)
        
        if decoded_string.startswith("Error:"):
            print(f"Decode error: {decoded_string}")
            continue
            
        print(f"\nDecoded string: {decoded_string}")
        
        # Identify the type of string
        string_type = identify_string_type(decoded_string)
        print(f"Identified as: {string_type} data")
        
        # Process based on type
        if string_type == "GPS":
            gps_format = prepare_gps_input(decoded_string)
            print(f"GPS Format: {gps_format}")
            print("\nYou can now copy this $GNRMC sentence and use it as input in GPS_Screen_Updater.py")
            
        elif string_type == "BMP":
            bmp_data = process_bmp_data(decoded_string)
            print(f"BMP Data: {bmp_data}")
        elif string_type == "IMU":
            print("IMU data detected - contains accelerometer/gyroscope readings")
        else:
            print("Unknown data format")

# Run the program when this script is executed directly
if __name__ == "__main__":
    main()
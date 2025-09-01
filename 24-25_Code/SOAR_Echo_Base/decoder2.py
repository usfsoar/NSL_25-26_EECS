import struct

# Given byte data
byte_data = b'IS>\x18\x9bA+\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00)\xb6\x1b\x00'

def decode_data(byte_data):
    # Check if the first two bytes are "IS"
    if byte_data.startswith(b'IS'):
        # Extract the float (altitude) and the two integers (distance and status)
        altitude = struct.unpack('f', byte_data[2:6])[0]
        distance = struct.unpack('i', byte_data[6:10])[0]
        status = struct.unpack('i', byte_data[10:14])[0]
        altTrigger_state = struct.unpack('i', byte_data[14:18])[0]
        # Last 4 bytes are time as a long
        milliseconds = struct.unpack('l', byte_data[18:22])[0]
        # Print the decoded values
        return milliseconds, altitude, distance, status, altTrigger_state
    else:
        print("Invalid data format")
        return None, None, None, None, None

output_file = open("flight_data.csv", "w")
output_file.write("Time,Altitude,Distance,Status,AltTrigger State\n")
with open("decoded_strings.csv", "r") as file:
    for line in file:
        try:
            # byte_data = bytes.fromhex(line.strip())
            byte_data = bytes(line, 'utf-8').decode('unicode_escape').encode('latin1')
            time, alt, dist, stat, altStat = decode_data(byte_data)
            if time is not None and alt is not None and dist is not None and stat is not None and altStat is not None:
                output_file.write(f"{time},{alt},{dist},{stat},{altStat}\n")           
                print(f"Time: {time}, Altitude: {alt}, Distance: {dist}, Status: {stat}, altTrigger State: {altStat}")
        except Exception as e:
            print(f"Error: {e}\nLine:{line}")

output_file.close()
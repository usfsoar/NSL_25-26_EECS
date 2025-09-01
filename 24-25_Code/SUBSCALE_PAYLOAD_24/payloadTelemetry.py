# Import necessary modules
import socket       # For establishing a TCP socket connection
import time         # For time delays
import subprocess   # For running external processes (i.e., Direwolf)
import RPi.GPIO as GPIO  # For controlling Raspberry Pi GPIO pins

# Paths for Direwolf executable and configuration file
direwolf_path = r'/usr/local/bin/direwolf'  # Path to Direwolf binary
config_path = r'/home/soar/Direwolf/direwolf/build/direwolf.conf'  # Path to Direwolf config file

# Start the Direwolf process using the specified configuration file
direwolf_process = subprocess.Popen([direwolf_path, '-c', config_path], shell=True)



# KISS protocol special characters
KISS_FEND = 0xC0    # Frame delimiter (start and end of a KISS frame)
KISS_FESC = 0xDB    # Escape character
KISS_TFEND = 0xDC   # Translated FEND (if FEND appears in data, it gets escaped)
KISS_TFESC = 0xDD   # Translated FESC (if FESC appears in data, it gets escaped)
KISS_CMD_DATA = 0x00  # Command byte for sending data

def kiss_encode(data):
    """
    Encodes a payload in KISS format.

    This function wraps the provided data in KISS protocol format,
    adding the necessary frame delimiters and escaping special characters.

    Parameters:
    - data (bytes): The data payload to be encoded.

    Returns:
    - bytearray: The KISS-encoded frame.
    """
    kiss_frame = bytearray([KISS_FEND, KISS_CMD_DATA])  # Start with FEND and command byte
    for byte in data:
        if byte == KISS_FEND:
            # Escape FEND character within data
            kiss_frame.extend([KISS_FESC, KISS_TFEND])
        elif byte == KISS_FESC:
            # Escape FESC character within data
            kiss_frame.extend([KISS_FESC, KISS_TFESC])
        else:
            # Regular byte, no escaping needed
            kiss_frame.append(byte)
    kiss_frame.append(KISS_FEND)  # Add FEND at the end of the frame
    return kiss_frame

def ax25_encode_call(call, ssid, last=False):
    """
    Encodes a callsign with SSID for AX.25 protocol.

    The callsign is padded to 6 characters, and each character is shifted left by 1 bit
    as required by the AX.25 specification. The SSID is also encoded with specific bits.

    Parameters:
    - call (str): The callsign (max 6 characters).
    - ssid (int): The SSID number (0-15).
    - last (bool): Indicates if this is the last address field (set the "last" bit).

    Returns:
    - bytearray: Encoded callsign and SSID.
    """
    call = call.ljust(6)  # Ensure the callsign is exactly 6 characters by padding with spaces
    encoded_call = bytearray((ord(c) << 1) for c in call)  # Shift each character left by 1 bit
    ssid_byte = (ssid & 0x0F) << 1 | 0x60  # Set the SSID in lower nibble, add control bits
    if last:
        ssid_byte |= 0x01  # Set "last" bit if this is the final address field
    encoded_call.append(ssid_byte)
    return encoded_call

def create_ax25_frame(source, source_ssid, destination, destination_ssid, data):
    """
    Creates an AX.25 frame with source, destination, and data.

    Parameters:
    - source (str): Source callsign.
    - source_ssid (int): SSID for the source callsign.
    - destination (str): Destination callsign.
    - destination_ssid (int): SSID for the destination callsign.
    - data (str): The payload (APRS message).

    Returns:
    - bytearray: Complete AX.25 frame with headers and data.
    """
    ax25_frame = bytearray()
    # Encode destination callsign and SSID
    ax25_frame.extend(ax25_encode_call(destination, destination_ssid))
    # Encode source callsign and SSID (last=True since it's the last address field)
    ax25_frame.extend(ax25_encode_call(source, source_ssid, last=True))
    ax25_frame.append(0x03)  # Control field: UI frame (Unnumbered Information)
    ax25_frame.append(0xF0)  # PID field: No Layer 3 protocol
    # Append the payload data (e.g., APRS message)
    ax25_frame.extend(data.encode("ascii"))
    return ax25_frame

def send_aprs_packet(aprs_data, source, source_ssid, destination, destination_ssid, tcp_ip="127.0.0.1", tcp_port=8001):
    """
    Encodes APRS data in AX.25, wraps in KISS format, and sends it to Direwolf via TCP.

    This function also activates a GPIO pin on the Raspberry Pi before sending.

    Parameters:
    - aprs_data (str): The APRS message to be sent.
    - source (str): Source callsign.
    - source_ssid (int): SSID for the source callsign.
    - destination (str): Destination callsign.
    - destination_ssid (int): SSID for the destination callsign.
    - tcp_ip (str): IP address for the Direwolf server (default: 127.0.0.1).
    - tcp_port (int): TCP port for the Direwolf server (default: 8001).
    """
    # Create the AX.25 frame with the given data
    ax25_frame = create_ax25_frame(source, source_ssid, destination, destination_ssid, aprs_data)
    # Wrap the AX.25 frame in KISS encoding
    kiss_frame = kiss_encode(ax25_frame)


    # Configure Raspberry Pi GPIO
    GPIO.setmode(GPIO.BCM)  # Set GPIO mode to Broadcom pin numbering
    pin = 17                # Pin number to control (GPIO 17)
    GPIO.setup(pin, GPIO.OUT)  # Set GPIO pin as an output

    # Activate GPIO pin to signal packet transmission
    GPIO.output(pin, GPIO.HIGH)

    # Send the KISS frame to Direwolf over TCP
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((tcp_ip, tcp_port))
        s.sendall(kiss_frame)  # Send the KISS frame
    time.sleep(15)  # Wait for 15 seconds (to ensure packet transmission completes)
    GPIO.cleanup()  # Clean up GPIO pins after use

# APRS packet details
aprs_data = "!The message works. This is cool"  # Example APRS payload
source = "KQ4FYU"   # Source callsign
source_ssid = 1      # Source SSID
destination = "APRS" # Destination callsign
destination_ssid = 0 # Destination SSID

# Continuously send APRS packets every 10 seconds
for i in range(10):
    time.sleep(10)
    send_aprs_packet(aprs_data, source, source_ssid, destination, destination_ssid)

# Wait for the Direwolf process to finish (blocks indefinitely)
direwolf_process.wait()
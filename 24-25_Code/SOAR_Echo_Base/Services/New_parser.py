import re
from Controllers import gps_controller, control_panel


def parse_send_gps(message):
    raw_string = match.group(1)
    # Check to see if this is the Rocket or the Payload
    address = raw_string[5:6]
    section = False
    # Address 7 is the Rocket Side GPS/RRC3 Altimeter
    # Address 5 is the Payload
    if int(address) == 7:
        section = False
    if int(address) == 5:
        section = True

    # Gets the NMEA data
    pattern = r"GPS:\s*(.+)"
    match = re.search(pattern, raw_string)
    nmea_data = match.group(1)
    gps_controller.update_gps(section, nmea_data)


    # Relay the message as something to be logged
    gps_controller.log_msg(message)


def relay_message(message):
    try:
        pattern = r"<LORA>(.*?)<\/LORA>"
        match = re.search(pattern, message)
        # global section
        if "GPS" in message:
            raw_string = match.group(1)
            # Check to see if this is the Rocket or the Payload
            address = raw_string[5:6]
            section = False
            # Address 7 is the Rocket Side GPS/RRC3 Altimeter
            # Address 5 is the Payload
            if int(address) == 7:
                section = False
            if int(address) == 5:
                section = True

            # Gets the NMEA data
            pattern = r"GPS:\s*(.+)"
            match = re.search(pattern, raw_string)
            nmea_data = match.group(1)
            gps_controller.update_gps(section, nmea_data)

            # Relay the message as something to be logged
            gps_controller.log_msg(message)

        elif "ALTITUDE" in message:  # Changed from "ALTI" to "ALTITUDE"
            control_panel.update_alti(
                message
            )  # Changed from update_alti to update_altitude
            control_panel.log_msg(message)
            print("Received altitude message")
        elif "IMU" in message:
            control_panel.update_imu(message)
            control_panel.log_msg(message)
            print("Received IMU message")
        elif "BAROMETER" in message:
            control_panel.update_barometer(message)
            control_panel.log_msg(message)
            print("Received barometer message")
        elif "DECIBEL" in message:
            control_panel.update_decibel(message)
            control_panel.log_msg(message)
            print("Received decibel message")
    except Exception as e:
        print(f"Error relaying serial message: {e}")

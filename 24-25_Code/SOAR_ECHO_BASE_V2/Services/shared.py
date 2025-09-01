from Config import *
import re
from Models import SerialMessage
recovery_serial = None

def recovery_log_handler(device):
    message = ""
    if device.message_queue.qsize() > 0:
        message = device.message_queue.get()
        if(message!="" or message!="\n" or message!="\r" or message!="\r\n"):
            serialMsg = SerialMessage(device.name, message)
            socketio.emit('serial_message', serialMsg.to_json())
    return message

def recovery_handler(device):
    while True:
        if not device.device.is_open:
            break
        message = recovery_log_handler(device)
        if message =="":
            continue

        pattern = r"GPS:\s*(.+)"
        match = re.search(pattern, message)
        if match:
            nmea_data = match.group(1)
            socketio.emit("gps_update", {"section": False, "nmea": nmea_data})
import time
import os
from Config import *
from Services.serial_device import SerialDevice
from flask import Flask, render_template, jsonify, jsonify, request, send_from_directory
from threading import Thread
import json
from Models import LogMessage, SerialMessage, LoraMessage
import re
import struct
from Services.shared import recovery_serial, recovery_handler

payloadSender = None
# recoverySender = None

addresses = {
    10: "Drone",
    5: "Deployment",
    7: "Recovery"
}

@app.route('/')
def control_panel():
    # render html under Views/control_panel.html
    directory = os.path.join(app.root_path,'..', 'Views', 'control_panel')
    return send_from_directory(directory, 'control_panel.html')

@app.route('/start_payload_serial/<port>')
def start_serial(port = "COM7"):
    global payloadSender
    try:
        payloadSender = SerialDevice("Payload")
        payloadSender.connect(port)
        print(f'Stablishing serial {port}')
        lora_telem_thread = Thread(target=payloadSender.receive_data)
        lora_telem_thread.daemon = True
        lora_telem_thread.start()
        time.sleep(0.5)
        message_handler_thread = Thread(target=payload_message_handler, args=(payloadSender,))
        message_handler_thread.daemon = True
        message_handler_thread.start()
        return jsonify(message = "Started Payload Serial"), 200
    except Exception as e:
        logMsg = LogMessage(f'Error connecting to Payload Serial: {e}', "error")
        # use the json method to convert the object to a json string
        socketio.emit('log_message', logMsg.to_json())
        print(logMsg.message)
        return jsonify(logMsg.message), 500

@app.route('/start_recovery_serial/<port>')
def start_recovery_serial(port = "COM7"):
    global recovery_serial
    try:
        recovery_serial = SerialDevice("Recovery")
        recovery_serial.connect(port)
        recovery_telem_thread = Thread(target=recovery_serial.receive_data)
        recovery_telem_thread.daemon = True
        recovery_telem_thread.start()
        time.sleep(0.5)
        recovery_handler_thread = Thread(target=recovery_handler, args=(recovery_serial,))
        recovery_handler_thread.daemon = True
        recovery_handler_thread.start()
        return jsonify(message="Started Recovery Serial"), 200
    except Exception as e:
        logMsg = LogMessage(f'Error connecting to Recovery Serial: {e}', "error")
        socketio.emit('log_message', logMsg.to_json)
        print (logMsg.message)
        return jsonify(logMsg.message), 500
def log_message(message, type = "info"):
    logMsg = LogMessage(message, type)
    socketio.emit('log_message', logMsg.to_json())
    print(logMsg.message)

def log_handler(device):
    message = ""
    lora_packet = None
    if device.message_queue.qsize() > 0:
        message = device.message_queue.get()
        if(message!="" or message!="\n" or message!="\r" or message!="\r\n"):
            serialMsg = SerialMessage(device.name, message)
            socketio.emit('serial_message', serialMsg.to_json())
    if device.lora_packet_queue.qsize() > 0:
        lora_packet = device.lora_packet_queue.get()
        # the lora packet is of type LoraPacket send it as json
        loraMsg = LoraMessage(device.name, lora_packet)
        socketio.emit('lora_message', loraMsg.to_json())
    return message, lora_packet

# function to check if byte array are equal to a string
def compBytesStr(bytes, str_val, start = 0):
    #if the string is longer than the bytes at the start index then return false
    if len(str_val) > len(bytes) - start:
        return False
    #iterate through the string and compare each byte to the string starting at the start index
    for i in range(len(str_val)):
        if bytes[i + start] != ord(str_val[i]):
            return False
    return True
    
    

def payload_message_handler(device):
    # Constantly check for the queue on assigned serial device
    # If serial device closes, break the loop
    while True:
        if not device.device.is_open:
            break
        message, lora_packet  = log_handler(device)
        continue
        if lora_packet == None:
            continue
        # proceed to process the LoraPacket Response
        '''
        |Command|Definition|Response|Definition|
|---|---|---|---|
|`PI`|Ping|`PO{T-time}`|Pong with time|
|`AS`|Altitude Single|`AS{T-time}{Altitude-float}`|Altitude Single Data with time|
|`AR`|Altitude Repeat|`AR{T-time}`|Altitude Repeat Received with time|
|`AW`|Write altitude thresholds|`AW{T-time}`|Thresholds written with time|
|||`TW{T-time}:F`|Failed to set thresholds with time|
|`AT`|Get altitude thresholds|`AT{T-time}{H1-float}{H2-float}{H3-float}`|Thresholds data with time|
|`DP`|Deploy|`DP{T-time}`|Deploy Received with time|
|`DS`|Deploy Status|`DS{T-time}{Status-uint8_t}`|Deploy Status Data with time|
|`DR`|Deploy Status Repeat|`DR{T-time}:R`|Deploy Status Repeat Received with time|
|`DT`|Deploy Stop|`DT{T-time}`|Deploy Stop Received with time|
|`DR`|Deploy Reset|`DR{T-time}`|Deploy Reset Received with time|
|`DC`|Deploy Retract|`DC{T-time}`|Deploy Retract Received with time|
|`LI`|Distance sensor|`LI{T-time}{Distance-uint16_t}`|Distance sensor data with time|
|`LR`|Distance sensor repeat|`LR{T-time}`|Distance sensor repeat received with time|
|`IS`|All info single|`IS{T-time}{Altitude-float}{Distance-uint16_t}{Status-uint8_t}`|All info data with time|
|`IR`|All info repeat|`IR{T-time}`|All info repeat received with time|
|`JF`|Jog Forward|`JF{T-time}`|Jog Forward Received with time|
|`JR`|Jog Reverse|`JR{T-time}`|Jog Reverse Received with time|
|`RS`|Stop any repeating data|`RS{T-time}`|Stop any repeating data received with time|
|`NH`|Not handled (n bytes)|`NH{T-time}{Command-nB}`|Not handled command with time|
        '''
        if not lora_packet.valid_data:
            # TODO: Handle invalid data, maybe predict what it was supposed to be
            return
        data_bytes = lora_packet.data_bytes
        sender = addresses[lora_packet.address]
        length = lora_packet.length
        
        if length - 6 > 2: #if the format is valid (4 bytes for time, 2 bytes for checksum)
            time = int.from_bytes(data_bytes[:4], byteorder='big')
            cmd_bytes = bytearray() # the first 2 bytes are the command
            cmd_bytes.extend(data_bytes[0])
            cmd_bytes.extend(data_bytes[1])
            cmd = cmd_bytes.decode('utf-8')
            if cmd == "PO":
                socketio.emit('')

        #Handle the different responses
        # #if the first 2 data bytes are "PO" then it is a pong
        # if compBytesStr(data_bytes, "PO"):
        #     print(f"Pong from {sender}")
        # elif compBytesStr(data_bytes, "AS"):
        #     altitude = int.from_bytes(data_bytes[2:], byteorder='big')
        #     print(f"Altitude from {sender}: {altitude}")
        # elif compBytesStr(data_bytes, "AR"):
        #     altitude = int.from_bytes(data_bytes[2:], byteorder='big')
        #     print(f"Altitude from {sender}: {altitude}")
        # elif compBytesStr(data_bytes, "AWTR:R"):
        #     print(f"Thresholds written to {sender}")
        # elif compBytesStr(data_bytes, "AWTR:F"):
        #     print(f"Failed to write thresholds to {sender}")
        # elif compBytesStr(data_bytes, "AT"):
        #     h1 = int.from_bytes(data_bytes[2:6], byteorder='big')
        #     h2 = int.from_bytes(data_bytes[6:10], byteorder='big')
        #     h3 = int.from_bytes(data_bytes[10:14], byteorder='big')
        #     print(f"Thresholds from {sender}: {h1}, {h2}, {h3}")
        # elif compBytesStr(data_bytes, "DPLY:R"):
        #     print(f"Deploy received from {sender}")
        # elif compBytesStr(data_bytes, "DS"):
        #     status = int.from_bytes(data_bytes[2:], byteorder='big')
        #     print(f"Deploy Status from {sender}: {status}")
        # elif compBytesStr(data_bytes, "DSTP:R"):
        #     print(f"Deploy Stop received from {sender}")
        # elif compBytesStr(data_bytes, "DRST:R"):
        #     print(f"Deploy Reset received from {sender}")
        # elif compBytesStr(data_bytes, "DRTC:R"):
        #     print(f"Deploy Retract received from {sender}")
        # elif compBytesStr(data_bytes, "LI"):
        #     distance = int.from_bytes(data_bytes[2:], byteorder='big')
        #     print(f"Distance from {sender}: {distance}")
        # elif compBytesStr(data_bytes, "IS"):
        #     altitude = int.from_bytes(data_bytes[2:6], byteorder='big')
        #     distance = int.from_bytes(data_bytes[6:8], byteorder='big')
        #     status = int.from_bytes(data_bytes[8:], byteorder='big')
        #     print(f"All info from {sender}: {altitude}, {distance}, {status}")
        # elif compBytesStr(data_bytes, "IR:R"):
        #     print(f"All info repeat received from {sender}")
        # elif compBytesStr(data_bytes, "IR"):
        #     #all info but repeating
        #     altitude = int.from_bytes(data_bytes[2:6], byteorder='big')
        #     distance = int.from_bytes(data_bytes[6:8], byteorder='big')
        #     status = int.from_bytes(data_bytes[8:], byteorder='big')
        #     print(f"Repeating info from {sender}: {altitude}, {distance}, {status}")

        # elif compBytesStr(data_bytes, "JFWD:R"):
        #     print(f"Jog Forward received from {sender}")
        # elif compBytesStr(data_bytes, "JREV:R"):
        #     print(f"Jog Reverse received from {sender}")

@app.route('/close_serial/<device_name>')
def close_serial(device_name):
    try:
        if device_name == "payload":
            payloadSender.close()
        elif device_name == "recovery":
            recovery_serial.close()

    except Exception as e:
        msg = f"Unable to Close Serial Connection: {e}"
        print(msg)
        return jsonify(message = msg), 500
    return jsonify(message='OK'),200

# Route for sending a command to a serial device
# The command will come as a json object with the following format:
# {
#   "device_name": "payload" or "recovery",
#   "destination_address": int,
#   "command": [{data, type}, {data, type}, ...]
# }
# The data will be of different types
@app.route('/send_command', methods=['POST'])
def send_command():
    content_type = request.headers.get('Content-Type')
    if (content_type == 'application/json'):
        json = request.json
        device_name = json['device_name']
        command = json['command']
        address = int(json['destination_address'])
        # The command is a bunch of data of different types
        # To bulid the command we need to convert the data to bytes
        # The types are: string, int, float, bool
        # The command will be sent as a byte array

        command_bytes = bytearray()
        for data in command:
            if data['type'] == 'string':
                command_bytes.extend(data['data'].encode('utf-8'))
            elif data['type'] == 'int':
                command_bytes.extend(data['data'].to_bytes(4, byteorder='big'))
            elif data['type'] == 'float':
                # convert the float to a byte array
                command_bytes.extend(bytearray(struct.pack(">f", data['data'])))
            elif data['type'] == 'bool':
                command_bytes.extend(data['data'].to_bytes(1, byteorder='big'))
            elif data['type'] == 'uint32_t':
                # conver the uint32_t to a byte array (4 bytes)
                uint32_bytes = struct.pack("<I", data['data'])
                command_bytes.extend(uint32_bytes)
        # will be in the format: {0x02}{0x02}{destination_address 2B}{message_length 2B}{data_bytes undefined length}\n
        final_command = bytearray()
        final_command.extend(b'\x02\x02')
        final_command.extend(address.to_bytes(2, byteorder='big'))
        final_command.extend(len(command_bytes).to_bytes(2, byteorder='big'))
        final_command.extend(command_bytes)
        log_message(f"Sending command to {device_name}: {final_command}")
        
        if device_name == "payload":
            payloadSender.serial_input_bytes(final_command)
        elif device_name == "recovery":
            recovery_serial.serial_input_bytes(final_command)
        return jsonify(message = "OK"), 200
    else:
        return 'Content-Type not supported!'


@app.route('/string_command/<command>') 
def string_command(command):
    print(command)
    # if recovery_serial.close:
    #   return jsonify("PORT NOT OPEN"), 500
    recovery_serial.serial_input(command)
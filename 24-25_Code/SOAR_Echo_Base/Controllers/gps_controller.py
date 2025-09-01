import time
from Config import *
import Services.serial_device as serial_device
from flask import Flask, render_template, jsonify, send_from_directory
from threading import Thread
import re


# RECEIVER FUNCTIONS
@app.route("/gps_data")
def gps_controller():
    directory = os.path.join(app.root_path,'..', 'Views', 'gps_data')
    return send_from_directory(directory, 'gps_data.html')


@socketio.on("disconnect")
def test_disconnect():
    print("Client disconnected")


def display_gps_data():
    # WIll receive data
    pass


@app.route("/telemetry_start/<port_name>")
def telemetry_start(port_name="COM7"):
    # start the gps/telemetry thread
    try:
        serial_device.connect(port_name)
        gps_telem_thread = Thread(target=serial_device.receive_data)
        gps_telem_thread.daemon = True
        gps_telem_thread.start()
        time.sleep(2)
        serial_device.gps_repeat()
        serial_device.telemetry_repeat()
    except Exception as e:
        msg = f"Exception with GPS/Telemetry system: {e}"
        print(msg)
        return jsonify(message=msg), 500
    return jsonify(message="OK"), 200


# @app.route('/close_serial_gps')
# def close_serial_gps():
#     try:
#         lora.close()
#     except Exception as e:
#         msg = f"Unable to Close Serial Connection: {e}"
#         print(msg)
#         return jsonify(message = msg), 500
#     return jsonify(message='OK'),200
# SENDER FUNCTIONS
def update_gps(section, nmea):
    socketio.emit("gps_update", {"section": section, "nmea": nmea})


def log_msg(message):
    socketio.emit("gps_log", {"message": message})

import serial
import struct
import time

ser = None

def setup_sim(port='/dev/ttyGS0', baudrate=115200, timeout=0):
    global ser
    ser = serial.Serial(port, baudrate, timeout=timeout)
    time.sleep(2)

def _write(data):
    ser.write(data)

def _read_exactly(n, timeout=1.0):
    buffer = b''
    start = time.time()
    while len(buffer) < n and (time.time() - start < timeout):
        if ser.in_waiting:
            buffer += ser.read(n - len(buffer))
    return buffer if len(buffer) == n else None

def getSimFloat(req_byte1, req_byte2, res_byte):
    _write(bytes([req_byte1, req_byte2]))
    response = _read_exactly(1)
    if response and response[0] == res_byte:
        float_bytes = _read_exactly(4)
        if float_bytes:
            return struct.unpack('<f', float_bytes)[0]
    return 44330.00

def getSimVector3(req_byte1, req_byte2, res_byte):
    _write(bytes([req_byte1, req_byte2]))
    response = _read_exactly(1)
    if response and response[0] == res_byte:
        vector_bytes = _read_exactly(12)
        if vector_bytes:
            return struct.unpack('<fff', vector_bytes)
    return 0.0, 0.0, 0.0

def setSimulatedAngle(angle):
    # Only sends one int32 angle (no servo number)
    _write(bytes([0x07, 0x01]) + struct.pack('<i', int(angle)))
    response = _read_exactly(1)
    return response and response[0] == 0x07

def getSimulatedAltitude():
    return getSimFloat(0x03, 0x01, 0x03)

def getSimulatedTemperature():
    return getSimFloat(0x03, 0x02, 0x03)

def getSimulatedPressure():
    return getSimFloat(0x03, 0x03, 0x03)

def getSimulatedAcceleration():
    return getSimVector3(0x04, 0x01, 0x04)

def getSimulatedLinearAcceleration():
    return getSimVector3(0x04, 0x02, 0x04)

def getSimulatedGravity():
    return getSimVector3(0x04, 0x03, 0x04)

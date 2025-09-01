import serial
import platform
import csv
import queue
from datetime import datetime
from Models import LoraPacket

class SerialDevice:
    def __init__(self, name):
        self.name = name
        self.port = None
        self.device = None
        self.message_queue = queue.Queue()
        # Queue of lora packets
        self.lora_packet_queue = queue.Queue()

    def connect(self, port=None):
        if not port:
            if platform.system() == "Windows":
                port = "COM3"  # Update with the correct COM port for Windows
            else:
                port = "/dev/ttyACM0"  # Update with the correct port for Linux
        self.port = port

        baudrate = 115200
        self.device = serial.Serial(port, baudrate, timeout=1)
    
    def close(self):
        self.device.close()

    # Assume an array of bytes as input
    def serial_input_bytes(self, bytes_array):
        # check the device is open
        if not self.device:
            raise Exception("Device not connected")
        # Write the bytes to the device
        self.device.write(bytes_array)

    def serial_input(self, msg):
        self.device.write(bytes(msg, "utf-8"))

    def receive_data(self):
        if not self.device:
            raise Exception("Device not connected")
        
        file_name_datetime = datetime.now().strftime("%m-%d-%Y %H-%M-%S")
        # Create a file with the name "{device_name}_serial_log_{current_date}.csv"
        filename = f"{self.name}_serial_log_{file_name_datetime}.csv"
        with open(filename, "w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(["datetime", "serial msg"])

        while True:
            #Check if device is open, if not, break the loop
            if not self.device.is_open:
                break
            # Check if data is available to read
            if self.device.in_waiting > 0:
                # Read the first 2 bytes
                cmd = self.device.read(2)
                # If the first 2 bytes are b'\x05\x15', then print LORA message
                if cmd == b'\x05\x15':
                    # The rest of the message will look like:
                    # {valid_data 1 byte}{address 2 bytes}{length 2 bytes}{data_bytes unknown # of bytes};{2 byte checksum}{2 byte rssi}{2 byte snr}\n
                    # Read byte by byte by getting the ';'
                    self.message_queue.put("LORA message received")
                    valid_data_byte = self.device.read(1)
                    address_bytes = self.device.read(2)
                    length_bytes = self.device.read(2)
                    data_bytes = bytearray()
                    length = int.from_bytes(length_bytes, "big")
                    for i in range(length-2):
                        byte = self.device.read(1)
                        data_bytes += byte
                    checksum_bytes = self.device.read(2)
                    rssi_bytes = self.device.read(2)
                    snr_bytes = self.device.read(2)
                    raw_msg = bytearray()
                    raw_msg.extend(b'\x05\x15')
                    raw_msg.extend(valid_data_byte)
                    raw_msg.extend(address_bytes)
                    raw_msg.extend(length_bytes)
                    raw_msg.extend(data_bytes)
                    raw_msg.extend(checksum_bytes)
                    raw_msg.extend(rssi_bytes)
                    raw_msg.extend(snr_bytes)
                    self.message_queue.put(f'{raw_msg}')
                    # Convert valid_data_byte to bool, address_bytes, length_bytes, checksum, rssi and snr to int
                    valid_data = bool(int.from_bytes(valid_data_byte, "big"))
                    address = int.from_bytes(address_bytes, "big")
                    checksum = int.from_bytes(checksum_bytes, "big")
                    rssi = int.from_bytes(rssi_bytes, "big")
                    snr = int.from_bytes(snr_bytes, "big")

                    # Create a LoraPacket object and put it in the queue
                    lora_packet = LoraPacket(valid_data, address, length, data_bytes, checksum, rssi, snr)
                    self.lora_packet_queue.put(lora_packet)

                    # Save the raw data in it's og format and the LoraPacket object string representation to the file
                    with open(filename, "a", newline="") as file:
                        writer = csv.writer(file)
                        # raw data is x05x15 + the rest of the message with the ; and the \n
                        writer.writerow([datetime.now(), lora_packet])
                    
                else:
                    # If decoding fails, handle the data as raw bytes
                    full_msg_bytes = bytearray()
                    full_msg_bytes.extend(cmd)
                    # Read the rest of the message until \n is found
                    for i in range(10000):
                        byte = self.device.read(1)
                        if byte == b'\n':
                            break
                        full_msg_bytes.extend(byte)
                    # Convert the bytes to a string
                    full_msg =str(full_msg_bytes)
                    # remove "bytearray(b'" and "')" from the string
                    full_msg = full_msg[12:-2]
                    # remove \r and \n from the string
                    full_msg = full_msg.replace("\\r", "")
                    full_msg = full_msg.replace("\\n", "")
                    
                    if full_msg and full_msg != "\r\n":
                        # Add the message to the queue
                        self.message_queue.put(full_msg)

                        # Write the message to the file
                        with open(filename, "a", newline="") as file:
                            writer = csv.writer(file)
                            writer.writerow([datetime.now(), full_msg])
                        print(full_msg)
                

# Example usage:
if __name__ == "__main__":
    arduino_device = SerialDevice()
    arduino_device.serial_input("Hello, Arduino!")
    arduino_device.close()

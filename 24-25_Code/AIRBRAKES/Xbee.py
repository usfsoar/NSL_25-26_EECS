#!/usr/bin/env python3
from digi.xbee.devices import XBeeDevice

# XBeeTransmitter class definition
class XBeeTransmitter:
    # Change remote_node_id to the NODE_ID of the ground station XBee
    def __init__(self, port="/dev/ttyS0", baud_rate=9600, remote_node_id="Payload"):
        self.port = port
        self.baud_rate = baud_rate
        self.remote_node_id = remote_node_id
        self.device = XBeeDevice(self.port, self.baud_rate)
        self.remote_device = None

    def open_connection(self):
        try:
            self.device.open()
            print(f"[INFO] XBee opened: Addr={self.device.get_64bit_addr()}, NodeID={self.device.get_node_id()}")
            
            xbee_network = self.device.get_network()
            self.remote_device = xbee_network.discover_device(self.remote_node_id)
            
            if self.remote_device is None:
                raise Exception(f"No Remote Device with Node ID '{self.remote_node_id}' found.")
            else:
                print(f"[INFO] Remote device found: {self.remote_device.get_64bit_addr()}")
                
        except Exception as e:
            print(f"[ERROR] {e}")
            self.close_connection()
            # Propagate the exception so that the test knows something went wrong.
            raise

    def send_milestone(self, message):
        try:
            self.device.send_data(self.remote_device, message)
            print(f"[INFO] Milestone sent: '{message}'")
        except Exception as e:
            print(f"[ERROR] Failed to send message: {e}")

    def close_connection(self):
        if self.device is not None and self.device.is_open():
            self.device.close()
            print("[INFO] XBee connection closed.")

# Test function for the XBeeTransmitter class
def main():
    xbee = None
    try:
        # Instantiate the transmitter. Adjust parameters if necessary.
        xbee = XBeeTransmitter(port="/dev/ttyS0", baud_rate=9600, remote_node_id="Payload")
        # Open the connection once, keeping it open throughout the test
        xbee.open_connection()
        
        print("=== XBeeTransmitter Test ===")
        test_message = "Airbrakes deployed at 1200m"
        print(f"Sending test milestone: '{test_message}'")
        xbee.send_milestone(test_message)
        
        # Depending on your test scenario, you could send more messages or add delays.
        # For example, uncomment the following if you want a simple loop test:
        #
        # import time
        # for i in range(3):
        #     milestone = f"Milestone {i+1}: t={time.time()}"
        #     xbee.send_milestone(milestone)
        #     time.sleep(1)
        
    except Exception as error:
        print(f"[EXCEPTION] {error}")
    finally:
        # Ensure that the connection is closed even if an error occurred
        if xbee is not None and xbee.device.is_open():
            xbee.close_connection()

if __name__ == '__main__':
    main()

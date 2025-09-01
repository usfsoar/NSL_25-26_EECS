from digi.xbee.devices import XBeeDevice

class XBeeTransmitter:
    #change remote_node_id to the NODE_ID of ground station XBEE
    def __init__(self, port="/dev/ttyS0", baud_rate=9600, remote_node_id="White Antenna"):
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
            
        except Exception as e:
            self.close_connection()
            raise

    def send_milestone(self, message):
        try:
            self.device.send_data(self.remote_device, message)

        except Exception as e:
            print(f"[ERROR] Failed to send message: {e}")

    def close_connection(self):
        if self.device is not None and self.device.is_open():
            self.device.close()
            print("[INFO] XBee connection closed.")

# Example usage in another file:
# from xbee_transmitter import XBeeTransmitter
# xbee = XBeeTransmitter()
# xbee.open_connection()
# xbee.send_milestone("Airbrakes deployed at 1200m")
# xbee.close_connection()

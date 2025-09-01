from PySide6.QtCore import QThread, Signal
import time
import random  # temporary import for simulating data reception


class DataReceiver(QThread):
    # signal to send new data to the UI thread
    data_received = Signal(str)

    def run(self):
        # simulate reading from the microcontroller
        while True:
            # replace this with actual data reception logic
            new_data = self.read_from_microcontroller()
            self.data_received.emit(new_data)
            time.sleep(1)  # Simulate delay (e.g., from serial communication)

    def read_from_microcontroller(self):
        # simulate receiving new data
        random_value = random.randint(0, 100)
        return str(random_value)

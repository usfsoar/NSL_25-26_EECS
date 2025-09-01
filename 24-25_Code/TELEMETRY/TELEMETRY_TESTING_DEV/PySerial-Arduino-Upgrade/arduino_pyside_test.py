import sys
import time
import serial
from PySide6.QtCore import QSize, Qt, QThread, Signal, Slot
from PySide6.QtWidgets import *
from PySide6.QtGui import QColor

class ReadESP32(QThread):
    esp32_response = Signal(str)
    send_command = Signal(str)

    def __init__(self):
        super().__init__()
        self.command = None
        try:
            self.esp32 = serial.Serial(port='/dev/cu.usbmodem101', baudrate=9600, timeout=1)
            time.sleep(5)  # Wait for ESP32 to stabilize
        except serial.SerialException as e:
            print(f"Error opening serial port: {e}")
            self.esp32 = None

    def run(self):
        if not self.esp32:
            return

        while True:
            if self.command:
                try:
                    print(f"Sending command: {self.command}")
                    self.esp32.write((self.command + "\n").encode())  # Send command to ESP32
                    time.sleep(0.5)  # Allow more time for response
                    response = self.esp32.readline().strip()  # Read response
                    if response:
                        print(f"Received response: {response.decode()}")
                        self.esp32_response.emit(response.decode())
                    else:
                        print("No response received or response is empty")
                except serial.SerialException as e:
                    print(f"Error communicating with ESP32: {e}")
                self.command = None

    @Slot(str)
    def receive_command(self, command):
        self.command = command

    def close(self):
        if self.esp32:
            self.esp32.close()  # Close Serial connection
            print("Connection closed.")

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")
        #self.setFixedSize(QSize(400, 300))

        self.input = QLineEdit()
        self.input.setPlaceholderText("Enter command (on/off/N to exit)")

        self.label = QLabel()

        self.input.returnPressed.connect(self.send_command)

        layout = QVBoxLayout()
        layout.addWidget(self.input)
        layout.addWidget(self.label)

        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        self.esp32_thread = ReadESP32()
        self.esp32_thread.esp32_response.connect(self.update_label)
        self.esp32_thread.send_command.connect(self.esp32_thread.receive_command)
        self.esp32_thread.start()

    @Slot()
    def send_command(self):
        command = self.input.text().strip().lower()
        if command in ["on", "off", "n"]:
            print(f"User choice: {command}")
            self.esp32_thread.send_command.emit(command)
            if command == "n":
                self.esp32_thread.close()
                self.esp32_thread.quit()
                self.esp32_thread.wait()
        else:
            self.label.setText("Invalid command! Use 'on', 'off', or 'N'.")

    @Slot(str)
    def update_label(self, response):
        self.label.setText(f"ESP32: {response}")

app = QApplication(sys.argv)

window = MainWindow()
window.show()

app.exec()
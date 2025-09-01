import sys
import time
from PySide6.QtCore import QSize, Qt, QThread, Signal, Slot
from PySide6.QtWidgets import *
import serial

# Set up Serial connection (Update 'COM3' based on your system)
arduino = serial.Serial(port="/dev/cu.usbmodem1101", baudrate=9600, timeout=1)

class ReadThread(QThread):
    arduino_response = Signal(str)

    def run(self):
        while True:
            response = arduino.readline().strip()  # Read response
            if response:
                print(f"Received response: {response.decode()}")  # Debug print
                self.arduino_response.emit(response.decode())
            else:
                print("No response received or response is empty")
                self.arduino_response.emit(f"No response received or response is empty{response.decode()}")
            time.sleep(0.1)  # Small delay to prevent high CPU usage

class WriteThread(QThread):
    stop_signal = Signal()

    def __init__(self, command):
        super().__init__()
        self.command = command

    def run(self):
        if self.command not in ["on", "off", "n"]:
            print("Invalid command! Use 'on', 'off', or 'n'.")
            return
        
        if self.command == "n":
            arduino.close()
            print("Connection closed.")
            self.stop_signal.emit()
            return
        
        arduino.write((self.command + "\n").encode())
        print(f"Sent command: {self.command}")

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")
        self.setFixedSize(QSize(400, 300))

        self.input = QLineEdit()
        self.button = QPushButton("Send")
        self.label = QLabel()

        self.input.setPlaceholderText("Enter command (on/off/N to exit): ")

        self.button.clicked.connect(self.the_button_was_clicked)

        layout_v = QVBoxLayout()
        layout_h = QHBoxLayout()
        layout_h.addWidget(self.input)
        layout_h.addWidget(self.button)
        layout_v.addLayout(layout_h)
        layout_v.addWidget(self.label)

        container = QWidget()
        container.setLayout(layout_v)
        self.setCentralWidget(container)

        # Start the read thread
        self.read_thread = ReadThread()
        self.read_thread.arduino_response.connect(self.update_label)
        self.read_thread.start()

    @Slot()
    def the_button_was_clicked(self):
        command = self.input.text().strip().lower()
        print(f"User choice: {command}")  # Print user choice
        self.write_thread = WriteThread(command)
        self.write_thread.stop_signal.connect(self.stop_application)
        self.write_thread.start()

    @Slot(str)
    def update_label(self, response):
        print(f"Updating label with response: {response}")  # Debug print
        self.label.setText(f"Arduino: {response}")

    @Slot()
    def stop_application(self):
        print("Stopping application...")
        self.read_thread.terminate()
        self.read_thread.wait()
        QApplication.quit()
        
app = QApplication(sys.argv)

window = MainWindow()
window.show()

app.exec()
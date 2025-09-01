"""
GPS_Screen_Updater.py - GPS Display Application

This application takes $GNRMC formatted sentences as input and displays 
the parsed GPS information in a graphical user interface.
"""

from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLineEdit, QPushButton, 
    QTextEdit, QScrollBar
)
import sys
from datetime import datetime

def parse_nmea_gnrmc(nmea_sentence: str) -> str:
    """
    Parses a $GNRMC sentence and extracts relevant data.
    
    Args:
        nmea_sentence (str): The $GNRMC sentence.
    
    Returns:
        str: The formatted output string.
    """
    try:
        # Check if the sentence starts with $GNRMC
        if not nmea_sentence.startswith("$GNRMC"):
            return "Unsupported or invalid NMEA sentence."
        
        # Split the sentence into parts
        parts = nmea_sentence.strip().split(",")
        if len(parts) < 10:
            return "Error: Incomplete NMEA sentence."
        
        # Extract rocket time
        rocket_hour = int(parts[1])
        rocket_minute = int(parts[2])
        rocket_second = int(parts[3])
        rocket_microsecond = int(parts[4])
        
        # Check the status field
        status = parts[5]  # Status field (e.g., "A" for valid data)
        if status != "A":
            return "Error: GPS data is not valid (status is not 'A')."
        
        # Extract GPS data
        lat_deg = float(parts[6])  # Latitude degrees
        lat_dir_input = parts[7]  # Latitude direction (N/S)
        lon_deg = float(parts[8])  # Longitude degrees
        lon_dir_input = parts[9]  # Longitude direction (E/W)
        
        # Get the current system time
        sys_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        return (f"Sys_Time: {sys_time}, Rocket_Time: {rocket_hour}h, {rocket_minute}m, {rocket_second}s, {rocket_microsecond}µs, "
                f"Latitude: {lat_deg}°, N/S: {lat_dir_input}, Longitude: {lon_deg}°, E/W: {lon_dir_input}")
    except Exception as e:
        return f"Error parsing NMEA sentence: {e}"

class GPSScreenUpdater(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("GPS Screen Updater")
        
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        
        self.input_field = QLineEdit()
        self.input_field.setPlaceholderText("Enter $GNRMC Sentence")
        self.layout.addWidget(self.input_field)
        
        self.button = QPushButton("Update GPS")
        self.button.clicked.connect(self.update_output)
        self.layout.addWidget(self.button)
        
        self.output_box = QTextEdit()
        self.output_box.setReadOnly(True)
        self.layout.addWidget(self.output_box)
        
        # Add helper text
        self.output_box.setPlaceholderText(
            "Input format: $GNRMC,hour,minute,second,microsecond,A,latitude,N/S,longitude,E/W,speed\n\n"
            "Example: $GNRMC,657,2,35,500,A,34.123,N,118.467,W,5"
        )
    
    def update_output(self, input_text: str = None):
        text = self.input_field.text()
        result = parse_nmea_gnrmc(input_text or text)
        self.output_box.append(result)
        self.input_field.clear()
        
        # Scroll to bottom automatically
        scrollbar = self.output_box.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GPSScreenUpdater()
    window.resize(500, 400)
    window.show()
    sys.exit(app.exec())
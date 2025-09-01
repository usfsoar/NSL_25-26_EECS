import json
import os
import time
import sys
import serial
from serial.tools import list_ports
from PySide6.QtCore import QThread, Signal, Slot
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QTabWidget,
    QWidget,
    QTabBar,
    QMessageBox,
    QInputDialog,
)
from ChartBuilder import ChartBuilder
from sensor_data import SensorData
from data_processor import data_processor
from ButtonHandler import ButtonHandler
from data_transform import *
from GPS_Screen_Updater import GPSScreenUpdater
from charts import TemplateChart

def get_available_ports():
    """Returns a list of available serial ports."""
    ports = list_ports.comports()
    return [port.device for port in ports]

class ReadThread(QThread):
    arduino_response = Signal(str)
    sensor_data_signal = Signal(SensorData)
    def __init__(self, arduino):
        super().__init__()
        self.arduino = arduino
    def run(self):
        while True:
            if self.arduino and self.arduino.is_open:
                try:
                    response = self.arduino.readline().strip()  # Read response
                    if response:
                        print(f"Received response: {response.decode()}")  # Debug print
                        self.arduino_response.emit(response.decode())
                    else:
                        print("No response received or response is empty")
                except serial.SerialException as e:
                    print(f"Serial error: {e}")
                    self.arduino_response.emit(f"Serial error: {e}")
                    break
            else:

                print("Serial port not open")

                #time.sleep(1)  # Avoid high CPU usage when waiting

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Ground Station")
        self.arduino = None
        self.read_thread = None

        # Record the start time
        self.start_time = time.time()

        # Create a tab widget
        self.tabs = QTabWidget()
        self.setCentralWidget(self.tabs)

        # Create custom tabs for IMU and BMP charts
        self.tab_imu = QTabWidget()
        self.tab_bmp = QTabWidget()
        self.gps = QTabWidget()
        self.custom_tab = ChartBuilder()  # For user-created charts

        # Add these custom tabs to the main tab widget
        self.tabs.addTab(self.tab_bmp, "BMP Sensors")
        self.tabs.addTab(self.tab_imu, "IMU Sensors")
        self.tabs.addTab(self.gps, "GPS")
        self.tabs.addTab(self.custom_tab, "Custom Charts")

        # Add "+" button to add new custom tabs
        self.tabs.setTabsClosable(True)
        self.tabs.tabBar().addTab("+")
        self.tabs.tabBarClicked.connect(self.handle_tab_click)

        self.tabs.tabCloseRequested.connect(self.close_tab)

        # Remove the close button on the '+' tab and the main sensor tabs
        self.tabs.tabBar().setTabButton(self.tabs.count() - 1, QTabBar.RightSide, None)
        self.tabs.tabBar().setTabButton(0, QTabBar.RightSide, None)  # BMP tab
        self.tabs.tabBar().setTabButton(1, QTabBar.RightSide, None)  # IMU tab
    

        # Initialize GPS screen updater before serial connection
        self.gps_screen_updater = GPSScreenUpdater()
        self.gps_screen_updater.setWindowTitle("GPS Screen Updater")
        self.gps_screen_updater.show()  # Show the GPS window immediately

        # Initialize serial connection
        self.init_serial_connection()

        # Initialize BMP charts
        self.chart_altitude = TemplateChart("Altitude", "Time", "Altitude (m)")
        self.chart_pressure = TemplateChart("Pressure", "Time", "Pressure (Pa)")
        self.chart_temperature = TemplateChart("Temperature", "Time", "Temperature (°C)")

        # Initialize IMU charts
        # Accelerometer charts
        self.chart_accel_x = TemplateChart("Acceleration X", "Time", "Acceleration (m/s²)")
        self.chart_accel_y = TemplateChart("Acceleration Y", "Time", "Acceleration (m/s²)")
        self.chart_accel_z = TemplateChart("Acceleration Z", "Time", "Acceleration (m/s²)")
        
        # Linear acceleration charts
        self.chart_linear_x = TemplateChart("Linear X", "Time", "Linear Acceleration (m/s²)")
        self.chart_linear_y = TemplateChart("Linear Y", "Time", "Linear Acceleration (m/s²)")
        self.chart_linear_z = TemplateChart("Linear Z", "Time", "Linear Acceleration (m/s²)")
        
        # Gravity charts
        self.chart_gravity_x = TemplateChart("Gravity X", "Time", "Gravity (m/s²)")
        self.chart_gravity_y = TemplateChart("Gravity Y", "Time", "Gravity (m/s²)")
        self.chart_gravity_z = TemplateChart("Gravity Z", "Time", "Gravity (m/s²)")
        
        # Quaternion charts
        self.chart_quat_w = TemplateChart("Quaternion W", "Time", "Quaternion W")
        self.chart_quat_x = TemplateChart("Quaternion X", "Time", "Quaternion X")
        self.chart_quat_y = TemplateChart("Quaternion Y", "Time", "Quaternion Y")
        self.chart_quat_z = TemplateChart("Quaternion Z", "Time", "Quaternion Z")
        
        # Gyroscope charts
        self.chart_gyro_x = TemplateChart("Gyroscope X", "Time", "Angular Velocity (rad/s)")
        self.chart_gyro_y = TemplateChart("Gyroscope Y", "Time", "Angular Velocity (rad/s)")
        self.chart_gyro_z = TemplateChart("Gyroscope Z", "Time", "Angular Velocity (rad/s)")

        # Add BMP charts to the BMP tab
        self.tab_bmp.addTab(self.chart_altitude, "Altitude")
        self.tab_bmp.addTab(self.chart_pressure, "Pressure")
        self.tab_bmp.addTab(self.chart_temperature, "Temperature")

        # Add IMU charts to the IMU tab, organized by sensor type
        # Create subtabs for different sensor types within the IMU tab
        self.tab_imu_accel = QTabWidget()
        self.tab_imu_linear = QTabWidget()
        self.tab_imu_gravity = QTabWidget()
        self.tab_imu_quat = QTabWidget()
        self.tab_imu_gyro = QTabWidget()
        
        # Add these subtabs to the IMU tab
        self.tab_imu.addTab(self.tab_imu_accel, "Accelerometer")
        self.tab_imu.addTab(self.tab_imu_linear, "Linear Acceleration")
        self.tab_imu.addTab(self.tab_imu_gravity, "Gravity")
        self.tab_imu.addTab(self.tab_imu_quat, "Quaternion")
        self.tab_imu.addTab(self.tab_imu_gyro, "Gyroscope")
        
        # Add charts to the appropriate subtabs
        # Accelerometer
        self.tab_imu_accel.addTab(self.chart_accel_x, "X-Axis")
        self.tab_imu_accel.addTab(self.chart_accel_y, "Y-Axis")
        self.tab_imu_accel.addTab(self.chart_accel_z, "Z-Axis")
        
        # Linear acceleration
        self.tab_imu_linear.addTab(self.chart_linear_x, "X-Axis")
        self.tab_imu_linear.addTab(self.chart_linear_y, "Y-Axis")
        self.tab_imu_linear.addTab(self.chart_linear_z, "Z-Axis")
        
        # Gravity
        self.tab_imu_gravity.addTab(self.chart_gravity_x, "X-Axis")
        self.tab_imu_gravity.addTab(self.chart_gravity_y, "Y-Axis")
        self.tab_imu_gravity.addTab(self.chart_gravity_z, "Z-Axis")
        
        # Quaternion
        self.tab_imu_quat.addTab(self.chart_quat_w, "W")
        self.tab_imu_quat.addTab(self.chart_quat_x, "X")
        self.tab_imu_quat.addTab(self.chart_quat_y, "Y")
        self.tab_imu_quat.addTab(self.chart_quat_z, "Z")
        
        # Gyroscope
        self.tab_imu_gyro.addTab(self.chart_gyro_x, "X-Axis")
        self.tab_imu_gyro.addTab(self.chart_gyro_y, "Y-Axis")
        self.tab_imu_gyro.addTab(self.chart_gyro_z, "Z-Axis")

    def init_serial_connection(self):
        """Initialize the serial connection by letting the user choose a port."""
        ports = get_available_ports()
        if not ports:
            QMessageBox.critical(
                self,
                "No Ports Available",
                "No serial ports found. Please connect a device.",
            )
            return

        # Let the user select a port
        port, ok = QInputDialog.getItem(
            self,
            "Select Serial Port",
            "Available Ports:",
            ports,
            0,  # Default index
            False,  # Not editable
        )
        if ok and port:
            try:
                self.arduino = serial.Serial(port=port, baudrate=115200, timeout=1)
                print(f"Connected to port: {port}")

                # Start the read thread
                self.read_thread = ReadThread(self.arduino)
                self.read_thread.arduino_response.connect(self.serialReader)
                self.read_thread.sensor_data_signal.connect(self.update_sensor_data)
                self.read_thread.start()
            except Exception as e:
                QMessageBox.critical(
                    self, "Connection Error", f"Failed to connect: {e}"
                )

    @Slot(str)
    def serialReader(self, response):
        print(f"Updating label with response: {response}")  # Debug print

        try:
            # Decode the response
            ResponseASCII = process_hex_input(response)  # Process the hex input
            ResponseType = identify_string_type(ResponseASCII)  # Identify the type of string

            if (ResponseType == "GPS"):
                # Process GPS data
                gps_data = prepare_gps_input(ResponseASCII)
                print(f"Processed GPS data: {gps_data}")
                self.gps_screen_updater.update_output(gps_data)
            
            elif (ResponseType == "BMP"):
                # Process BMP data
                bmp_data = process_bmp_data(ResponseASCII)
                print(f"Processed BMP data: {bmp_data}")
                elapsed_time = time.time() - self.start_time

                # Update the charts for altitude, temperature, and pressure
                if hasattr(self, "chart_altitude") and bmp_data.get("altitude") is not None:
                    self.chart_altitude.update_chart([elapsed_time], [bmp_data["altitude"]])
                if hasattr(self, "chart_temperature") and bmp_data.get("temperature") is not None:
                    self.chart_temperature.update_chart([elapsed_time], [bmp_data["temperature"]])
                if hasattr(self, "chart_pressure") and bmp_data.get("pressure") is not None:
                    self.chart_pressure.update_chart([elapsed_time], [bmp_data["pressure"]])

            elif (ResponseType == "IMU"):
                # Process IMU data
                imu_data = process_imu_data(ResponseASCII)
                print(f"Processed IMU data: {imu_data}")

                # Get the time
                elapsed_time = time.time() - self.start_time

                # Update accel_x, accel_y, and accel_z charts -------------------------------------
                if hasattr(self, "chart_accel_x") and imu_data.get("accel_x") is not None:
                    self.chart_accel_x.update_chart([elapsed_time], [imu_data["accel_x"]])
                if hasattr(self, "chart_accel_y") and imu_data.get("accel_y") is not None:
                    self.chart_accel_y.update_chart([elapsed_time], [imu_data["accel_y"]])
                if hasattr(self, "chart_accel_z") and imu_data.get("accel_z") is not None:
                    self.chart_accel_z.update_chart([elapsed_time], [imu_data["accel_z"]])

                # Update linear_x, linear_y, and linear_z charts ----------------------------------
                if hasattr(self, "chart_linear_x") and imu_data.get("linear_x") is not None:
                    self.chart_linear_x.update_chart([elapsed_time], [imu_data["linear_x"]])
                if hasattr(self, "chart_linear_y") and imu_data.get("linear_y") is not None:
                    self.chart_linear_y.update_chart([elapsed_time], [imu_data["linear_y"]])
                if hasattr(self, "chart_linear_z") and imu_data.get("linear_z") is not None:
                    self.chart_linear_z.update_chart([elapsed_time], [imu_data["linear_z"]])

                # Update gravity_x, gravity_y, and gravity_z charts -------------------------------
                if hasattr(self, "chart_gravity_x") and imu_data.get("gravity_x") is not None:
                    self.chart_gravity_x.update_chart([elapsed_time], [imu_data["gravity_x"]])
                if hasattr(self, "chart_gravity_y") and imu_data.get("gravity_y") is not None:
                    self.chart_gravity_y.update_chart([elapsed_time], [imu_data["gravity_y"]])
                if hasattr(self, "chart_gravity_z") and imu_data.get("gravity_z") is not None:
                    self.chart_gravity_z.update_chart([elapsed_time], [imu_data["gravity_z"]])

                # Update quat_w, quat_x, quat_y, and quat_z charts --------------------------------
                if hasattr(self, "chart_quat_w") and imu_data.get("quat_w") is not None:
                    self.chart_quat_w.update_chart([elapsed_time], [imu_data["quat_w"]])
                if hasattr(self, "chart_quat_x") and imu_data.get("quat_x") is not None:
                    self.chart_quat_x.update_chart([elapsed_time], [imu_data["quat_x"]])
                if hasattr(self, "chart_quat_y") and imu_data.get("quat_y") is not None:
                    self.chart_quat_y.update_chart([elapsed_time], [imu_data["quat_y"]])
                if hasattr(self, "chart_quat_z") and imu_data.get("quat_z") is not None:
                    self.chart_quat_z.update_chart([elapsed_time], [imu_data["quat_z"]])    
                    
                # Update gyro_x, gyro_y, and gyro_z charts -------------------------------------
                if hasattr(self, "chart_gyro_x") and imu_data.get("gyro_x") is not None:
                    self.chart_gyro_x.update_chart([elapsed_time], [imu_data["gyro_x"]])
                if hasattr(self, "chart_gyro_y") and imu_data.get("gyro_y") is not None:
                    self.chart_gyro_y.update_chart([elapsed_time], [imu_data["gyro_y"]])
                if hasattr(self, "chart_gyro_z") and imu_data.get("gyro_z") is not None:
                    self.chart_gyro_z.update_chart([elapsed_time], [imu_data["gyro_z"]])

        except (ValueError, IndexError) as e:
            print(f"Error parsing response: {e}")

    @Slot(SensorData)
    def update_sensor_data(self, sensor_data):
        print(f"Received SensorData: {sensor_data}")  # Debug print
        data_processor(sensor_data, charts={})

    def add_new_tab(self):
        tab = ChartBuilder()
        index = self.tabs.addTab(tab, "Custom Chart")
        self.tabs.setCurrentIndex(index)

    def handle_tab_click(self, index):
        if index == self.tabs.count() - 1:  # "+" tab clicked
            self.add_new_tab()

    def close_tab(self, index):
        # Don't allow closing the BMP, IMU, or + tabs
        if index > 1 and index < self.tabs.count() - 1:
            self.tabs.removeTab(index)

    def closeEvent(self, event):
        """Clean up serial connection when closing the window."""
        if self.read_thread and self.read_thread.isRunning():
            self.read_thread.terminate()
        if self.arduino and self.arduino.is_open:
            self.arduino.close()
        # Close the GPS screen updater window
        if hasattr(self, "gps_screen_updater"):
            self.gps_screen_updater.close()
        event.accept()
    def update_text(self, evCode, evData):
        print(str(evCode) + str(evData))
def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
if __name__ == "__main__":
    main()




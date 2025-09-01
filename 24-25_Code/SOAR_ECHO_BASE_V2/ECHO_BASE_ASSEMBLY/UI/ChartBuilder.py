import os
from PySide6.QtWidgets import (
    QVBoxLayout,
    QPushButton,
    QLabel,
    QTreeWidget,
    QTreeWidgetItem,
    QGridLayout,
    QWidget,
)
from PySide6.QtCore import Qt
from typing import List, Tuple
import json
import sys
from sensor_data import SensorData

# sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'Charts'))
# from SOAR_ECHO_BASE_V2.ECHO_BASE_ASSEMBLY.UI.charts import TemplateChart

from charts import TemplateChart

services_path = os.path.join(os.path.dirname(__file__), '..', '..', 'Services')
sys.path.append(services_path)
from GPS_Screen_Updater import GPSScreenUpdater

class ChartBuilder(QWidget):
    def __init__(self, json_file="charts.json"):
        super().__init__()

        # Construct the full path to the JSON file
        json_file_path = os.path.join(os.path.dirname(__file__), json_file)

        # Create a main layout for this widget
        main_layout = QVBoxLayout(self)  # Set the layout directly on the widget

        main_layout.addWidget(QLabel("Select Charts:"))

        self.tree_widget = QTreeWidget()
        self.tree_widget.setHeaderHidden(True)
        main_layout.addWidget(self.tree_widget)

        self.load_sections_from_json(json_file_path)

        build_button = QPushButton("Build Charts")
        build_button.clicked.connect(self.on_build_charts)
        main_layout.addWidget(build_button)

    # Add sections and items to the tree widget based on the JSON file
    def load_sections_from_json(self, json_file):
        def add_sensor_section(section_name, items):
            # Create a top-level item for the sensor
            section_item = QTreeWidgetItem([section_name])
            self.tree_widget.addTopLevelItem(section_item)

            # Add items to the section
            for item_name in items:
                item = QTreeWidgetItem([item_name])
                item.setCheckState(
                    0, Qt.CheckState.Unchecked
                )  # Add a checkbox to the item
                section_item.addChild(item)

        # Load the JSON file and add sections and items to the tree widget
        with open(json_file, "r") as file:
            data = json.load(file)
            for section in data["sections"]:
                add_sensor_section(section["name"], section["items"])

    # Get all the checked items in the tree widget where the tuple is (sensor_name, value_name)
    def get_checked_items(self) -> List[Tuple[str, str]]:
        checked_items = []
        root = self.tree_widget.invisibleRootItem()
        for i in range(root.childCount()):
            section_item = root.child(i)
            section_name = section_item.text(0)
            for j in range(section_item.childCount()):
                item = section_item.child(j)
                if item.checkState(0) == Qt.CheckState.Checked:
                    checked_items.append((section_name, item.text(0)))
        return checked_items

    # Method called when the "Build Charts" button is clicked
    def on_build_charts(self):
        checked_items = self.get_checked_items()
        print("Checked items:", checked_items)

        # Create a new widget with a grid layout
        chart_widget = QWidget()
        grid_layout = QGridLayout(chart_widget)

        # Add placeholder widgets to the grid layout
        for index, (section, item) in enumerate(checked_items):
            print(f"Processing section: {section}, item: {item}")  # Debug print
            if section.lower().strip() == "gps":  # Check if the section is "GPS"
                gps_screen = GPSScreenUpdater()
                grid_layout.addWidget(gps_screen, index // 2, index % 2)
            else:
                chart = TemplateChart(f"Chart: {section} - {item}", "X Axis", "Y Axis")
                grid_layout.addWidget(chart, index // 2, index % 2)

        # Find the parent tab widget
        tab_widget = self.parent().parent()  # Go up two levels to reach the QTabWidget

        # Replace the current tab's widget with the new chart widget
        current_tab_index = tab_widget.currentIndex()
        tab_widget.removeTab(current_tab_index)
        tab_widget.insertTab(current_tab_index, chart_widget, "Charts")
        tab_widget.setCurrentIndex(current_tab_index)


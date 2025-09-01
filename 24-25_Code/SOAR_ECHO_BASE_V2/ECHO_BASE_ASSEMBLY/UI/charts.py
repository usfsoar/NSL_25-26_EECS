import sys
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QVBoxLayout,
    QWidget,
    QPushButton,
    QLineEdit,
    QLabel,
)
from matplotlib.figure import Figure
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas

class TemplateChart(FigureCanvas):
    def __init__(self, title, x_label, y_label, width=6, height=5, dpi=100):
        self.title = title
        self.x_label = x_label
        self.y_label = y_label
        fig = Figure(figsize=(width, height), dpi=dpi)
        self.axes = fig.add_subplot(1,1,1)
        self.axes.set_title(title)
        self.axes.set_xlabel(x_label)
        self.axes.set_ylabel(y_label)
        super(TemplateChart, self).__init__(fig)
        self.xdata = []
        self.ydata = []

    def update_chart(self, new_xdata, new_ydata):
        self.xdata.extend(new_xdata)
        self.ydata.extend(new_ydata)
        self.axes.clear()  # Clear the previous plot
        self.axes.set_title(self.title)  # Reset the title
        self.axes.set_xlabel(self.x_label)  # Reset the x-axis label
        self.axes.set_ylabel(self.y_label)  # Reset the y-axis label
        self.axes.plot(self.xdata, self.ydata)
        self.draw()


# Example usage
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = QMainWindow()
    window.setWindowTitle("Chart Example")

    chart = TemplateChart("Acceleration X", "Time", "Acceleration")

    input_x = QLineEdit()
    button = QPushButton("Update Chart")

    def on_button_click():
        input_text = input_x.text()
        try:
            x_str, y_str = input_text.split(",")
            x_value = float(x_str.strip())
            y_value = float(y_str.strip())
            chart.update_chart([x_value], [y_value])
        except ValueError:
            print("Invalid input format. Please enter two numbers separated by a comma.")

    button.clicked.connect(on_button_click)

    layout = QVBoxLayout()
    layout.addWidget(QLabel("Input [X Data, Y Data] (comma-separated):"))
    layout.addWidget(input_x)
    layout.addWidget(button)
    layout.addWidget(chart)

    container = QWidget()
    container.setLayout(layout)
    window.setCentralWidget(container)

    window.show()
    sys.exit(app.exec())


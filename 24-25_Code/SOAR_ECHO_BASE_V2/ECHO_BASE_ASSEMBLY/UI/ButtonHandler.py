from PySide6.QtCore import Qt, QThread, Slot, Signal
from inputs import get_gamepad

class ButtonHandler(QThread):
    gamepadEvent = Signal(str, float)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.button = 0

    @Slot(int)
    def run(self):
        while True:
            events = get_gamepad()
            for event in events:
                self.gamepadEvent.emit(event.code, event.state)
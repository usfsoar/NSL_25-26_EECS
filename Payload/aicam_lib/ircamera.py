from aicam_lib.camera import Camera

class IRCamera(Camera):
    def __init__(self, size: tuple[int, int] = (640, 480)):
        super().__init__(size)
        self.start()
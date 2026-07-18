from picamera2 import Picamera2

class Camera:
    def __init__(self, size: tuple[int, int]):
        self._cam = Picamera2()

        config = self._cam.create_video_configuration(
            main={"format": "RGB888", "size": size},
        )

        self._cam.configure(config)

    def start(self):
        self._cam.start()

    def stop(self):
        self._cam.stop()

    def getFrame(self):
        return self._cam.capture_array()
from picamera2 import Picamera2
from picamera2.devices.imx500 import IMX500
from aicam_lib.camera import Camera
from aicam_lib.inference import Inference
from aicam_lib.post_processing import boxFloatToInt
import copy

RESOLUTION_WIDTH = 2028
RESOLUTION_HEIGHT = 1520
AICAM_FRAME_RATE = 30
FOV_HORIZONTAL = 66
FOV_VERTICAL = 52.3

class AICamera(Camera):
    def __init__(self, network: str, size: tuple[int, int] = (640, 480)):
        super().__init__(size)

        self._imx500 = IMX500(network)
        self._previous_output = [[], [], []]
        self.start()

    def getInference(self):
        request = self._cam.capture_request()

        try:
            frame = request.make_array("main")
            metadata = request.get_metadata()
            outputs = self._imx500.get_outputs(metadata)

            if outputs is None:
                outputs = self._previous_output
            else:
                self._previous_output = copy.deepcopy(outputs)

            inferences = [Inference(boxFloatToInt(outputs[0][i]), outputs[1][i], int(outputs[2][i])) for i in range(len(outputs[0]))]
            return inferences, frame
        
        finally:
            request.release()
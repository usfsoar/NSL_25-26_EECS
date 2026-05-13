from picamera2 import Picamera2
from picamera2.devices.imx500 import IMX500
from aicamlib.camera import Camera
from aicamlib.inference import Inference
from aicamlib.post_processing import boxFloatToInt

class AICamera(Camera):
    def __init__(self, network: str, size: tuple[int, int] = (640, 480)):
        super().__init__(size)

        self._imx500 = IMX500(network)
        self._previous_output = [[], [], []]
        self.start()

    def getInference(self):
        metadata = self._cam.capture_metadata()
        outputs = self._imx500.get_outputs(metadata)

        if outputs is None:
            outputs = self._previous_output
        else:
            self._previous_output = outputs

        inferences = [Inference(boxFloatToInt(outputs[0][i]), outputs[1][i], int(outputs[2][i])) for i in range(len(outputs[0]))]
        return inferences
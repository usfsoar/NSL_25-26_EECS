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
MODEL_WIDTH = 640
MODEL_HEIGHT = 480

CROPPED_WIDTH = 1989

class AICamera(Camera):
    def __init__(self, network: str):
        super().__init__((RESOLUTION_WIDTH, RESOLUTION_HEIGHT))

        self._imx500 = IMX500(network)

        self._cam.set_controls({
           "ScalerCrop": ((RESOLUTION_WIDTH - CROPPED_WIDTH) // 2, 0, CROPPED_WIDTH, RESOLUTION_HEIGHT)
        })

        config = self._cam.create_preview_configuration(
            main={"size" : (MODEL_WIDTH, MODEL_HEIGHT)},
            raw={"size" : (RESOLUTION_WIDTH, RESOLUTION_HEIGHT)}
        )

        self._cam.configure(config)

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

            inferences = [Inference(boxFloatToInt(outputs[0][i]), outputs[1][i], int(outputs[2][i])) for i in range(len(outputs[0])) if outputs[1][i] > 0.05]
            return inferences, frame
        
        finally:
            request.release()
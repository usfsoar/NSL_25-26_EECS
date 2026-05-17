# from picamera2 import Picamera2
# from aicam_lib.camera import Camera
from aicam_lib.inference import Inference
from aicam_lib.post_processing import boxFloatToInt

import cv2
from ultralytics import YOLO

class WebotsAICamera():
    def __init__(self, network: str, size: tuple[int, int] = (640, 480)):
        self.model = YOLO(model=network)

        self._previous_output = [[], [], []]

    def getInference(self):
        results = self.model("testingyolo.png")
        img = cv2.imread("testingyolo.png")
        results = self.model.predict(source=img, save=False, save_txt=False, show_boxes=False, verbose=False)

        result = results[0]
        boxes = result.boxes.xyxy.cpu().numpy()
        scores = result.boxes.conf.cpu().numpy()
        classes = result.boxes.cls.cpu().numpy()

        # Match your old IMX500 inference format
        outputs = [boxes, scores, classes]

        inferences = [Inference(boxFloatToInt(outputs[0][i]), outputs[1][i], int(outputs[2][i])) for i in range(len(outputs[0]))]
        
        return inferences, img
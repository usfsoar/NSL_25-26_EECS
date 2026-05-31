# from picamera2 import Picamera2
# from aicam_lib.camera import Camera
from aicam_lib.inference import Inference
from aicam_lib.post_processing import boxFloatToInt

import cv2
from ultralytics import YOLO
import os
import multiprocessing as mp
import numpy as np

class WebotsAICamera():
    def __init__(self, network: str, shm_name, lock, size: tuple[int, int] = (640, 640)):
        self.model = YOLO(model=network)
        self.SHAPE = (1520, 2028, 3)

        self.shm = mp.shared_memory.SharedMemory(name=shm_name)
        self.frame = np.ndarray(self.SHAPE, dtype=np.uint8, buffer=self.shm.buf)
        self.lock = lock

        self._previous_output = [[], [], []]

    def getInference(self):
        with self.lock:
            img = self.frame.copy()

        img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
        results = self.model.predict(source=img, save=False, save_txt=False, show_boxes=False, verbose=False)

        result = results[0]
        boxes = result.boxes.xyxy.cpu().numpy()
        scores = result.boxes.conf.cpu().numpy()
        classes = result.boxes.cls.cpu().numpy()

        # Match IMX500 inference format
        outputs = [boxes, scores, classes]

        inferences = [Inference(boxFloatToInt(outputs[0][i]), outputs[1][i], int(outputs[2][i])) for i in range(len(outputs[0]))]
        
        return inferences, img
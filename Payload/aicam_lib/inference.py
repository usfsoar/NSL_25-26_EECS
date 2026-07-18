class Inference:
    def __init__(self, box: tuple[int, int, int, int], confidence: float, label_class: int):
        self.box = box
        self.confidence = confidence
        self.label_class = label_class

    def to_string(self):
        return f"Box: {self.box}, Confidence: {self.confidence}, Label: {self.label_class}" 

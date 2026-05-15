from aicamlib.inference import Inference

def boxFloatToInt(box: tuple[float, float, float, float]) -> tuple[int, int, int, int]:
    return (int(box[0]), int(box[1]), int(box[2]), int(box[3]))

def getInferencesAboveConfidence(inferences: list[Inference], threshold: float):
    above = []

    for inference in inferences:
        if inference.confidence > threshold:
            above.append(inference)
        else:
            break

    return above
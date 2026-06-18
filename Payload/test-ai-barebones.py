import aicam_lib.aicamera as ai
import aicam_lib.rendering as rendering
import cv2

aicam = ai.AICamera(network="model_final.rpk")

inferences, frame = aicam.getInference()

color = (0,255,0)

for inf in inferences:
    rendering.drawBox(frame, box=inf.box, color=color)
    rendering.drawText(frame,
                               text=f"ID: {id}, Conf: {inf.confidence}",
                               position=(inf.box[0], inf.box[1]),
                               size=1,
                               color=color,
                               thickness=2)

cv2.imwrite(f"frame.jpg", frame)

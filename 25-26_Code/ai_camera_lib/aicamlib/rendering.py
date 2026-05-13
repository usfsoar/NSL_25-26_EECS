import cv2

def drawWindow(window_name, frame):
    cv2.imshow(window_name, frame)
    cv2.waitKey(1)

def drawBox(frame, box: tuple[int, int, int, int], color: tuple[int, int, int] = (0, 0, 255)):

    box = [int(n) for n in box]
    cv2.rectangle(
        frame,
        (box[0], box[1]),
        (box[2], box[3]),
        color,
        2
    )

def drawText(frame, text: str, position: tuple[int, int], size: int, color: tuple[int, int, int], thickness: int):
    cv2.putText(
        frame,
        text,
        position,
        cv2.FONT_HERSHEY_SIMPLEX,
        size,
        color,
        thickness
    )
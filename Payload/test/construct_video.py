import cv2
import glob

fps = 10

files = sorted(glob.glob("../webots/controllers/bot_with_model/aicam/*.jpg"))

# Read first frame to get dimensions
first_frame = cv2.imread(files[0])
height, width, _ = first_frame.shape

writer = cv2.VideoWriter(
    "output.avi",
    cv2.VideoWriter_fourcc(*"MJPG"),
    fps,
    (width, height)
)

for file in files:
    frame = cv2.imread(file)
    writer.write(frame)

writer.release()
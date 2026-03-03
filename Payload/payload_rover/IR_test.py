# Testing for IR Camera


import time
import cv2
import os
import numpy as np
from picamera2 import Picamera2


# Configuration
BRIGHTNESS_THRESHOLD = 180   # IR brightness cutoff (NEEDS TO BE TUNED)
MIN_BRIGHT_RATIO = 0.10      # % of bright pixels required (NEEDS TO BE TUNED)
MAX_CAPTURES = 5
COOLDOWN_SECONDS = 0.75

SAVE_FOLDER = "plant_images"

# Creates folder if doesn't exist (It should)
if not os.path.exists(SAVE_FOLDER):
    os.makedirs(SAVE_FOLDER)

# Camera Setup
picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"format": "RGB888"})
picam2.configure(config)
picam2.start()

time.sleep(2)  

capture_count = 0
last_capture_time = 0

print("Starting IR plant detection test...")

# Main Loop
while capture_count < MAX_CAPTURES:
    frame = picam2.capture_array()

    # Extract RED channel 
    red_channel = frame[:, :, 2]

    # Threshold for bright IR reflection
    _, mask = cv2.threshold(
        red_channel,
        BRIGHTNESS_THRESHOLD,
        255,
        cv2.THRESH_BINARY
    )

    bright_ratio = np.sum(mask > 0) / mask.size

    print(f"Bright ratio: {bright_ratio:.3f}")

    current_time = time.time()

    if bright_ratio > MIN_BRIGHT_RATIO:
        if current_time - last_capture_time > COOLDOWN_SECONDS:
            
            capture_count += 1
            filename = os.path.join(
                SAVE_FOLDER,
                f"plant_{capture_count}.jpg"
            )

            cv2.imwrite(filename, frame)
            print(f"Saved {filename}")

            last_capture_time = current_time

    time.sleep(0.1) 

print("Captured 5 images. Exiting.")

picam2.stop()

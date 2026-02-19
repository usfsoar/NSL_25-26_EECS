from picamera2 import Picamera2, Preview
import time
picam2 = Picamera2()
camera_config = picam2.create_preview_configuration()
picam2.configure(camera_config)
picam2.start_preview(Preview.DRM)
picam2.start()
time.sleep(2)
picam2.capture_file("flash.jpg")
input()
picam2.capture_file("noflash.jpg")

# picam2.start_and_record_video("test.mp4", duration=5)

# from picamera2 import Picamera2
# picam2 = Picamera2()
# picam2.start_and_record_video("test.mp4", duration=5)
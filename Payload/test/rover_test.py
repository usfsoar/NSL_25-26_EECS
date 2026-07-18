import asyncio
from payload_rover.motors import Motor
from payload_rover.motors import DriveController

import cv2
from ultralytics import YOLO

from payload_sensor.vl53l4cx import VL53L4CX

import time
import board


left_motor = Motor(wheel_diameter=0.1, pwm_channel=0, direction_pin=13, output_pin=-1)
right_motor = Motor(wheel_diameter=0.1, pwm_channel=1, direction_pin=12, output_pin=-1)

drive = DriveController(left_motor, right_motor)

i2c = board.I2C()
vl53 = VL53L4CX(i2c)
vl53.distance_mode = 2
vl53.timing_budget = 50

print("VL53L4CX Simple Test.")
print("--------------------")
model_id, module_type, mask_rev = vl53.model_info
print(f"Model ID: 0x{model_id:0X}")
print(f"Module Type: 0x{module_type:0X}")
print(f"Mask Revision: 0x{mask_rev:0X}")
print("Distance Mode: ", end="")
if vl53.distance_mode == 1:
    print("SHORT")
elif vl53.distance_mode == 2:
    print("LONG")
else:
    print("UNKNOWN")
print(f"Timing Budget: {vl53.timing_budget}")
print("--------------------")

vl53.start_ranging()



model = YOLO("yolo_200epoch.pt") 

cap = cv2.VideoCapture(0)

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)

prev_time = 0

def setup():
    return

async def rover():
    while cap.isOpened():
            success, frame = cap.read()
            if not success:
                break

            results = model.predict(frame, conf=0.3, imgsz=160, verbose=False)

            if (len(results[0].boxes) > 0) or (vl53.data_ready and vl53.distance < 50):
                print("object detected")
                drive.stop()
            else:
                drive.move_forward(speed=100)
            
            names = model.names

            for result in results:
                for box in result.boxes:
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    
                    conf = box.conf[0]
                    cls_id = int(box.cls[0])
                    label = f"{names[cls_id]} {conf:.2f}"

                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                    tf = max(1, int(1))
                    t_size = cv2.getTextSize(label, 0, fontScale=0.5, thickness=tf)[0]
                    c2 = x1 + t_size[0], y1 - t_size[1] - 3
                    
                    cv2.rectangle(frame, (x1, y1), c2, (0, 255, 0), -1)
                    cv2.putText(frame, label, (x1, y1 - 2), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), thickness=tf, lineType=cv2.LINE_AA)

            curr = time.time()
            fps = 1 / (curr - prev_time)
            prev_time = curr
            cv2.putText(frame, f"FPS: {int(fps)}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)

            vl53.clear_interrupt()
            time.delay(0.01)

    cap.release()
    cv2.destroyAllWindows()

async def main():
    await asyncio.gather(rover())

if __name__ == '__main__':
    setup()
    asyncio.run(main())

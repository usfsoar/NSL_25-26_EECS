import numpy as np
import adafruit_mlx90640

THERMAL_CAM_WIDTH = 32
THERMAL_CAM_HEIGHT = 24
SKY_THRESHOLD = None

class ThermalCam:
    def __init__(self):
        mlx = adafruit_mlx90640.MLX90640(i2c)

    def getFrame(self) -> list[int]:

        frame = [0] * THERMAL_CAM_HEIGHT * THERMAL_CAM_WIDTH

        try:
            mlx.getFrame(frame)
            return frame
        except ValueError:
            return None


def crop_array(frame: list[int], crop_box: tuple[int, int, int, int]) -> list[list[int]]:
    thermal_frame = np.array(thermal_frame).reshape(THERMAL_CAM_HEIGHT, THERMAL_CAM_WIDTH)

    row_crop_first, row_crop_last = (crop_box[1] + crop_box[3]) * THERMAL_CAM_HEIGHT, (crop_box[1] - crop_box[3]) * THERMAL_CAM_HEIGHT
    col_crop_first, col_crop_last = (crop_box[0] - crop_box[2]) * THERMAL_CAM_WIDTH, (crop_box[0] + crop_box[2]) * THERMAL_CAM_WIDTH

    thermal_frame = thermal_frame[row_crop_first:row_crop_last, col_crop_first:col_crop_last]

    return thermal_frame


# ATCD = Air-to-Canopy Differential
def calculate_ATCD(thermal_frame: list[int], bounding_box: tuple[int, int, int, int]) -> float:
    cropped_frame = crop_array(thermal_frame, bounding_box)

    ambient_temp = PLACEHOLDER_GET_TEMPERATURE()

    for i in len(cropped_frame):
        for j in len(cropped_frame[i]):
            if cropped_frame[i][j] < SKY_THRESHOLD:
                cropped_frame[i][j] = 0

    plant_temp_arr = 
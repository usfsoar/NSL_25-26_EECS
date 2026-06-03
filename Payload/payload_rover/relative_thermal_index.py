import numpy as np
import adafruit_mlx90640

from payload_sensor.bmp580 import BMP

_ALPHA_BMP = 0.8
_bmp: BMP | None = None

THERMAL_CAM_WIDTH = 32
THERMAL_CAM_HEIGHT = 24
SKY_THRESHOLD = None

class ThermalCam:
    def __init__(self):
        i2c = board.I2C(board.SCL, board.SDA)
        mlx = adafruit_mlx90640.MLX90640(i2c)

    def getFrame(self) -> list[int]:

        frame = [0] * THERMAL_CAM_HEIGHT * THERMAL_CAM_WIDTH

        try:
            self.mlx.getFrame(frame)
            return frame
        except ValueError:
            return None


def crop_array(frame: list[float], crop_box: tuple[int, int, int, int]) -> np.ndarray:
    """
    bbox = (x1, y1, x2, y2)

    Coordinates are already translated into
    MLX90640 pixel coordinates.
    """
    thermal_frame = np.array(thermal_frame).reshape(THERMAL_CAM_HEIGHT, THERMAL_CAM_WIDTH)
    x1, y1, x2, y2 = crop_box
    x1 = max(0, min(x1, THERMAL_CAM_WIDTH))
    y1 = max(0, min(y1, THERMAL_CAM_HEIGHT))
    x2 = max(0, min(x2, THERMAL_CAM_WIDTH))
    y2 = max(0, min(y2, THERMAL_CAM_HEIGHT))

    return thermal_frame[y1:y2, x1:x2]

    # row_crop_first, row_crop_last = (crop_box[1] + crop_box[3]) * THERMAL_CAM_HEIGHT, (crop_box[1] - crop_box[3]) * THERMAL_CAM_HEIGHT
    # col_crop_first, col_crop_last = (crop_box[0] - crop_box[2]) * THERMAL_CAM_WIDTH, (crop_box[0] + crop_box[2]) * THERMAL_CAM_WIDTH

    # thermal_frame = thermal_frame[row_crop_first:row_crop_last, col_crop_first:col_crop_last]

    # return thermal_frame

def _get_bmp() -> BMP:
    global _bmp
    if _bmp is None:
        _bmp = BMP()
        _bmp.initialize(_ALPHA_BMP)
    return _bmp


def get_ambient_temperature() -> float:
    """Returns ambient temperature in Celsius from the BMP580 barometer."""
    temperature, _ = _get_bmp().get_temperature()
    return temperature

# ATCD = Air-to-Canopy Differential 
def calculate_ATCD(thermal_frame: list[float], bounding_box: tuple[int, int, int, int]) -> float | None:
    cropped_frame = crop_array(thermal_frame, bounding_box)

    if cropped_frame == 0:
        return None

    ambient_temp = get_ambient_temperature()

    # Remove sky and backgroundpixels
    plant_pixels = cropped_frame[cropped_frame > SKY_THRESHOLD]

    if plant_pixels.size == 0:
        return None

    average_plant_temp = np.mean(plant_pixels)

    return float(ambient_temp - average_plant_temp)

   """ 
   for i in len(cropped_frame):
        for j in len(cropped_frame[i]):
            if cropped_frame[i][j] < SKY_THRESHOLD:
                cropped_frame[i][j] = 0
   
    plant_temp_arr =
    """

class RTI:
    def __init__():
        pass

    def calculate_rti(self, thermal_frame, ai_frame, temp):
        """
        calculates the relative thermal index for a given thermal frame, AI classification frame, and temperature data
        returns rti value of the frame
        """
        
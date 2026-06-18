import board
import busio
import numpy as np
import adafruit_mlx90640
import cv2
import os
import time

class MLX90640Camera:
    def __init__(
        self,
        save_folder="thermal_images",
        refresh_rate=adafruit_mlx90640.RefreshRate.REFRESH_2_HZ,
        i2c_frequency=400000
    ):
        """
        Initialize the MLX90640 thermal camera.

        Parameters:
            save_folder (str): Directory for saved images.
            refresh_rate: MLX90640 refresh rate.
            i2c_frequency (int): I2C bus speed in Hz.
        """
        self.save_folder = save_folder
        os.makedirs(self.save_folder, exist_ok=True)

        for _ in range(8):
            try: 
                # Initialize I2C
                self.i2c = busio.I2C(
                    board.SCL,
                    board.SDA,
                    frequency=i2c_frequency
                )
            except Exception as e:
                print(f"MLX I2C Init Exception: {e}")

        for _ in range(8):
            try:
                # Initialize sensor
                self.sensor = adafruit_mlx90640.MLX90640(self.i2c)
                self.sensor.refresh_rate = refresh_rate
            except Exception as e:
                print(f"MLX Sensor Init Exception: {e}")
        
        # Allocate frame buffer
        self.frame = [0] * 768  # 24 x 32 pixels

        print("MLX90640 initialized.")
        print(f"Serial number: {self.sensor.serial_number}")

    def capture_frame(
        self,
        filename=None,
        colormap=cv2.COLORMAP_INFERNO,
        output_size=(320, 240)
    ):
        """
        Capture a thermal frame and save it as an image.

        Parameters:
            filename (str): Output filename. If None, generates one.
            colormap: OpenCV colormap to apply.
            output_size (tuple): Width and height of saved image.

        Returns:
            str: Path to saved image.
            np.ndarray: Raw temperature data (24 x 32) in °C.
        """

        # Read frame (retry on occasional I2C errors)
        while True:
            try:
                self.sensor.getFrame(self.frame)
                break
            except ValueError:
                continue

        thermal = np.array(self.frame).reshape((24, 32))

        # Normalize temperatures to grayscale
        img = cv2.normalize(
            thermal,
            None,
            0,
            255,
            cv2.NORM_MINMAX
        )

        img = img.astype(np.uint8)

        # Resize for better visibility
        img = cv2.resize(
            img,
            output_size,
            interpolation=cv2.INTER_CUBIC
        )

        # Apply color map
        color_img = cv2.applyColorMap(img, colormap)

        # Generate filename if needed
        if filename is None:
            timestamp = int(time.time())
            filename = f"thermal_{timestamp}.jpg"

        filepath = os.path.join(self.save_folder, filename)

        cv2.imwrite(filepath, color_img)

        print(
            f"Saved {filepath} | "
            f"Min: {thermal.min():.1f}°C | "
            f"Max: {thermal.max():.1f}°C"
        )

        return filepath, thermal

    def close(self):
        """Release I2C resources."""
        try:
            self.i2c.deinit()
        except Exception:
            pass

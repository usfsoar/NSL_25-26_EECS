import cv2
import os

# class FrameBrightness:
#     def __init__(self, threshold=75, white_ratio_limit=0.4):
#         self.threshold = threshold
#         self.white_ratio_limit = white_ratio_limit
    
#     def brightness(self, frame):
#         gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
#         _, binary = cv2.threshold(gray, self.threshold, 255, cv2.THRESH_BINARY)

#         total_pixels = binary.size
#         white_pixels = cv2.countNonZero(binary)
#         white_ratio = white_pixels / total_pixels
        
#         print(f"{white_ratio:.2%}")
#         if white_ratio > self.white_ratio_limit:
#             print(f"{white_ratio:.2%}")
#             #return bright
#             return 1
#         else:
#              #return 0
#             return 0

def test_image_brightness(file_path, threshold_value=75, white_pixel_ratio_limit=0.4):

    # Load the image in Grayscale
    # This automatically strips color and leaves pixel values from 0 (black) to 255 (white).
    gray_img = cv2.imread(file_path, cv2.IMREAD_GRAYSCALE)

    # Apply Binarization (Thresholding)
    # Any pixel brighter than threshold_value becomes 255 (white), others become 0 (black).
    _, binary_img = cv2.threshold(gray_img, threshold_value, 255, cv2.THRESH_BINARY)

    # 4. Calculate the ratio of white pixels
    total_pixels = binary_img.size
    white_pixels = cv2.countNonZero(binary_img)
    white_ratio = white_pixels / total_pixels

    # 5. Determine State
    # If even a small percentage of the image is bright, it indicates ejection.
    if white_ratio > white_pixel_ratio_limit:
        state = "BRIGHT (Ejected / External Environment)"
    else:
        state = "DARK (Inside Rocket Tube)"

    # Print results to console
    print(f"--- Test Results for: {os.path.basename(file_path)} ---")
    print(f"White Pixel Ratio: {white_ratio:.2%}")
    print(f"Detected State   : {state}")
    print("-" * 40)

# --- Example Usage ---
if __name__ == "__main__":
    # Replace this with the path to your test image
    image_location = r"C:\Users\samjo\Downloads\detect_pics\22836.jpg"
    
    test_image_brightness(image_location)
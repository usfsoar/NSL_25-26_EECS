import network
import socket
import time
import requests

def connect_to_esp32(ssid="SoarMonkey", password="IAmAMonkey"):
    """Connect to ESP32 WiFi Access Point"""
    
    # Try to connect to ESP32 WiFi
    max_retries = 5
    retry_count = 0
    
    while retry_count < max_retries:
        try:
            # Send data to ESP32's web server
            url = "http://192.168.4.1/send-data"  # ESP32's default AP IP
            data = {"data": "Hello from Pi"}
            
            response = requests.post(url, data=data)
            
            if response.status_code == 200:
                print("Successfully connected and sent data to ESP32")
                return True
            else:
                print(f"Failed to send data, status code: {response.status_code}")
                
        except Exception as e:
            print(f"Connection attempt {retry_count + 1} failed: {str(e)}")
            retry_count += 1
            time.sleep(2)  # Wait before retrying
            
    print("Failed to connect after maximum retries")
    return False

def send_data_to_esp32(data):
    """Send data to ESP32 web server"""
    count = 0
    
    try:
        url = "http://192.168.4.1/send-data"
        payload = {"data": data}
        response = requests.post(url, data=payload)
        return response.status_code == 200
    except:
        return False

if __name__ == "__main__":
    # Connect to ESP32
    if connect_to_esp32():
        # Send periodic data
        while True:
            if send_data_to_esp32(data) & count < 5:
                print("Data sent successfully")
            elif (count < 5):
                print("Failed to send data")
                count += 1
            else:
                print("Max attempts reached, stopping data send")
                break
            time.sleep(1)  # Wait 1 second between sends
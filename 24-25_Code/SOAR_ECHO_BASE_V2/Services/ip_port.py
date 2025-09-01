import socket
import os

def get_ip_address():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]

def display_ip_address():
    ip_address = get_ip_address()
    port = 5000  
    print(f"Access the site at: http://{ip_address}:{port}/")
    
def get_port():
    return os.getenv("PORT", default=5000)
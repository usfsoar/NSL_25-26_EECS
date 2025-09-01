import serial
import time

# Set up Serial connection (Update 'COM3' based on your system)
arduino = serial.Serial(port='/dev/cu.usbmodem101', baudrate=9600, timeout=1)
time.sleep(2)  # Wait for Arduino to stabilize

while True:
    user_input = input("Enter command (on/off/n to exit): ").strip().lower()

    if user_input not in ["on", "off", "n"]:
        print("Invalid command! Use 'on', 'off', or 'n'.")
        continue  # Ask for input again

    arduino.write((user_input + "\n").encode())  # Send command to Arduino
    time.sleep(0.1)  # Allow time for response

    response = arduino.readline().strip()  # Read response
    print("Arduino:", response)

    if user_input == "n":  # Exit when "N" is entered
        break

arduino.close()  # Close Serial connection
print("Connection closed.")


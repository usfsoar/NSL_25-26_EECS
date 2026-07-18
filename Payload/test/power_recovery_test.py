from payload_main import power_loss_recovery
import time
import os

data = {
    "time": 0,
    "timestamp": 0,
    "state": "READY",
    "raw_g_force": 0,
    "g_force": 0,
    "raw_altitude": 0,
    "altitude": 0,
    "raw_velocity": 0,
    "velocity": 0,
    "apogee": 0
    }

with open(".running.txt", "w") as file:
    file.write("sensor_data_21:51:17.979564.csv")

print(power_loss_recovery())

# time.sleep(10)


if os.path.exists(".running.txt"):
        os.remove(".running.txt")
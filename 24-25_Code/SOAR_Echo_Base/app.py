import sys
import threading
from Config import *
from Controllers import control_panel, gps_controller
import os
import sys
import argparse

# Route for serving JS and CSS files

if __name__=="__main__":
    parser = argparse.ArgumentParser(description="SOAR Echo Base Main Program: Ground station")
    parser.add_argument("--fake-serial", action='store_true', help="Fake the serial data for development purposes")
    args = parser.parse_args()

    fake_serial = args.fake_serial
    if fake_serial:
        print("FAKE SERIAL ACTIVE ! --------------------------")

    try:
        # Server setup
        # server_thread = threading.Thread(target=run_server)
        # server_thread.daemon = True
        # server_thread.start()
        run_server()
        
        # Other processes
        while True:
            pass
    except KeyboardInterrupt:
        print("Ctrl+C pressed shutting down")
        sys.exit()
    except Exception as e:
        print(f'Exception: {e}')
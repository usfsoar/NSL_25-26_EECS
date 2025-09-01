import time
import random
import math
from digital_twin import *
from config import USE_SIM

def setup():
    if USE_SIM:
        setup_sim()
    else:
        print("Setting up real hardware...")

def convert_Flap_Length_to_Angle(flaps_length):
    a = -0.0002
    b = 0.0309
    c = -(0.0877 + flaps_length)

    discriminant = b**2 - 4*a*c
    try:
        angle1 = round((-b + math.sqrt(discriminant)) / (2 * a), 1)
        angle2 = round((-b - math.sqrt(discriminant)) / (2 * a), 1)
    except:
        return 2.9

    if 2.9 <= angle1 <= 39.63:
        return angle1
    else:
        return angle2

def getAltitude():
    if USE_SIM:
        return getSimulatedAltitude()
    else:
        return 123.45

def setServo(angle):
    if USE_SIM:
        return setSimulatedAngle(angle)
    else:
        print(f"[REAL] Set angle to {angle}")
        return True

def getAcceleration():
    if USE_SIM:
        return getSimulatedAcceleration()
    else:
        return 0,0,-9.8
    

def main():
    setup()
    min_length = 0.0
    max_length = 0.7
    last_sent = 0

    while True:
        try:
            altitude = getAltitude()
            print(f"ALT: {altitude:.2f}")
            _,_,z = getAcceleration()
            print(f"ACC: {z}")

            if time.time() - last_sent > 2:
                flap_length = random.uniform(min_length, max_length)
                angle = int(convert_Flap_Length_to_Angle(flap_length))
                confirmed = setServo(angle)

                print(f"Sent flap length: {flap_length:.3f} → angle: {angle} {'✅' if confirmed else '❌'}")
                last_sent = time.time()

            time.sleep(0.2)

        except Exception as e:
            print("❌ Error:", e)
            time.sleep(1)

if __name__ == "__main__":
    main()

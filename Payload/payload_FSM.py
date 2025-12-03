#!/usr/bin/env python3
"""
States:
  0 = Ready / Pre-launch
  1 = Launched / Ascent
  2 = Descending
  3 = Landed
"""

import time
import math

import board
import gpiozero as GPIO
import bmp580
import time
import bno055

bno = bno055.BNO()
bmp = bmp580.BMP()
BMPstatus = True
BNOstatus = True
state = 0

def initialize():
    try:
        initializeBMP = bmp.initialize()
    except Exception as e:
        BMPstatus = False
        print(e)
        
    try: 
        initializeBNO = bno.initialize()
    except Exception as e:
        BMPstatus = False
        print(e)

    
    
    return  
# ------------------------ Tunables (copy from 24 code) ------------------------

HZ = 20.0          # sample rate 
DT = 1.0 / HZ

# Thresholds (from your C++ comments / constants)
LAUNCH_ACCEL_THRESHOLD      = 1.5   # EMA G-force > 1.5 G
LAUNCH_ALTITUDE_THRESHOLD   = 10.0  # m AGL
DESCENT_ALTITUDE_THRESHOLD  = 10.0  # m below apogee to call descent
DESCENT_APOGEE_THRESHOLD    = 5.0   # m minimum apogee to care
LANDING_ACCEL_THRESHOLD     = 0.2   # |EMA G - 1| < 0.2
LANDING_VEL_THRESHOLD       = 0.8   # m/s
LANDING_ALTITUDE_THRESHOLD  = 3.0   # m

STABLE_READINGS_FOR_LAUNCH  = 3
STABLE_READINGS_FOR_DESCENT = 3
STABLE_READINGS_FOR_LANDING = 10
STABLE_READINGS_FOR_LANDING_VG = 3

STATE_TIMEOUT_ALL           = 300.0 # 5 min safety

# EMA alphas
ALPHA_GFORCE   = 0.8
ALPHA_ALTITUDE = 0.5
ALPHA_VELOCITY = 0.8

# ------------------------ States ------------------------

STATE_READY      = 0
STATE_LAUNCHED   = 1
STATE_DESCENDING = 2
STATE_LANDED     = 3

def main():
 

    # EMAs 
    ema_g   = 1.0
    ema_alt = 0.0
    ema_vel = 0.0

    apogee = 0.0
    descent_started = False

    state = STATE_READY
    flight_start_time = None

    # counters
    stable_launch  = 0
    stable_descent = 0
    stable_land    = 0
    stable_land_vg = 0

    landing_velocity = 0.0
    landing_gforce   = 0.0

    try:
        while True:
            # --- sensor read ---
            g_force = bno.get_linear_acceleration()[2] / 9.81  # in g's
            alt = bmp.get_altitude()
            vel_z  = bmp.get_vertical_velocity()

            # --- EMAs ---
            ema_g   = ALPHA_GFORCE   * g_force     + (1 - ALPHA_GFORCE)   * ema_g
            ema_alt = ALPHA_ALTITUDE * alt         + (1 - ALPHA_ALTITUDE) * ema_alt
            ema_vel = ALPHA_VELOCITY * abs(vel_z)  + (1 - ALPHA_VELOCITY) * ema_vel

            # update apogee
            if ema_alt > apogee:
                apogee = ema_alt

            now = time.time()

            # global timeout for launched/descending
            if state in (STATE_LAUNCHED, STATE_DESCENDING) and flight_start_time is not None:
                if (now - flight_start_time) > STATE_TIMEOUT_ALL:
                    print("\n[Timeout] 5 min exceeded, forcing Landed.")
                    state = STATE_LANDED

            # ------------- FSM ------------- #
            if state == STATE_READY:
                # 0 -> 1: launch if EMA G or EMA Altitude crosses threshold for N samples
                if (ema_g > LAUNCH_ACCEL_THRESHOLD) or (ema_alt > LAUNCH_ALTITUDE_THRESHOLD):
                    stable_launch += 1
                else:
                    stable_launch = 0

                if stable_launch >= STABLE_READINGS_FOR_LAUNCH:
                    print("\n*** LAUNCH DETECTED (0 -> 1) ***")
                    state = STATE_LAUNCHED
                    flight_start_time = now
                    stable_descent = stable_land = stable_land_vg = 0

            elif state == STATE_LAUNCHED:
                # 1 -> 2: apogee reached + we are DESCENT_ALTITUDE_THRESHOLD below it (EMA)
                cond_apogee_min = apogee > DESCENT_APOGEE_THRESHOLD
                cond_below = ema_alt < (apogee - DESCENT_ALTITUDE_THRESHOLD)

                if (not descent_started) and cond_apogee_min and cond_below:
                    stable_descent += 1
                else:
                    stable_descent = 0

                if stable_descent >= STABLE_READINGS_FOR_DESCENT:
                    print("\n*** DESCENT STARTING (1 -> 2), apogee ≈ "
                          f"{apogee:.1f} m ***")
                    state = STATE_DESCENDING
                    descent_started = True
                    stable_land = stable_land_vg = 0

            elif state == STATE_DESCENDING:
                # 2 -> 3: landing — low altitude, low velocity, near 1g
                cond_vel = (ema_vel < LANDING_VEL_THRESHOLD)
                cond_g   = (abs(ema_g - 1.0) < LANDING_ACCEL_THRESHOLD)
                cond_alt = (ema_alt < LANDING_ALTITUDE_THRESHOLD)

                if cond_vel and cond_g and cond_alt:
                    stable_land += 1
                    stable_land_vg += 1
                else:
                    stable_land = 0
                    stable_land_vg = 0

                # track approximate max landing vel / g
                if stable_land_vg >= STABLE_READINGS_FOR_LANDING_VG:
                    landing_velocity = max(landing_velocity, ema_vel)
                    landing_gforce   = max(landing_gforce, ema_g)

                if stable_land >= STABLE_READINGS_FOR_LANDING:
                    print("\n*** LANDING DETECTED (2 -> 3) ***")
                    print(f"  Apogee:          {apogee:.2f} m")
                    print(f"  Land vel (EMA):  {landing_velocity:.2f} m/s")
                    print(f"  Land g (EMA):    {landing_gforce:.2f} g")
                    state = STATE_LANDED

            elif state == STATE_LANDED:
                #Deploy rover...
                pass

            # ------------- Debug print ------------- #
            print(
                f"st={state} alt={ema_alt:7.2f} apg={apogee:7.2f} "
                f"vel={ema_vel:5.2f} g={ema_g:4.2f}",
                end="\r"
            )

            time.sleep(DT)

    except KeyboardInterrupt:
        print("\nExiting flight FSM.")


main()

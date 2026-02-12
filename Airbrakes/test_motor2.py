"""
Demo file for basic movement
"""

from tmc_driver import (
    Tmc2209,
    Loglevel,
    Board,
    tmc_gpio,
    MovementAbsRel,
    TmcEnableControlPin,
    TmcMotionControlStepDir,
)
from tmc_driver.com import TmcComUart

import serial
import time


print("---")
print("SCRIPT START")
print("---")


# -----------------------------------------------------------------------
# initiate the Tmc2209 class
# use your pins for pin_en, pin_step, pin_dir here
# -----------------------------------------------------------------------
UART_PORT = {
    Board.RASPBERRY_PI: "/dev/serial0",
    Board.RASPBERRY_PI5: "/dev/ttyAMA0",
    Board.NVIDIA_JETSON: "/dev/ttyTHS1",
}

ser = serial.Serial("/dev/serial0", baudrate=19200, timeout=1)
time.sleep(10)
tmc = Tmc2209(
    TmcEnableControlPin(16),
    TmcMotionControlStepDir(24, 23),
    TmcComUart(UART_PORT.get(tmc_gpio.BOARD, "/dev/serial0")),
    loglevel=Loglevel.DEBUG,
)


tmc.set_home()
tmc.movement_abs_rel = MovementAbsRel.ABSOLUTE

tmc.set_direction_reg(False)
tmc.set_current_rms(600)
tmc.set_interpolation(True)
tmc.set_spreadcycle(False)
tmc.set_microstepping_resolution(8)
tmc.set_internal_rsense(False)

tmc.acceleration_fullstep = 750
tmc.max_speed_fullstep = 250

tmc.set_motor_enabled(True)

tmc.run_to_position_fullsteps(300)  # move to position 200 (fullsteps)
tmc.run_to_position_fullsteps(0)  # move to position 0

tmc.set_motor_enabled(False)

del tmc
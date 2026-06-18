# SPDX-FileCopyrightText: 2025 Liz Clark for Adafruit Industries
#
# SPDX-License-Identifier: MIT

import time
import board
from digitalio import DigitalInOut, Direction

# direction and step pins as outputs
DIR = DigitalInOut(board.D23)
DIR.direction = Direction.OUTPUT
STEP = DigitalInOut(board.D24)
STEP.direction = Direction.OUTPUT
MS3 = DigitalInOut(board.D25)
MS3.direction = Direction.OUTPUT
MS3.value = False
RST = DigitalInOut(board.D1)
RST.direction = Direction.OUTPUT
RST.value = True
SLP = DigitalInOut(board.D15)
SLP.direction = Direction.OUTPUT
SLP.value = True
EN = DigitalInOut(board.D16)
EN.direction = Direction.OUTPUT
EN.value = False


# microstep mode, default is 1/8 so 8
# another ex: 1/16 microstep would be 16
microMode = 1
# full rotation multiplied by the microstep divider
steps = 200 * microMode
DIR.value = False

currentpos = 0
while True:
    # change direction every loop
    DIR.value = not DIR.value
    print(DIR.value)
    # toggle STEP pin to move the motor
    for i in range(200*steps):
        STEP.value = True
        time.sleep(0.002)
        STEP.value = False
        time.sleep(0.002)
        if i % microMode == 0:
            currentpos += 1
    print(currentpos)
    print("rotated! now reverse")
    # 1 second delay before starting again
    time.sleep(1)

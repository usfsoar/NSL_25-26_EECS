# SPDX-FileCopyrightText: 2025 Liz Clark for Adafruit Industries
#
# SPDX-License-Identifier: MIT

import time
import curses
import board
from digitalio import DigitalInOut, Direction

DIR = DigitalInOut(board.D23)
DIR.direction = Direction.OUTPUT

STEP = DigitalInOut(board.D24)
STEP.direction = Direction.OUTPUT

currentpos = 0
minSteps = 0
maxSteps = 6000

stepsPerPress = 50
step_delay = 0.00005

def pulse_step():
    STEP.value = True
    time.sleep(step_delay)
    STEP.value = False
    time.sleep(step_delay)

def move_motor(direction, amount):
    global currentpos

    if direction == "w":
        stepsToMove = min(amount, maxSteps - currentpos)
        DIR.value = True
        for i in range(stepsToMove):
            pulse_step()
        currentpos += stepsToMove

    elif direction == "s":
        stepsToMove = min(amount, currentpos - minSteps)
        DIR.value = False
        for i in range(stepsToMove):
            pulse_step()
        currentpos -= stepsToMove

def main(stdscr):
    curses.cbreak()
    stdscr.nodelay(True)
    stdscr.keypad(True)
    stdscr.clear()

    stdscr.addstr(0, 0, "Press/hold W to extend, S to retract, Q to quit.")
    stdscr.addstr(1, 0, f"Each key event moves {stepsPerPress} steps.")

    while True:
        key = stdscr.getch()

        if key == ord("q"):
            break
        elif key == ord("w"):
            move_motor("w", stepsPerPress)
        elif key == ord("s"):
            move_motor("s", stepsPerPress)

        stdscr.addstr(3, 0, f"Position: {currentpos} steps      ")
        stdscr.refresh()
        time.sleep(0.001)

try:
    curses.wrapper(main)
finally:
    DIR.deinit()
    STEP.deinit()

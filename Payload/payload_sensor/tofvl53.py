# SPDX-FileCopyrightText: 2017 Scott Shawcroft, written for Adafruit Industries
# SPDX-FileCopyrightText: Copyright (c) 2021 Carter Nelson for Adafruit Industries
# SPDX-FileCopyrightText: Copyright (c) 2025 Pat Daderko
#
# SPDX-License-Identifier: Unlicense

# Simple demo of the VL53L4CX distance sensor.
# Will repeatedly print the sensed range/distance, sigma, and status.

import time

import board

import vl53l4cx

i2c = board.I2C()  # uses board.SCL and board.SDA
#i2c = board.STEMMA_I2C()  # For using the built-in STEMMA QT connector on a microcontroller

vl53 = vl53l4cx.VL53L4CX(i2c)

# OPTIONAL: can set non-default values
vl53.distance_mode = 2
vl53.timing_budget = 50

print("VL53L4CX Simple Test.")
print("--------------------")
model_id, module_type, mask_rev = vl53.model_info
print(f"Model ID: 0x{model_id:0X}")
print(f"Module Type: 0x{module_type:0X}")
print(f"Mask Revision: 0x{mask_rev:0X}")
print("Distance Mode: ", end="")
if vl53.distance_mode == 1:
    print("SHORT")
elif vl53.distance_mode == 2:
    print("LONG")
else:
    print("UNKNOWN")
print(f"Timing Budget: {vl53.timing_budget}")
print("--------------------")

vl53.start_ranging()

while True:
    if vl53.data_ready:
        print(f"Dist: {vl53.distance} cm, Sig: {vl53.sigma} cm, Stat: {vl53.range_status}")
        vl53.clear_interrupt()
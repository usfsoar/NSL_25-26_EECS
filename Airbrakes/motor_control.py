# Class which will contain functions to move the motor
import time
import board
from digitalio import DigitalInOut, Direction

try:                             #tmc import checks
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
    tmc = True
except:
    tmc = False
    print("TMC import failed")



class Motor():
    def __init__(self):
        self.current_pos = 0
        self.micro_mode = 8

        if tmc:
            try:
                UART_PORT = {
                Board.RASPBERRY_PI: "/dev/serial0",
                Board.RASPBERRY_PI5: "/dev/ttyAMA0",
                Board.NVIDIA_JETSON: "/dev/ttyTHS1",
                }

                self.ser = serial.Serial("/dev/serial0", baudrate=19200, timeout=1)
                time.sleep(10)
                self.tmc = Tmc2209(
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
                tmc.set_microstepping_resolution(self.micro_mode)
                tmc.set_internal_rsense(False)
                tmc.acceleration_fullstep = 750
                tmc.max_speed_fullstep = 250
                tmc.set_motor_enabled(True)

                self.method = 2
                print("TMC Initialized")
                return
            except Exception as e:
                print(f"TMC initialization failed:{e}")
            
            self.method = 1
            self.dir = DigitalInOut(board.D5)
            self.dir.direction = Direction.OUTPUT
            self.step = DigitalInOut(board.D6)
            self.step.direction = Direction.OUTPUT
            print("Basic GPIO initialized")

    def move_to(self, target_fullsteps):
        if self.method == 2:
            self.tmc.run_to_position_fullsteps(target_fullsteps)
        else:
            diff = target_fullsteps - self.current_pos
            self.dir.value = (diff > 0) #true when out direction, false when in direction

            steps = abs(diff) * self.micro_mode
            for i in range(steps):
                self.step.value = True
                time.sleep(0.001)
                self.step.value = False
                time.sleep(0.001)
            
            self.current_pos = target_fullsteps



if __name__ == '__main__':
    motor = Motor()
    motor.move_to(200)
    time.sleep(1)
    motor.move_to(0)
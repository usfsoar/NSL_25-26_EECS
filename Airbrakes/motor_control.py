# Class which will contain functions to move the motor
import time
import board
from digitalio import DigitalInOut, Direction
import serial
import threading

class Motor():
    def __init__(self):
        self.current_pos = 0
        self.micro_mode = 8
        self._stop_event = threading.Event()
        self._move_thread = None

        
        self.dir = DigitalInOut(board.D23)
        self.dir.direction = Direction.OUTPUT
        self.step = DigitalInOut(board.D24)
        self.step.direction = Direction.OUTPUT
        self.max_step = 6000
        print("Basic GPIO initialized")

    def move_to(self, target_fullsteps):
        if self._move_thread and self._move_thread.is_alive():
            self._stop_event.set() 
            self._move_thread.join()
        
        self._stop_event.clear()
        
        self._move_thread = threading.Thread(
            target=self._execute_move, 
            args=(target_fullsteps,)
        )
        self._move_thread.start()

    def _execute_move(self, target_fullsteps):
        diff = target_fullsteps - self.current_pos
        self.dir.value = (diff > 0)
        steps_to_go = abs(diff) * self.micro_mode
        
        for i in range(steps_to_go):
            if self._stop_event.is_set():
                print("Movement Interrupted!")
                break
                
            self.step.value = True
            time.sleep(0.001)
            self.step.value = False
            time.sleep(0.001)
            
            step_direction = 1 if diff > 0 else -1
            if i % self.micro_mode == 0:
                self.current_pos += step_direction
    def stop_now(self):
        self._stop_event.set()



if __name__ == '__main__':
    motor = Motor()
    motor.move_to(200)
    time.sleep(1)
    motor.move_to(0)
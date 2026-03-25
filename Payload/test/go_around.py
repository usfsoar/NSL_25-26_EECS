#import classes
from payload_pipeline.telemetry_logger import TelemetryLogger
from payload_sensor.sr04 import DistanceSensor
from payload_sensor.bno085 import BNO
from payload_rover.rover_control import RoverControl
from payload_rover.motor_control import MotorControl

ROVER_SCAN_TIMEOUT = 900
ROVER_EXIT_TIMEOUT = 60

log = TelemetryLogger()
tof = DistanceSensor()
motors = MotorControl(pins=[1, 2, 3, 4])
bno = BNO()
rover = RoverControl(motors, tof, bno, ROVER_SCAN_TIMEOUT, ROVER_EXIT_TIMEOUT)

rover.exit_rover()
rover.do_scan_2()
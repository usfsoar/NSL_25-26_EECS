from rpi_hardware_pwm import HardwarePWM
from gpiozero import DigitalOutputDevice, DigitalInputDevice
from time import sleep

# pigpio attempt. need pigpio demon running and also you need to remove all the gpiozero code below and use pigpio instead. 
# Both had the same problems lol
# import pigpio

# pi = pigpio.pi()
# if not pi.connected:
#     print("Error: pigpio daemon not running")
#     exit(1)

# def cbf(gpio, level, tick):
#     global count
#     count += 1

# pi.callback(HALL_PIN, pigpio.RISING_EDGE, cbf)


# --- Motor Initialization ---
pwmRight = HardwarePWM(pwm_channel=0, hz=25000)
Right_Motor_Direction = DigitalOutputDevice(19) # GPIO 19 (pin (36?))

# --- Hall Sensor Initialization ---
# Using gpiozero instead of pigpio. pull_up=True mimics pigpio.PUD_UP
HALL_PIN = 21 # GPIO 21 (pin (40?))
hall_sensor = DigitalInputDevice(HALL_PIN, pull_up=True, bounce_time=0.001) 
count = 0

# Callback function to count pulses
def cbf():
    global count
    count += 1

def setup():
    # Start PWM with 100% duty cycle (Off usually, depending on your motor driver)
    pwmRight.start(100) 
    
    # Attach the callback function to the falling edge of the sensor pulse
    hall_sensor.when_activated = cbf 
    
    sleep(1)
    
def loop():
    global count
    prev_count  = 0
    while True:
        # Right Motor Test 
        print(f'pin 12 fast! V')
        pwmRight.change_duty_cycle(0)
        sleep(5)
        delta = max(0, count - prev_count)
        prev_count = count
        print(f'hall pulse count: {delta}')
        count = 0

        print(f'pin 12 slow! V')
        pwmRight.change_duty_cycle(50)
        print('pin 36 reverse!')
        Right_Motor_Direction.on()
        sleep(5)
        delta = max(0, count - prev_count)
        prev_count = count
        print(f'hall pulse count: {delta}')
        count = 0

        print(f'pin 12 off! V')
        pwmRight.change_duty_cycle(100)
        print('pin 36 forward!')
        Right_Motor_Direction.off()
        sleep(5)
        delta = max(0, count - prev_count)
        prev_count = count
        print(f'hall pulse count: {delta}')
        count = 0

if __name__ == '__main__':
    setup()
    try:
        loop()
    except KeyboardInterrupt:
        print("\nExiting program...")
        # HardwarePWM needs to be stopped gracefully
        pwmRight.stop()

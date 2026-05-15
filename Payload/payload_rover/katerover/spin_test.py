import asyncio
from motors import Motor#, DriveController

left_motor_channel = 0
left_motor_direction_pin =
left_motor_output_pin = 38

right_motor_channel = 1
right_motor_direction_pin =
right_motor_output_pin = 42

left_motor = Motor(wheel_diameter=0.1, pwm_channel=0, direction_pin=19, output_pin=38)
right_motor = Motor(wheel_diameter=0.1, pwm_channel=1, direction_pin=19, output_pin=42)

def setup():
    return

# Spin robot in place. Speed is an integer from 0 to 100 and time is in seconds
async def spin_robot(motor1, motor2, speed, time):
    motor1.set_direction(forward=True)  # Set motor1 to forward
    motor2.set_direction(forward=False) # Set motor2 to backward
    motor1.set_speed(speed)
    motor2.set_speed(speed)
    await asyncio.sleep(time)  # Move for the specified time
    pass

async def outer():
    async def loop():
        while True:
            print("Starting loop")
            print("spin for 4 seconds at speed 100")
            await spin_robot(left_motor, right_motor, speed=100, time=4)  # Spin for 4 seconds at speed 100
            print("spin for 2 seconds at speed 50")
            await spin_robot(left_motor, right_motor, speed=50, time=2)  # Spin for 2 seconds at speed 50

    task = asyncio.create_task(loop())
    done, pending = await asyncio.wait([task], return_when=asyncio.FIRST_COMPLETED)

    if task in done:
        print("Loop completed successfully")
    else:
        print("Loop is incomplete (timeout reached)")
        # Cancel the task
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            print("Loop task cancelled cleanly")


if __name__ == '__main__':
    setup()
    asyncio.run(outer())
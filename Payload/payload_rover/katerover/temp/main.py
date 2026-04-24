import asyncio
from motors import Motor#, DriveController

left_motor = Motor(wheel_diameter=0.1, pwm_channel=0, direction_pin=19, output_pin=38)
#right_motor = Motor(wheel_diameter=0.1, pwm_channel=1, direction_pin=19, output_pin=42)

def setup():
    return

async def outer():
    async def loop():
        print("Starting loop")
        print("forward 2 seconds")
        await left_motor.move_forward_time(2)  # Move forward for 2 seconds
        print("backward 2 seconds")
        await left_motor.move_backward_time(2)  # Move backward for 2 seconds
        print("forward 4 seconds")
        await left_motor.move_forward_time(4)  # Move forward for 4 seconds
        print("backward 4 seconds")
        await left_motor.move_backward_time(4)  # Move backward for 4 seconds
        await asyncio.sleep(4)	

    task = asyncio.create_task(loop())
    done, pending = await asyncio.wait([task], return_when=asyncio.FIRST_COMPLETED, timeout=15)
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

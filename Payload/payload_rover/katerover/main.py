import asyncio
from motors import Motor, DriveController

left_motor = Motor(wheel_diameter=0.1, pwm_channel=0, direction_pin=13, output_pin=-1)
right_motor = Motor(wheel_diameter=0.1, pwm_channel=1, direction_pin=12, output_pin=-1)

drive = DriveController(left_motor, right_motor)


def setup():
    return

async def drive_test():
    drive.stop()
    print("Testing drive controller")
    print("Move forward for 4 seconds at speed 100")
    await drive.move_forward_timed(time=4, speed=90)  # Move forward for 4 seconds at speed 100
    print("Move backward for 4 seconds at speed 100")
    await drive.move_backward_timed(time=4, speed=80)  # Move backward for
    print("Spin left for 4 seconds at speed 100")
    await drive.spin_left_timed(time=4, speed=70)  # Spin left for 4 seconds at speed 100
    print("Spin right for 4 seconds at speed 100")
    await drive.spin_right_timed(time=4, speed=60)  # Spin right for
    drive.stop()


async def outer():
    async def loop():
        print("Starting loop")
        print("clockwise 2 seconds")
        await left_motor.move_clockwise_time(2)  # Move clockwise for 2 seconds
        print("counterclockwise 2 seconds")
        await left_motor.move_counterclockwise_time(2)  # Move counterclockwise for 2 seconds
        print("clockwise 4 seconds")
        await left_motor.move_clockwise_time(4)  # Move clockwise for 4 seconds
        print("counterclockwise 4 seconds")
        await left_motor.move_counterclockwise_time(4)  # Move counterclockwise for 4 seconds

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
    asyncio.run(drive_test())
    # asyncio.run(outer())
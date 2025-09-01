import spidev
import time

# Open SPI bus 0, device 0 (CE0)
spi = spidev.SpiDev()
spi.open(0, 0)  # (bus, device)
spi.max_speed_hz = 500000  # 500 kHz is safe for testing

# Send bytes and receive response
try:
    while True:
        tx_data = [0x9F]  # 0x9F is "Read JEDEC ID" — common on flash chips
        rx_data = spi.xfer2(tx_data + [0x00] * 3)  # Send command + dummy bytes
        print("Received:", rx_data)
        time.sleep(1)
except KeyboardInterrupt:
    spi.close()

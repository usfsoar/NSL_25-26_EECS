import bmp_sensor

bmp = bmp_sensor.BMP()
bmp.initialize_bmp()

print("BMP Sensor Test")   
print(bmp.get_altitude())
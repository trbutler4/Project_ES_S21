import time
import serial 

arduino = serial.Serial(port = 'COM4', baudrate=9600, timeout = .1)
def write(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(0.05)

test_strings = ['55000,3000,\n', '54940,2980,\n', '54990,2950,\n', '54920,3050,\n','54900,3100,\n']

while True:
 for s in test_strings:
     write(s)
     time.sleep(10) 

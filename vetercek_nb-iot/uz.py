import serial
import time
import csv
from time import sleep
import datetime


ser = serial.Serial('/dev/ttyUSB0',9600)
ser.flushInput()
previous_time=time.time()


def interface(ker):
	ser.write(str.encode(">ComMode:"+str(ker)+"\r\n"))   
	ser.write(str.encode(">SaveConfig\r\n"))
	ser.write(str.encode(">UartPro:0\r\n"))
	ser.write(str.encode(">SaveConfig\r\n"))

def sleep(kolk):
	ser.write(str.encode(">PwrIdleCfg:1,"+str(kolk)+"\r\n"))
	ser.write(str.encode(">SaveConfig\r\n"))

def test():
	ser.write(str.encode(">*\r\n"))  

sleep(1)

st=0

while True:
    try:
			
        if st <20:
           st=st+1
           #interface(1)
           #test()
           sleep(1)
           time.sleep(0.5)

        data = ser.readline()
        print(time.time()-previous_time)
        previous_time=time.time()
        print(data)


    except:
        print("end")
        break

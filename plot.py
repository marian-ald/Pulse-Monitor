import matplotlib.pyplot as plt
import serial
from drawnow import *
import atexit

# this port addres is for the serial tx/rx pins on the GPIO header
SERIAL_PORT = 'COM5'
# be sure to set this to the same rate used on the Arduino
SERIAL_RATE = 9600

values = []

plt.ion()
cnt=0

serialArduino = serial.Serial(SERIAL_PORT, SERIAL_RATE, parity=serial.PARITY_ODD, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=0)

def plotValues():
    plt.title('Serial value from ATMega324')
    plt.grid(True)
    plt.ylabel('Values')
    plt.plot(values, 'r.-', label='values', dash_capstyle='round')
    plt.legend(loc='upper right')

def doAtExit():
    serialArduino.close()
    print("Close serial")
    print("serialArduino.isOpen() = " + str(serialArduino.isOpen()))

atexit.register(doAtExit)

print("serialArduino.isOpen() = " + str(serialArduino.isOpen()))

#pre-load dummy data
for i in range(0,130):
    values.append(0)
    
while True:
    while (serialArduino.inWaiting()==0):
    	pass
    print("readline()")
    valueRead = serialArduino.readline(10)

    #print ("citesc " + valueRead)
    #check if valid value can be casted
    try:
        valueInInt = int(valueRead)
        print(valueInInt)
        if valueInInt <= 1024:
            if valueInInt >= 0:
                values.append(valueInInt)
                values.pop(0)
                drawnow(plotValues)
            else:
                print("Invalid! negative number")
        else:
            print("Invalid! too large")
    except ValueError:
        print("Invalid! cannot cast")

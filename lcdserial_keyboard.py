#lcd interface via serial io, act as keyboard output
import serial
import io
from datetime import datetime
from time import sleep
import glob
import os
from sys import exit
#from serial.tools import list_ports_common
import argparse #used for argparser
import m_serialstuff

global sPort
global lPortlist
global ser


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# test

###############################################################################
# exit semi gracefully
###############################################################################
def ExitProgram(ExitString):
    print(ExitString)
    exit(0)


###############################################################################
# list available serial ports
###############################################################################
def listports():
    global lPortlist
    for info in sorted(m_serialstuff.comports()):
        #print("{0}: {0.subsystem}".format(info))
        sTstr = "{0}".format(info)
        lPortlist.append(sTstr.split()[0])
    if len(lPortlist) < 1:
        ExitProgram("\nNo ports available")
    for iIter in range(len(lPortlist)):
        print(f"{iIter}:", lPortlist[iIter])


def get_args():
    global sPort
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', action='store', help='port location', default='')
    args = parser.parse_args()
    if args.port=='': ExitProgram('\nno port given, use --port')
    sPort = args.port
    print(f'PORT: {args.port}')

def sendtime():
    now = datetime.now()
    #print("now =", now)
    # dd/mm/YY H:M:S
    dt_string = now.strftime("%d/%m/%Y %H:%M:%S")
    #print("date and time =", dt_string)
    ser.write(dt_string.encode('utf-8'))


###############################################################################
# main
###############################################################################
def main():
    global lPortlist
    global ser
    global sPort
    lPortlist = []
    print("available ports below:")
    listports()
    get_args()

    try:
        ser = serial.Serial(port=sPort, baudrate=38400, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)
    except:
        ExitProgram("error opening port")
    #else:
    sleep(2)    
    sendtime()
    ser.close()

if __name__ == '__main__':
    main()
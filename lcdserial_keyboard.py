#lcd interface via serial io, act as keyboard output
import serial
import io
from datetime import datetime
import time
import asyncio
import glob
import os
from sys import exit
#from serial.tools import list_ports_common
import argparse #used for argparser
import m_serialstuff
#from sshkeyboard import listen_keyboard, stop_listening
#sshkeyboard is no good for this, is blocking while waiting a key
global sPort
global lPortlist
global ser
global lasttime

#class tLCDData(object):
#    def __init__(self, ):
#        self.equipid = equipid

LCDTime = "[0A]"
LCDData = ["[1B]", "[2B]"]
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
    lPortlist = []
    print("available ports below:")
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
    if args.port=='': ExitProgram('\nNo port given, use --port')
    sPort = args.port
    print(f'PORT: {args.port}')

def sendtime():
#    await asyncio.sleep(0.1)
    global lasttime
    now = datetime.now()
    #print("now =", now)
    # dd/mm/YY H:M:S
    dt_string = now.strftime("%H:%M")
    #print("date and time =", dt_string)
    if dt_string != lasttime:
        lasttime = dt_string
        print(f"DT:{dt_string} LT:{lasttime}")
        dt_string=LCDTime+dt_string
        ser.write(dt_string.encode('utf-8'))


def CheckDataRecv():
    global ser
    sData = ser.readline()
    if sData != "":
        print(sData)
    return sData

###############################################################################
# main
###############################################################################
def main():
    global ser
    global sPort
    global lasttime
    lasttime = "a"
    listports()
    get_args()

    try:
        ser = serial.Serial(port=sPort, baudrate=38400, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=0.1)
    except:
        ExitProgram("error opening port")
    #else:
    time.sleep(2) #wait for display to init
#    for iIter in range(0,1000):
    while True:
        sendtime()
        CheckDataRecv()
    ExitProgram("bye.")

if __name__ == '__main__':
    main()
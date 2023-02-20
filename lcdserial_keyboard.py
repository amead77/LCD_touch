#lcd interface via serial io, act as keyboard output
import serial
import io
from datetime import datetime
import time
#import asyncio
import glob
import os
from sys import exit
#from serial.tools import list_ports_common
import argparse #used for argparser
import m_serialstuff
#from sshkeyboard import listen_keyboard, stop_listening
#sshkeyboard is no good for this, is blocking while waiting a key

import pyautogui


global sPort
global lPortlist
global ser
global lasttime
global pagecount
global pageboxes

#class tLCDData(object):
#    def __init__(self, ):
#        self.equipid = equipid
LCDSlots = 7
LCDTime = 0
LCDSlotSelect = ["[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]"]

class cPage(object):
    def __init__(self, pageno=None, boxno=None, boxmsg=None, boxchanged=None, recvdata=None, hotkey=None, ident=None):
        self.pageno = pageno
        self.boxno = boxno
        self.boxmsg = boxmsg
        self.boxchanged = boxchanged
        self.recvdata = recvdata
        self.hotkey = hotkey
        self.ident = ident


global Pages
Pages = []

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
    
    #override for vscode
    sPort = '/dev/ttyACM0'
    #if args.port=='': ExitProgram('\nNo port given, use --port')
    #sPort = args.port
    #print(f'PORT: {args.port}')

def sendtime():
#    await asyncio.sleep(0.1)
    global lasttime
    global Pages
    now = datetime.now()
    dt_string = now.strftime("%H:%M")
    
    if dt_string != lasttime:
        lasttime = dt_string
        print(f"DT:{dt_string} LT:{lasttime}")
        #dt_string=LCDSlotSelect[LCDTime]+dt_string
        
        Pages[0].pageno = 0
        Pages[0].boxno = 0
        Pages[0].boxmsg = dt_string
        Pages[0].boxchanged = True
        Pages[0].recvdata = False
        Pages[0].hotkey = "None"
        Pages[0].ident = "B*0"

        #ser.write(dt_string.encode('utf-8'))
        #time.sleep(0.3)
        #ser.write("[C]".encode('utf-8'))



# *********************************
# get to work on this
# *********************************

def CheckDataRecv():
    global ser
    global Pages
    sData = str(ser.readline(), 'utf-8')
    if sData != "":
        print(sData)
        sData = sData[0:4]
        match sData:
            case "B*00":
                now = datetime.now()
                dt_string = now.strftime("%H:%M")
                pyautogui.write(dt_string, interval=0.05)
                pass
            case _:
                pass
        if sData[2].isnumeric():
            iIter = int(sData[2])
            Pages[iIter].recvdata = True
    return sData


def buildpages():
    global Pages
    global pagecount
    pagecount = 1 #currently only 1 page
    Pages.append(cPage(0,0,"msg0/0", True, False, "None", "B*00")) #ignored as clock in ere
    Pages.append(cPage(0,1,"msg0/1", True, False, "None", "B*01"))
    Pages.append(cPage(0,2,"msg0/2", True, False, "None", "B*02"))
    Pages.append(cPage(0,3,"msg0/3", True, False, "None", "B*03"))
    Pages.append(cPage(0,4,"msg0/4", True, False, "None", "B*04"))
    Pages.append(cPage(0,5,"msg0/5", True, False, "None", "B*05"))
    Pages.append(cPage(0,6,"msg0/6", True, False, "None", "B*06"))
    Pages.append(cPage(0,7,"msg0/7", True, False, "None", "B*07"))


def sendpage():
    global Pages
    global pagecount
    global ser
    hasupdate = False
    iIter = 0
    strtmp = ""
    if pagecount > 0:
        for iIter in range(LCDSlots):
            if (Pages[iIter].boxchanged == True):
                Pages[iIter].boxchanged = False
                hasupdate = True
                strtmp = '['+str(Pages[iIter].boxno)+']'+Pages[iIter].boxmsg
                ser.write(strtmp.encode('utf-8'))
                time.sleep(0.125)
    if hasupdate:
        time.sleep(0.25)
        ser.write('[C]'.encode('utf-8'))


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
        ser = serial.Serial(port=sPort, baudrate=57600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=0.1, xonxoff=1)
    except:
        ExitProgram("error opening port")
    #else:
    time.sleep(3) #wait for display to init


    ###tests
#    for iIter in range(0,9):
    #ser.write("[A]1".encode('utf-8'))
    #time.sleep(0.5)
    #ser.write("[1]box1".encode('utf-8'))
    #time.sleep(0.5)
#    sendtime()
#    time.sleep(2)
#    ser.write("[B]".encode('utf-8'))

    buildpages()

    while True:
        sendtime()
        sendpage()
        CheckDataRecv()
    ExitProgram("bye.")

if __name__ == '__main__':
    main()
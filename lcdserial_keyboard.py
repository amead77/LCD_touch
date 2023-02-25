###############################################################################
# https://github.com/amead77/LCD_touch
# arduino touchscreen LCD is:
# https://www.amazon.co.uk/gp/product/B075CXXL1M
# uses Arduino UNO attached to LCD. 
# Python sends data to arduino, which puts them into boxes on lcd.
# LCD detects press and matches to a box, returns that box number
# Python receives that data then decides what to do with it.
#
# TODO: change sent data to include checksum
# TODO: if data not match checksum, arduino should request resend.
# TODO: find out why corruption happens in first place. Probably due to IO
#       blocking when using delay() and LCD routines.
#
#
# notes:
#   anyone reading this and lcd/lcd.ino will see I've tried various stuff
#   at points, the leftovers are still here.
#
#
# ALSO: LCD/Arduino port is hardcoded in get_args()
# just uncomment as required.
#
#
###############################################################################

import serial
import io
from datetime import datetime
import time
#import asyncio
import glob
import os
from sys import exit
#from serial.tools import list_ports_common #now uses m_serialstuff
import argparse #used for argparser
import m_serialstuff #modified version of code from serial.tools.list_ports_common
#from sshkeyboard import listen_keyboard, stop_listening
#sshkeyboard is no good for this, is blocking while waiting a key

import pyautogui #used to do stuff based on return from lcd


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
    """
    this is the stucture of the box on LCD.
    """
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



###############################################################################
def ExitProgram(ExitString):
    """
    Exits the program after printing ExitString
    """
    print(ExitString)
    exit(0)


###############################################################################
def computeChecksum(value):
    """
    ##no idea where I got this from, must have been py2 code as I had
    to add encoding='utf-8' to the bytearray.

    Compute a checksum for the specified string.
    The checksum is computed by XORing the bytes in the string together.
    Parameters
    ----------
    value : string
        The string to compute the checksum for.
    Returns
    -------
    int
        The computed checksum.
    """
    checksum = 0
    for byte in bytearray(value, encoding='utf-8'):
        checksum ^= byte
    return checksum 


###############################################################################
def listports():
    """
    List available serial ports, exits if none available.
    Otherwise fills lPortlist with port names
    """
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


###############################################################################
def get_args():
    """
    # get port data from the command line
    # currently overriden (commented out) because VSCode isn't friendly with
    # passing command line arguments

    """
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
    """
    Just sends the time to the LCD, using box[0]
    It doesn't directly send, only sets the data and puts .boxchanged=True
    sendpage() checks .boxchanged and does the actual sending
    """
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
    """
    check what data comes from the serial port and break it down into 
    something that can be worked on.
    This is the main part of determining what to do if a button is pressed on 
    the LCD
    """
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
            iIter = int(sData[2]) #todo use 2 bytes
            Pages[iIter].recvdata = True
    return sData


def buildpages():
    """
    builds up the array of cPage classes (record structure)
    this is being done to prevent range check errors if done on the fly.
    """
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
    """
    If Pages[3].boxchanged then send the data from that page.
    TODO: need to implement checksum send here
    """
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
                #compute and add checksum after !
                strtmp += '!'+str(computeChecksum(strtmp))
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
/*####################################################

# Arduino Touchscreen LCD interface.

# https://github.com/amead77/LCD_touch
# arduino touchscreen LCD is:
# https://www.amazon.co.uk/gp/product/B075CXXL1M
# uses Arduino UNO attached to LCD. 
# Python sends data to arduino, which puts them into boxes on lcd.
# LCD detects press and matches to a box, returns that box number
# Python receives that data then decides what to do with it.

TODO:
-create boxes and keep text in array - mostly done

-implement checksum on received data
-request resend when no match checksum
-find out why data corruption occurs

####################################################*/


//
//
//  320x480 in portrait with usb at top
//
//
#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include "TouchScreen.h" // only when you want to use touch screen 
//if the IC model is known or the modules is unreadable,you can use this constructed function
LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

//param calibration from kbv
#define TS_MINX 906
#define TS_MAXX 116

#define TS_MINY 92 
#define TS_MAXY 952

#define MINPRESSURE 5 //10
#define MAXPRESSURE 1000
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//define some colour values
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define header "-LCD0-"

typedef struct s_boxdef {
	int startx;
	int starty;
	int endx;
	int endy;
};

typedef struct s_boxdata {
	int iboxnum; //not actually required, array position will set
	String sboxdata;
};

s_boxdef boxno[6] = {{1,1,319, 45}, {1,47,319, 92}, {1,94,319, 139}, {1,141,319, 186}, {1,188,319, 233}, {1,235,319, 280}};
//String boxmsg[6] = {"TEST 00", "TEST 01", "TEST 02", "TEST 03", "TEST 04", "TEST 05", "TEST 06"};
//s_boxdef timeplace = {1,1,319,45};

s_boxdata boxdata[7];

int boxcount = -1;
int debounce = 0;
int lastpressed = -1;
int pressed = -1;
bool sendit = false;
bool firsttime = true;
bool refresh = false;
int numboxes = 7;


void setup() {
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);
	Serial.begin(57600);
	mylcd.Set_Rotation(0);
	//rotating doesn't affect touch
	mylcd.Init_LCD();
//	Serial.println(mylcd.Read_ID(), HEX);
	mylcd.Fill_Screen(BLACK);
//	Serial.println(header);
	for (int ii = 0; ii <= 7; ii++) {
		//boxdata[ii].iboxnum = ii;
		boxdata[ii].sboxdata = "";
	}
}

void printboxed(String msgstr, int boxnum, byte boxsize) {
	int sx = boxno[boxnum].startx;
	int sy = boxno[boxnum].starty;
	int ex = boxno[boxnum].endx;
	int ey = boxno[boxnum].endy;
//	int msglen = msgstr.length() * 13;
	mylcd.Set_Text_Back_colour(BLACK);
    mylcd.Set_Draw_color(YELLOW);
    mylcd.Set_Draw_color(BLACK);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(YELLOW);
	mylcd.Print_String(msgstr, sx+4, sy+3);
}

void printboxedhighlight(String msgstr, int boxnum, byte boxsize) {
	int sx = boxno[boxnum].startx;
	int sy = boxno[boxnum].starty;
	int ex = boxno[boxnum].endx;
	int ey = boxno[boxnum].endy;
	mylcd.Set_Text_Back_colour(YELLOW);
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(BLACK);
	mylcd.Print_String(msgstr, sx+4, sy+3);
}

String leadingzero(byte tx) {
	String lz="0";
	if (tx < 10) {
		lz+=String(tx);
	} else
	{
		lz=String(tx);
	}
	return lz;
}

void setdisp() {
	//printboxed("Test", 3, 1);
	//printboxed(String(mylcd.Get_Display_Height()), 4);
	//printboxed(String(mylcd.Get_Display_Width()), 5);
}

void dispcoord(int xx, int yy) {
	mylcd.Set_Text_colour(GREEN);
	mylcd.Print_Number_Int(xx, 0, 200, 3, ' ',10);
	mylcd.Print_Number_Int(yy, 0, 220, 3, ' ',10);
}


//###############################################################################
//# maps x,y co-ords to onscreen box
//###############################################################################
int boxnum(int px, int py) {
	String buildstr;
	for (int bx=0; bx < numboxes; bx++) {
		buildstr=">"+String(bx);
		if ((px >= boxno[bx].startx) && (px <= boxno[bx].endx) && (py >= boxno[bx].starty) && (py <= boxno[bx].endy)) {
			return bx;
			break;
		}
	}
	return -1;
}


void send_header() {
	Serial.println(header);
}

//###############################################################################
//# receives data from serial
//# format is: [X]yyy
//# where X is either A,B,C (commands) or 0..9 (lcd box)
//# where yyy is the data that comes with it
//# todo: implement checksum check
//###############################################################################
void get_ser_data() {
	int iData = 0;
	int iButton = -1;
	String iButtType = "A";
	String sData = "";
	String bData;
	String sButton = "";

	while (Serial.available() > 0) {
		iData = Serial.read();
		if ((iData != 10) && (iData != 13)) {
			sData += char(iData);
		} else {
			break;
		}
	} //while

	if (sData.length() > 2) {
		iButton = sData.indexOf("]");
		if (iButton >= 0) {
			boxcount = 7; //because i don't want to set how many boxes from python "[A]7"
			bData = sData[1];
			iButton = bData.toInt();
			switch (sData[1]) {
				case 'A': //define how many boxes (can ignore for now)
					bData = sData[3];
					//boxcount = bData.toInt();
					Serial.println("configurator [boxes]: "+String(boxcount));
					Serial.println("sData: "+sData);

					break;
				case 'B': //clear screen
					Serial.println("clr");

					mylcd.Fill_Screen(BLACK);
					delay(250);
					break;
				case 'C': //refresh screen
						Serial.println("refresh");

						refresh = true; //this does nothing. remove or fix
						delay(20);
						Serial.println("sData: "+sData);

					if (boxcount > -1) {
						for (int ii = 0; ii < boxcount; ii++) {
							printboxed(boxdata[ii].sboxdata, ii, 4);
							Serial.println("boxcount: "+String(boxcount));
							delay(25);
						}
					} //if (boxcount > -1) {
					break;
				case 'D': //send simple lcd/arduino header info for id dev.
					send_header();
					delay(50);
				default:
					sButton = sData.substring(3);
					Serial.println("iButton: "+String(iButton)+" / sButton: "+sButton);
					
					boxdata[iButton].sboxdata = sButton;
					break;
			} //switch
		} //if (iButton >= 0) {
	} //if (sData.length() > 2) {
}


void CheckButtonPress() {
	sendit = false;
	digitalWrite(13, HIGH);
	TSPoint p = ts.getPoint();
	digitalWrite(13, LOW);

	if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
		p.x = map(p.x, TS_MINX, TS_MAXX, mylcd.Get_Display_Width(), 0);
		p.y = map(p.y, TS_MINY, TS_MAXY, mylcd.Get_Display_Height(),0);
		//dispcoord(p.x, p.y);
		//printboxnum(p.x, p.y);
		pressed=boxnum(p.x, p.y);
		if (pressed != lastpressed) {
			if (debounce > -1) {
				printboxed(boxdata[lastpressed].sboxdata, lastpressed, 4);
			}
			debounce = 0;
			lastpressed = pressed;
			sendit = true;
		}
		if (sendit) {
			//printboxed(String(pressed), 5, 4);
			printboxedhighlight(boxdata[pressed].sboxdata, pressed, 4);
			Serial.println("B*"+leadingzero(pressed)+"$"+boxdata[pressed].sboxdata);
			delay(200); //remove this, might be fucking the serial data
			printboxed(boxdata[pressed].sboxdata, pressed, 4);
		}
	} //if p.z
}

void loop() {
	if (firsttime) {
		//setdisp(); //only used when testing
		send_header();
		firsttime = false;
	}
	if (debounce != -1) {
		debounce++;
	}
	if (debounce > 150) {
		debounce=-1;
		//printboxed(boxmsg[pressed], pressed, 4);
		lastpressed = -1;
	}
	
	if (Serial.available() > 0) {
		get_ser_data();
	}
	
	CheckButtonPress(); //refactored from here out to function

	delay(5); //reduced from 50 because of lag in functions

}
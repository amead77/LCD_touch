/*####################################################

# https://github.com/amead77/LCD_touch
# arduino touchscreen LCD is:
# https://www.amazon.co.uk/gp/product/B075CXXL1M
# uses Arduino UNO attached to LCD. 
# Python sends data to arduino, which puts them into boxes on lcd.
# LCD detects press and matches to a box, returns that box number
# Python receives that data then decides what to do with it.
#
2023-05-18: removed a lot of code, shrinking back down to basic level that was previously working.

####################################################*/


#include "LCDWIKI_GUI.h" //Core graphics library
#include "LCDWIKI_KBV.h" //Hardware-specific library
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
	String sboxdata; //text to display in box, technically not needed as boxdata has that, but legacy code
};
//for some reason using #define for these values causes the compilation to fail
int spacing = 10; //space between boxes
int boxheight = 50; //height of boxes, width is currently fixed at 320
int ey1 = boxheight; //yup, i'm this lazy
int ey2 = boxheight*2+spacing;
int ey3 = boxheight*3+spacing*2;
int ey4 = boxheight*4+spacing*3;
int ey5 = boxheight*5+spacing*4;
int ey6 = boxheight*6+spacing*5;
int ey7 = boxheight*7+spacing*6;
int ey8 = boxheight*8+spacing*7;

int pages = 1; //number of pages of boxes
int currentpage = 1; //current page of boxes

String boxdata[8] = {"Page Up", "Page Down", "three", "four", "five", "six", "seven", "eight"};

s_boxdef boxno[8] = {{1,1,319, ey1, boxdata[0]}, {1,ey1+spacing,319, ey2, boxdata[1]}, {1,ey2+spacing,319, ey3,boxdata[2]}, 
{1,ey3+spacing,319, ey4, boxdata[3]},{1,ey4+spacing,319, ey5, boxdata[4]}, {1,ey5+spacing,319, ey6, boxdata[5]}, 
{1,ey6+spacing,319, ey7, boxdata[6]}, {1,ey7+spacing,319, ey8, boxdata[7]}};

//int boxcount = -1;

//these are part of the debounce code
int debounce = 0;
int lastpressed = -1;
int pressed = -1;
bool sendit = false;


bool firsttime = true;
//bool refresh = false;

int numboxes = 7; //number of onscreen boxes
int textoffsetx = 10; //offset from box edge
int textoffsety = 10;

void setup() {
	Serial.begin(57600); //115200 was causing corruption, slower wasn't really fast enough
	mylcd.Set_Rotation(0);
//rotating doesn't affect touch. Keep orientaion in the same direction or you'll drive yourself crazy.
//
//  320x480 in portrait with usb at top
//
	mylcd.Init_LCD();

//	Serial.println(mylcd.Read_ID(), HEX);
	mylcd.Fill_Screen(BLACK);
	//Serial.println(header);
	
}

void printboxed(String msgstr, int boxnum, byte boxsize) {
	int sx = boxno[boxnum].startx;
	int sy = boxno[boxnum].starty;
	int ex = boxno[boxnum].endx;
	int ey = boxno[boxnum].endy;
//	int msglen = msgstr.length() * 13;
	mylcd.Set_Text_Back_colour(BLACK);
    //mylcd.Set_Draw_color(YELLOW);
    mylcd.Set_Draw_color(BLACK);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(YELLOW);
	mylcd.Print_String(msgstr, sx+textoffsetx, sy+textoffsety);
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
	mylcd.Print_String(msgstr, sx+textoffsetx, sy+textoffsety);
}

String leadingzero(byte tx) {
/**
 * returns a string from byte tx, if 0..9, includes a leading zero.
 */
	return (tx < 10) ? "0" + String(tx) : String(tx);
}


void dispcoord(int xx, int yy) {
/**
 * used in debugging, prints xx,yy at location
*/
	mylcd.Set_Text_colour(GREEN);
	mylcd.Print_Number_Int(xx, 0, 200, 3, ' ',10);
	mylcd.Print_Number_Int(yy, 0, 220, 3, ' ',10);
}


int boxnum(int px, int py) {
/**
 * maps x,y co-ords to onscreen box
*/
	String buildstr;
	for (int bx=0; bx <= numboxes; bx++) {
		buildstr=">"+String(bx);
		if ((px >= boxno[bx].startx) && (px <= boxno[bx].endx) && (py >= boxno[bx].starty) && (py <= boxno[bx].endy)) {
			return bx;
			break;
		}
	}
	return -1;
}



void CheckButtonPress() {
/**
 * check for LCD press, map to xy coords. if match box pos, highlight box
 */
	sendit = false;
	//*******************************************************************************
	//-------here
	digitalWrite(13, HIGH);
	TSPoint p = ts.getPoint();
	digitalWrite(13, LOW);
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);
	//-------to here, MUST be in this order. do not move pinMode() out to setup()
	// something resets it and the screen stops working properly.
	//*******************************************************************************
	if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
		p.x = map(p.x, TS_MINX, TS_MAXX, mylcd.Get_Display_Width(), 0);
		p.y = map(p.y, TS_MINY, TS_MAXY, mylcd.Get_Display_Height(),0);
		//dispcoord(p.x, p.y);
		//printboxnum(p.x, p.y);
		pressed=boxnum(p.x, p.y);
		if (pressed != lastpressed) {
			if (debounce > -1) {
				//printboxed(boxno[lastpressed].sboxdata, lastpressed, 4);
			}
			debounce = 0;
			lastpressed = pressed;
			sendit = (pressed >= 0 && pressed <= numboxes) ? true : false;
			//if (pressed >= 0 && pressed <= numboxes) {
			//	sendit = true;
			//}
		}
		if (sendit) {
			//printboxed(String(pressed), 5, 4);
			printboxedhighlight(boxno[pressed].sboxdata, pressed, 4);
			Serial.println("B*"+leadingzero(pressed)+"$"+boxno[pressed].sboxdata);

			/**
			 * TODO: remove this delay, find another way.
			*/
			delay(50); //remove this, modify below to compensate

			printboxed(boxno[pressed].sboxdata, pressed, 4);
		}
	} //if p.z
}  


void RefreshScreen() {
	for (int ix = 0; ix <= numboxes; ix++) {
		printboxed(boxno[ix].sboxdata, ix, 4);
	}
}


//###############################################################################
//# main
//###############################################################################
void loop() {
	if (firsttime) {
		//this is here instead of setup() because reasons.
		pinMode(XM, OUTPUT);
		pinMode(YP, OUTPUT);
		firsttime = false;
		RefreshScreen();
	}

	debounce = (debounce != -1) ? debounce++ : debounce;
//	if (debounce != -1) {
//		debounce++;
//	}
	if (debounce > 50) { //100-150 if delay(10)
		debounce=-1;
		//printboxed(boxmsg[pressed], pressed, 4);
		lastpressed = -1;
	} 
	
	CheckButtonPress(); //refactored from here out to function

	delay(5); //no delay causes no data to be received over serial?!?!

}

/***********************************************************************************
*This program is a demo of displaying string
*This demo was made for LCD modules with 8bit or 16bit data port.
*This program requires the the LCDKIWI library.

* File                : display_string.ino
* Hardware Environment: Arduino UNO&Mega2560
* Build Environment   : Arduino

*Set the pins to the correct ones for your development shield or breakout board.
*This demo use the BREAKOUT BOARD only and use these 8bit data lines to the LCD,
*pin usage as follow:
*                  LCD_CS  LCD_CD  LCD_WR  LCD_RD  LCD_RST  SD_SS  SD_DI  SD_DO  SD_SCK 
*     Arduino Uno    A3      A2      A1      A0      A4      10     11     12      13                            
*Arduino Mega2560    A3      A2      A1      A0      A4      10     11     12      13                           

*                  LCD_D0  LCD_D1  LCD_D2  LCD_D3  LCD_D4  LCD_D5  LCD_D6  LCD_D7  
*     Arduino Uno    8       9       2       3       4       5       6       7
*Arduino Mega2560    8       9       2       3       4       5       6       7 

*Remember to set the pins to suit your display module!
*
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE 
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**********************************************************************************/


/*

ideas:
-create boxes and keep text in array or defines - mostly done
-create pages. figure out what this lcd is actually for.
-maybe use it as interface with rpi/python. read serial data, send serial data back.
-set highlighting as byte var, on off flash. use array for all possible butts.
*/


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
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define header "LCD001"
#define numboxes 5

typedef struct s_boxdef {
	int startx;
	int starty;
	int endx;
	int endy;
};

s_boxdef boxno[6] = {{1,1,319, 45}, {1,26,150, 46}, {1,51,150, 71}, {1,76,150, 96}, {1,101,150, 121}, {1,126,150, 146}};
String boxmsg[6] = {"TEST 01", "TEST 02", "TEST 03", "TEST 04", "TEST 05", "TEST 06"};
s_boxdef timeplace = {1,1,319,45};

int debounce = 0;
int lastpressed = -1;
int pressed = -1;
bool sendit = false;
bool firsttime = true;

void setup() {
	Serial.begin(38400);
	mylcd.Set_Rotation(0);
	//rotating doesn't affect touch
	mylcd.Init_LCD();
//	Serial.println(mylcd.Read_ID(), HEX);
	mylcd.Fill_Screen(BLACK);
	Serial.println(header);
}

void printboxed(String msgstr, int boxnum, byte boxsize) {
	int sx = boxno[boxnum].startx;
	int sy = boxno[boxnum].starty;
	int ex = boxno[boxnum].endx;
	int ey = boxno[boxnum].endy;
//	int msglen = msgstr.length() * 13;
	mylcd.Set_Text_Back_colour(BLACK);
    mylcd.Set_Draw_color(YELLOW);
//	mylcd.Draw_Rectangle(0,0,100,40);  	
//	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
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
//	int msglen = msgstr.length() * 13;
	mylcd.Set_Text_Back_colour(YELLOW);
    mylcd.Set_Draw_color(YELLOW);
//	mylcd.Draw_Rectangle(0,0,100,40);  	
//	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
//    mylcd.Set_Draw_color(BLACK);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
//    mylcd.Set_Draw_color(YELLOW);
//	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
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
	//printboxed("00:00", 0);
	//printboxed("Test 02", 1, 1);
	//printboxed("Test 03", 2, 1);
	//printboxed("Test 04", 3, 1);
	//printboxed(String(mylcd.Get_Display_Height()), 4);
	//printboxed(String(mylcd.Get_Display_Width()), 5);
}

void dispcoord(int xx, int yy) {
	mylcd.Set_Text_colour(GREEN);
	mylcd.Print_Number_Int(xx, 0, 200, 3, ' ',10);
	mylcd.Print_Number_Int(yy, 0, 220, 3, ' ',10);

}

int boxnum(int px, int py) {
	String buildstr;
	for (int bx=0; bx < numboxes; bx++) {
		buildstr=">"+String(bx);
//		Serial.println(buildstr);
		if ((px >= boxno[bx].startx) && (px <= boxno[bx].endx) && (py >= boxno[bx].starty) && (py <= boxno[bx].endy)) {
//			Serial.println(bx);
			return bx;
			break;
		}
	}
	return -1;
}

void displaytime(String sTime) {
	int sx = timeplace.startx;
	int sy = timeplace.starty;
	int ex = timeplace.endx;
	int ey = timeplace.endy;
//	int msglen = msgstr.length() * 13;

//    mylcd.Set_Draw_color(YELLOW);
//	mylcd.Draw_Rectangle(0,0,100,40);  	
//	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
    mylcd.Set_Draw_color(BLACK);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(4);
	mylcd.Set_Text_colour(YELLOW);
	mylcd.Set_Text_Back_colour(BLACK);
	mylcd.Print_String(sTime, sx+4+70, sy+3);

	
}

void get_ser_data() {
	int iData = 0;
	int iButton = -1;
	String iButtType = "A";
	String sData = "";
	String sButton = "";


	while (Serial.available() > 0) {
		iData = Serial.read();
		if ((iData != 10) && (iData != 13)) {
			sData += char(iData);  //String(iData+48);
		} else {
			break;
		}
	}
	if (sData.length() > 3) {
		iButton = sData.indexOf("]");
		if (iButton > 0) {
			iButton = sData[1];
			iButtType = sData[2];
			sButton = sData.substring(4);
			displaytime(sButton);

		}
	}
}

void loop() {
	if (firsttime) {
		setdisp();
		firsttime = false;
	}
	if (debounce != -1) {
		debounce++;
	}
	if (debounce > 15) {
		debounce=-1;
		//printboxed(boxmsg[pressed], pressed, 4);
		lastpressed = -1;
	}
	
	if (Serial.available() > 0) {
		get_ser_data();
	}
	sendit = false;
	digitalWrite(13, HIGH);
	TSPoint p = ts.getPoint();
	digitalWrite(13, LOW);
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);
	if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
		p.x = map(p.x, TS_MINX, TS_MAXX, mylcd.Get_Display_Width(), 0);
		p.y = map(p.y, TS_MINY, TS_MAXY, mylcd.Get_Display_Height(),0);
		//dispcoord(p.x, p.y);
		//printboxnum(p.x, p.y);
		pressed=boxnum(p.x, p.y);
		if (pressed != lastpressed) {
			if (debounce > -1) {
				printboxed(boxmsg[lastpressed], lastpressed, 4);
			}
			debounce = 0;
			lastpressed = pressed;
			sendit = true;
		}
		if (sendit) {
			printboxed(String(pressed), 5, 4);
			printboxedhighlight(boxmsg[pressed], pressed, 4);
			Serial.println("B*"+leadingzero(pressed));
		}
	}
	
	delay(50);

}
/*
//if the IC model is known or the modules is unreadable,you can use this constructed function
LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset
//if the IC model is not known and the modules is readable,you can use this constructed function
//LCDWIKI_KBV mylcd(320,480,A3,A2,A1,A0,A4);//width,height,cs,cd,wr,rd,reset
//define some colour values
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

void setup() 
{
  Serial.begin(9600);
  mylcd.Init_LCD();
  Serial.println(mylcd.Read_ID(), HEX);
  mylcd.Fill_Screen(BLACK);
}

void loop() 
{
  mylcd.Set_Text_Mode(0);
  //display 1 times string
  mylcd.Fill_Screen(0x0000);
  mylcd.Set_Text_colour(RED);
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_Size(1);
  mylcd.Print_String("Hello World!", 0, 0);
  mylcd.Print_Number_Float(01234.56789, 2, 0, 8, '.', 0, ' ');  
  mylcd.Print_Number_Int(0xDEADBEF, 0, 16, 0, ' ',16);
  //mylcd.Print_String("DEADBEF", 0, 16);

  //display 2 times string
  mylcd.Set_Text_colour(GREEN);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("Hello World!", 0, 40);
  mylcd.Print_Number_Float(01234.56789, 2, 0, 56, '.', 0, ' ');  
  mylcd.Print_Number_Int(0xDEADBEF, 0, 72, 0, ' ',16);
  //mylcd.Print_String("DEADBEEF", 0, 72);

  //display 3 times string
  mylcd.Set_Text_colour(BLUE);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Hello World!", 0, 104);
  mylcd.Print_Number_Float(01234.56789, 2, 0, 128, '.', 0, ' ');  
  mylcd.Print_Number_Int(0xDEADBEF, 0, 152, 0, ' ',16);
 // mylcd.Print_String("DEADBEEF", 0, 152);

  //display 4 times string
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(4);
  mylcd.Print_String("Hello!", 0, 192);

  //display 5 times string
  mylcd.Set_Text_colour(YELLOW);
  mylcd.Set_Text_Size(5);
  mylcd.Print_String("Hello!", 0, 224);

  //display 6 times string
  mylcd.Set_Text_colour(RED);
  mylcd.Set_Text_Size(6);
  mylcd.Print_String("Hello!", 0, 266);

  delay(3000);
}
*/
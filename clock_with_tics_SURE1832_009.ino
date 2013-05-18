/*
 Clock With Tics 4Sure Version 009
 uses 0832 LED Matrix Display from Sure Electronics
 June 2011, January 2013
 by Michael B. LeBlanc
 NSCAD University
 
 *** VERSION 009 IS UPDATED FOR ARDUINO 1.0.2 ***
 
 version 008 includes a "child safe" switch and LED
 indicator at D13. 
 
 MatrixDisplay and DisplayToolbox from Miles Burton 
 http://milesburton.com/HT1632_Arduino_%22Matrix_Display%22_Library_for_the_Sure_2416_and_0832
 */


#include "TimedAction.h"

// ***** Timedaction stuff *****
TimedAction blinkAction = TimedAction(1000,blink);

//#define ledPin 13
boolean ledState = false;
boolean childSafe = false;
int lightPin;               // light sensor to adjust brightness


// ***** SURE 0832 stuff *****
#include "MatrixDisplay.h"
#include "DisplayToolbox.h"
#include "font.h"

// Macro to make it the initDisplay function a little easier to understand
#define setMaster(dispNum, CSPin) initDisplay(dispNum,CSPin,true)
#define setSlave(dispNum, CSPin) initDisplay(dispNum,CSPin,false)

// 1 = Number of displays
// Data = 10/
// WR = 11
// True. Do you want a shadow buffer? (A scratch pad)

// Init Matrix
MatrixDisplay disp(1,11,10, true);
// Pass a copy of the display into the toolbox
DisplayToolbox toolbox(&disp);

// Prepare boundaries
uint8_t X_MAX = 0;
uint8_t Y_MAX = 0;

long randNumber;            // randomization for expletives
int minut;
int hr;


// ***** DS1307 Real Time Clock stuff *****

#define hourPin 9         // Pin 9 is the pin that adds an hour
#define minutePin 8      // Pin 8 is the pin that adds a minute

#include "Wire.h"
#define DS1307_I2C_ADDRESS 0x68

// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers

void setDateDs1307(byte second,    // 0-59
byte minute,    // 0-59
byte hour,     // 1-23
byte dayOfWeek,   // 1-7
byte dayOfMonth,  // 1-28/29/30/31
byte month,     // 1-12
byte year)     // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));   // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

// Gets the date and time from the ds1307
void getDateDs1307(byte *second,byte *minute,byte *hour,byte *dayOfWeek,byte *dayOfMonth,byte *month,byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second   = bcdToDec(Wire.read() & 0x7f);
  *minute   = bcdToDec(Wire.read());
  *hour    = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month   = bcdToDec(Wire.read());
  *year    = bcdToDec(Wire.read());
}

void setup() {
  //Serial.begin(9600);   

  // Fetch bounds (dynamically work out how large this display is)
  X_MAX = disp.getDisplayCount() * (disp.getDisplayWidth()-1)+1;
  Y_MAX = disp.getDisplayHeight();
  // Prepare display
  disp.setMaster(0,4);

  randomSeed(analogRead(0));     // Random seed from Light senso Pin 0
  pinMode(7, INPUT);             // ChildSafe switch as input on Digital 7
  pinMode(13, OUTPUT);           // ChildSafe indicator LED

  /* RTC functions */
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  Wire.begin();
}


void loop() {
  blinkAction.check();        // heartbeat function
  childSafe = digitalRead(7); // check childSafe switch
  adjustBrightness();
  clock(); 
}


// ***** Heartbeat function *****
void blink() {
  ledState ? ledState=false : ledState=true;
}


// *****   Clock function   *****
void clock() {
  // ***** Get current data from real time clock *****

  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);

  int decminute = int(minute);
  int mnt = decminute % 10;
  int tenmnt = decminute /10;
  int dechour = int(hour);
  int hr = dechour % 10;
  int tenhr = dechour /10;

  // **** adjust minutes and hours ****

  boolean hourpressed = digitalRead(hourPin);
  boolean minutepressed = digitalRead(minutePin);

  if (hourpressed == HIGH) {        // add one hour   
    if (dechour < 23) {
      dechour++;
    }
    else {
      dechour = 0;
    }
    byte newhour = byte(dechour);
    byte newminute = byte(minute);
    setDateDs1307(second, minute, dechour, dayOfWeek, dayOfMonth, month, year );
    delay(150);
  }
  else if (minutepressed == HIGH) {        // add one minute  
    if (decminute < 59) {
      decminute++;
    }
    else {
      decminute = 0;
    }
    byte newminute = byte(decminute);
    byte newhour = byte(hour);
    setDateDs1307(second, decminute, hour, dayOfWeek, dayOfMonth, month, year );
    delay(150);
  }

  // ****  Prepare time data for display  ****
  char minuto[2];
  itoa ( mnt, minuto, 10 );
  char minutt[2];
  itoa ( tenmnt, minutt, 10 );
  char hro[2];
  itoa ( hr, hro, 10 );
  char hrt[2];
  itoa ( tenhr, hrt, 10 ); 

  // ****  Draw time data to display  ****
  disp.clear();

  drawString(3,0,hrt);      // draw hours tens column
  drawString(9,0,hro);      // draw hours ones column

  if (ledState == true) {   // draw colon every other second  
    drawString(14,0,":");
  }
  else { 
    drawString(14,0,"Z");
  }

  drawString(18,0,minutt); // draw minutes tens column
  drawString(24,0,minuto); // draw minutes ones column
  disp.syncDisplays();

  if ( childSafe == false) {
    digitalWrite(13, LOW); // childSafe indicates "OFF"
    expletive();
  }
  else {
    digitalWrite(13, HIGH); // childSafe indicates "ON"
  }
}


void expletive(){
  // Choose a random number
  randNumber = random(1500);

  // function to randomly display an expletive
  if (randNumber == 10){   // frack
    disp.clear();
    drawString(5,0,"FUCK");
    disp.syncDisplays();
    delay(120);  // wait 120 milliseconds
  }

  if (randNumber == 11){   // shite
    disp.clear();
    drawString(5,0,"SHIT");
    disp.syncDisplays();
    delay(120);   // wait 120 milliseconds
  }
}


/*
 * Copy a character glyph from the myfont data structure to
 * display memory, with its upper left at the given coordinate
 * This is unoptimized and simply uses setPixel() to draw each dot.
 */
void drawChar(uint8_t x, uint8_t y, char c) {
  uint8_t dots;
  if (c >= 'A' && c <= 'Z' ||
    (c >= 'a' && c <= 'z') ) {
    c &= 0x1F;   // A-Z maps to 1-26
  } 
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 27; //use 37 for blocky numbers
  } 
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == ':') {
    c = 37; // colon
  } 
  for (char col=0; col< 5; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row=0; row < 7; row++) {
      if (dots & (64>>row))   	     // only 7 rows.
        toolbox.setPixel(x+col, y+row, 1);
      else 
        toolbox.setPixel(x+col, y+row, 0); // erase with "0" pixels
    }
  }
}


// Write out an entire string (Null terminated)
void drawString(uint8_t x, uint8_t y, char* c) {
  for(char i=0; i< strlen(c); i++)
  {
    drawChar(x, y, c[i]);
    x+=6; // Width of each glyph
  }
}

// Adjust the Brightness
void adjustBrightness() {
  int b = ((analogRead(lightPin)) - 10) / 55;
  toolbox.setBrightness(b); 
}


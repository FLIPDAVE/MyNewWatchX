#include <SPI.h>
#include <Wire.h>
//#include <Time.h>
//#include <TimeLib.h>
#include <Adafruit_GFX.h>             //Adafruit graphic display library used for the OLED display
#include <Adafruit_SSD1306.h>         //Adafruit driver for OLED
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SparkFun_MAG3110.h>

#define LeftUpper 8
#define RightUpper 11
#define RightLower 10
#define BATLVL 5
static byte bPin = A11;
static byte battEn = 4;

RTC_DS3231 rtc;
MAG3110 mag = MAG3110(); //Instantiate MAG3110

long count = 0;
bool compass = false;
bool timer = false;
bool timerStarted = false;
short Ch, Cm, Cs;
int total = Ch * 3600 + Cm * 60 + Cs;
bool screenCleared = false;
bool sleeping = false;
// Battery range 3.4 - 4.22v?
// Voltage divider resistor values.
float R1 = 10000;
float R2 = 10000;
float vDivider;
float voltage;
int percent;

/*****************************************************************
MicroView_Analog_Clock.ino
SFE_LSM9DS0 Library Simple Example Code
Jim Lindblom @ SparkFun Electronics
Original Creation Date: February 28, 2015
https://github.com/sparkfun/MicroView/tree/master/Libraries


This sketch requires the Arduino time library. Get it from
here: http://playground.arduino.cc/Code/Time


This code is beerware. If you see me (or any other SparkFun 
employee) at the local, and you've found our code helpful, please 
buy us a round!

Distributed as-is; no warranty is given.
*****************************************************************/



// Define how big the clock is. Don't make it larger than 23
// This is the radius of the clock:
#define CLOCK_SIZE 28

// Use these defines to set the clock's begin time

int year;
int month;
int day;
int hour;
int minute;
int second;

int dataYear, dataMonth, dataDay, dataHour, dataMinute, dataSecond;


#define OLED_DC     A3
#define OLED_CS     A5
#define OLED_RESET  A4
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
//Adafruit_SSD1306 display(OLED_RESET);


const uint8_t maxW = 128;
const uint8_t midW= 96;
const uint8_t maxH = 64;
const uint8_t midH = maxH/2;
static float currentminX, currentminY, currenthourX, currenthourY;
static float degresshour, degressmin, hourx, houry, minx, miny, adjustedhour, temp;
static boolean Drawn = true;
bool resetadjhour;
bool incrementHour = false; 
unsigned long currentMillis;
unsigned long previousMillis = 0;


Adafruit_BMP280 bme;
int timeout = 10;
bool dim = false;
float altitude;

DateTime now;

void setup()
{
  pinMode(8, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  
  Serial.begin(9600);
  bme.begin();
  mag.initialize(); //Initializes the mag sensor
  mag.start();      //Puts the sensor in active mode 
  
  // Set the time in the time library:
    rtc.begin();
    display.begin(SSD1306_SWITCHCAPVCC);   
    DateTime realTime(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 8));
    rtc.adjust(realTime);
    
    getTime();
    adjustedhour= dataHour;
    display.clearDisplay();  
    initScreen();
    drawFace();
    drawTime(); 

    // For voltage calulation
   vDivider = (R2 / (R1 + R2));
  
}

void loop() 
{
  
    getTime();
   if(screenCleared == false)
    {
      delay(4000);
      clearScreen();
      screenCleared = true;
    }
    drawFace();
    drawTime();
    checkTime();
    buttons();
}

void drawTime()
{

  //Serial.println(firstDraw);
 static unsigned long mSec = millis() + 1000;
 
 
  // If mSec
  if ( currentMillis == previousMillis) 
  { 
   temp = dataMinute *.01;
   adjustedhour = dataHour + temp;                                                                    
   degresshour = (((adjustedhour * 360) / 12) + 270) * (PI / 180);        // Calculate hour hand degrees:                                                                 
   degressmin = (((dataMinute * 360) / 60) + 270) * (PI / 180);      // Calculate minute hand degrees:                                                                  
   hourx = cos(degresshour) * (CLOCK_SIZE / 1.5);                   // Calculate x,y coordinates of hour hand:
   houry = sin(degresshour) * (CLOCK_SIZE / 1.5);                                                                  
   minx = cos(degressmin) * (CLOCK_SIZE / 1.2);                      // Calculate x,y coordinates of minute hand:
   miny = sin(degressmin) * (CLOCK_SIZE / 1.2);                      // First time draw requires extra line

   
    if (Drawn == true) 
    {
                                                                  // Draw hands with the line function:
  
    display.drawLine(midW, midH, midW+hourx, midH+houry, WHITE);
    currenthourX = hourx;
    currenthourY = houry;
 
   
    
    display.drawLine(midW, midH, midW+minx, midH+miny, WHITE);
    currentminX = minx;
    currentminY = miny;
    Drawn = false;
    } 
    
   
     display.display(); 
  }
}


void drawFace()
{
  display.setTextSize(0);                                     // set font type 0 (Smallest)
  
  uint8_t fontW = 6;
  uint8_t fontH = 8;
                                                              // Draw the clock face. That includes the circle outline and the 12, 3, 6, and 9 text.
  
  display.setCursor(midW-fontW-1, midH-CLOCK_SIZE+1);
  display.print(12);                                          // Print the "12"
  display.setCursor(midW-(fontW/2), midH+CLOCK_SIZE-fontH-1);
  display.print(6);                                           // Print the "6"
  display.setCursor(midW-CLOCK_SIZE+1, midH-fontH/2);
  display.print(9);                                           // Print the "9"
  display.setCursor(midW+CLOCK_SIZE-fontW-2, midH-fontH/2);
  display.print(3);                                           // Print the "3"
  display.drawCircle(midW-1, midH-1, CLOCK_SIZE, WHITE);
  display.drawCircle(midW-1, midH-1, CLOCK_SIZE+2, WHITE);    // Draw second circle
  display.display();                                          //Draw the clock
}

void checkTime()
{
  if( minx != currentminX || miny != currentminY)
  { 
    if (dataMinute == 10 || 20 || 30 || 40 || 50)
    {
      incrementHour = true;
    }
    updateMinute();
  }
  if( hourx != currenthourX || houry != currenthourY)
  {
    updateHour();
  }
  
}

void updateMinute()
{
  display.drawLine(midW, midH, midW+currentminX, midH+currentminY, BLACK);
  display.display();
  Drawn = true;
}

void updateHour()
{
  display.drawLine(midW, midH, midW+currenthourX, midH+currenthourY, BLACK);
  display.display();
  Drawn = true;
}




void buttons()
{
   if (digitalRead(LeftUpper) == LOW)
  { 
      initScreen();
      clearScreen();
      Drawn =true;
     return;
  }

  if (digitalRead(RightLower) == LOW) 
  {
    compassapp();
    //display.clearDisplay();
    //display.display();
   // Drawn = true;
    //return;
  }
   if (digitalRead(RightUpper) == LOW) 
  {
    if (sleeping == false)
    {
      sleeping = true;
      sleep();
    }
    else
    {
     if (sleeping == true)
     {
      sleeping = false;
     }
      Drawn = true;
      return;
    }
    
  }

}


void getTime()
{
   DateTime now = rtc.now();
    dataYear = now.year();
    dataMonth = now.month();
    dataDay = now.day();
    dataHour = now.hour();
    dataMinute = now.minute();
    dataSecond = now.second();
}

void initScreen()
{ 
    display.setTextSize(0);
    display.setTextColor(WHITE);
    //display.setCursor(20, 0); // points cursor to x=20 y=0
    //display.print("Time");
    display.setCursor(10, 0); // points cursor to x=10 y=10
    display.print(dataHour);
    display.print(":");
    display.print(dataMinute);
    display.print(":");
    display.print(dataSecond);
    //display.setCursor(20, 20); // points cursor to x=20 y=20
   // display.print("Date");
    display.setCursor(10, 15); // points cursor to x=10 y=30
    display.print(dataMonth);
    display.print(":");
    display.print(dataDay);
    display.print(":");
    display.print("18");
    display.setCursor(20, 25); // points cursor to x=20 y=20
    display.println("Alt");
    display.setCursor(10, 35); // points cursor to x=10 y=30
    altitude = bme.readAltitude(1017.25);
    altitude = altitude * 3.280;
    display.print(altitude,0);
    display.setCursor(35, 35); // points cursor to x=20 y=20
    display.print(" Ft");
    display.display();
    Watchface();                         
}

void clearScreen()
{
  delay(4000);
  display.clearDisplay();// erase hardware memory inside the OLED
  display.display();
  Drawn = true;
}


void sleep()
{
  int i=0;
  int p=60;
  clearScreen();
  while (sleeping == true)

  {
     if ( i < 80)
     { 
     display.clearDisplay();
     display.drawCircle(64, 32, i, WHITE);
      display.display();
      delay(500);
      display.drawCircle(64, 32, p, WHITE);
      display.display();
      delay(500);
     if (i < 60)
        {
            i++, p--;
        }
    else
        {
          i=0, p=60;
          if (sleeping == true)
          {
          sleeping = false;
          clearScreen();
          getTime();
          return;
          }
      } 
    }
  }
}



void Watchface() {
  //Serial.print("Watch face");
  //display.clearDisplay();
  // Battery measurements and calculations
  digitalWrite(battEn, HIGH);
  delay(50);
  voltage = analogRead(bPin);
  voltage = (voltage / 1024) * 3.35;
  voltage = voltage / vDivider;
  delay(50);
  digitalWrite(battEn, LOW);
  percent = (voltage - 3.4) / 0.008;
  if (percent < 0) {
    percent = 0;

  }
  if (percent > 100) {
    percent = 100;
  }

  // Screen stuff
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(5, 50);
  display.print("Batt:");
  display.print(percent);
  display.print("%");
  display.display();
  

  

//  delay(debounce);
}


void compassapp()

{
  int x, y, z;
  //Only read data when it's ready
  if(mag.dataReady()) {
    //Read the data
    mag.readMag(&x, &y, &z);
  
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.print(y);
    Serial.print(", Z: ");
    Serial.println(z);
  
    Serial.println("--------");
  }
}
  

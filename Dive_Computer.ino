/*
Name:       Dive_Computer.ino
Created:  5/27/2021 1:41:00 PM
Author:     Jason Scott
*/

#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "RTClib.h"
#include "Adafruit_MPRLS.h"

SSD1306AsciiWire oled;

const int chipSelect = 10;

// OLED FeatherWing buttons map to different pins depending on board:
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5

RTC_PCF8523 rtc;
DateTime now;

// You dont *need* a reset and EOC pin for most uses, so we set to -1 and don't connect
#define RESET_PIN  -1  // set to any GPIO pin # to hard-reset on begin()
#define EOC_PIN    -1  // set to any GPIO pin to read end-of-conversion by pin
Adafruit_MPRLS mpr = Adafruit_MPRLS(RESET_PIN, EOC_PIN);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

File file;
File file2;

const uint8_t OLED_I2C_ADDRESS = 0x3C;
bool oled_is_present = false;
unsigned long newTime = 0;
unsigned long oldTime = 0;
unsigned long oldTime2 = 0;
unsigned long oldTime3 = 0;
unsigned long oldTime4 = 0;
unsigned long oldTime5 = 0;
unsigned long timeout = 250;
//unsigned long timeout2 = 3000;
unsigned long timeout3 = 1000;
unsigned long timeout4 = 1000;
unsigned long timeout5 = 200;
//unsigned long loopTime = 0;
int rtcQueryInterval = 500;
int oldSeconds = 0;
int oldMinute = 0;
int oldHour = 0;
byte ClockPosX = 82;
byte ClockPosY = 3;
bool clockOutline = false;
int oldvolts = 0;

const int numReadings = 10;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
int oldAverage = 0;             // the previous average
int inputPin = A0;
bool flag = 0;
float initial_pressure_PSI = 0;
float pressure_hPa = 0;
float initial_pressure_hPa = 0;
float pressure_PSI = 0;
float feet_SW = 0;

int countDownHour = 0;
int countDownMinute = 0;
int countDownSecond = 0;

int futureHour = 0;
int futureMinute = 0;
int futureSecond = 0;

long hourFromUnix = 0;
long minuteFromUnix = 0;
long secondFromUnix = 0;

long unixFromHMS = 0;
bool logging = false;
bool diveStartFlag = 1;
bool diveEndFlag = 0;
long diveStartTime = 0;
long diveEndTime = 0;
float maxDepth = 0.0;
int currentWaitTime = 0;  // minutes
char endGroup = ' ';
char currentGroup = ' ';
char startGroup = ' ';
int diveDurationMin = 0;
long diveDuration = 0;
bool currentlyDiving = 0;
long currentDiveTime = 0;
long surfaceInterval = 0;
bool runOnce = true;
long timeNow = 0;
byte junk = 0;
long resNTime = 0;
long durationMem = 0;
long totalTimeDown = 0;
bool safetyStopReqd = 0;


////////////////////// SETUP /////////////////////////////////////////
void setup() {

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  pinMode(A0, INPUT);

  Serial.begin(9600);

  ////////////////////// OLED /////////////////////////////////////////
  delay(1000);  // delay for OLED to boot before issuing commands
  Wire.begin();
  Wire.setClock(400000L);
  Wire.beginTransmission(OLED_I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {        // OLED is present
    oled_is_present = true;
    oled.begin(&Adafruit128x32,OLED_I2C_ADDRESS);
    oled.setFont(System5x7);
    //Serial.println("OLED Initialized");
    //oled.println("OLED Initialized");
  }
  else{
    //Serial.println("OLED not found");
    //oled.println("OLED not found");
  }

  ////////////////////// SD CARD /////////////////////////////////////////
  if (!SD.begin(chipSelect)) {
    //Serial.println("SD card not found");
    //oled.println("SD card not found");
  }
  else{
    //Serial.println("SD card initialized");
    //oled.println("SD card initialized");
  }
  
  ////////////////////// RTC /////////////////////////////////////////
  if (! rtc.begin()) {
    //Serial.println("RTC not found");
    //oled.println("RTC not found");
  }
  else{
    //Serial.println("RTC initialized");
    //oled.println("RTC initialized");
  }
  // When time needs to be set on a new device, or after a power loss, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  
  
  ////////////////////// MPRLS sensor ///////////////////////////////////
  if (! mpr.begin())
  {
    //Serial.println("Press sensor failed");
    //oled.println("Press sensor failed");
  }
  else
  {
    //Serial.println("Press sensor initialized");
    //oled.println("Press sensor initialized");
  }

  initial_pressure_hPa = mpr.readPressure();
  //initial_pressure_PSI = initial_pressure_hPa / 68.947572932;
  
  
  ////////////////////// Misc stuff /////////////////////////////////////////
  delay(1000);
  oled.clear();
  
  oled.setCursor(12, 0);
  oled.print("FEET SW");
  

  
}  // end void Setup()


////////////////////// MAIN /////////////////////////////////////////
void loop() {

  newTime = millis();

  if ((newTime - oldTime5) > timeout5)  // Pressure Sensor
  {
    oldTime5 = newTime;
    float pressure_hPa = mpr.readPressure() - initial_pressure_hPa;
    pressure_PSI = pressure_hPa / 68.947572932;
    
    pressure_PSI = pressure_PSI * 10;    // FOR SCALING ONLY !!!!!!!!
    
    feet_SW = pressure_PSI / 0.445;
    if(abs(feet_SW) > maxDepth)
    {
      maxDepth = abs(feet_SW);
    }
    oled.set2X();
    oled.setCursor(10, 2);
    if(abs(feet_SW) < 10.0)
    {
      oled.setCursor(10, 2);
      oled.print(" ");
      oled.setCursor(20, 2);
    }
    if(abs(feet_SW) > 99.9)
    {
      feet_SW = 99.9;
    }
    oled.println(abs(feet_SW), 1);
    oled.set1X();


    
    if((abs(feet_SW) > 2))    ////////////// Start of Dive ////////////////
    {
      logging = true;
      
      if(diveStartFlag)
      {
        diveStartFlag = false;
        diveEndFlag = true;
        maxDepth = 0;
        currentWaitTime = 0;
        junk = 1;
        durationMem = 0;
        startGroup = currentGroup;
        
        diveStartTime = now.unixtime();
        unix2HMS(diveStartTime);
        //Serial.println();
        //Serial.print("Start of Dive");
        //Serial.print('\t');
        //Serial.print(hourFromUnix);
        //Serial.print(":");
        //if(minuteFromUnix < 10) Serial.print("0");
        //Serial.print(minuteFromUnix);
        //Serial.print(":");
        //if(secondFromUnix < 10) Serial.print("0");
        //Serial.print(secondFromUnix);
        //if(now.isPM())
        //{
          //Serial.println(" PM");
        //}
        //else
        //{
          //Serial.println(" AM");
        //}
        //Serial.println();
        
        File dataFile = SD.open("datalog.txt", FILE_WRITE);
        // if the file is available, write to it:
        if (dataFile)
        {
          dataFile.println();
          dataFile.print("Start of Dive");
          dataFile.print('\t');
          dataFile.print(hourFromUnix);
          dataFile.print(":");
          if(minuteFromUnix < 10) dataFile.print("0");
          dataFile.print(minuteFromUnix);
          dataFile.print(":");
          if(secondFromUnix < 10) dataFile.print("0");
          dataFile.print(secondFromUnix);
          if(now.isPM())
          {
            dataFile.println(" PM");
          }
          else
          {
            dataFile.println(" AM");
          }
          dataFile.println();
          dataFile.close();
        }
      }

      unix2HMS(currentDiveTime);
      long junk2 = ((hourFromUnix * 60) + minuteFromUnix);
      totalTimeDown = junk2 + resNTime;    
      safetyStopReqd |= getSafetyStopReqd(maxDepth, totalTimeDown);
      if(safetyStopReqd == 1)
      {
        Serial.println("Safety Stop Required");
      }
    
    }
    
    else            ////////////////////// End of Dive ///////////////////////
    {
      logging = false;
      if(diveEndFlag)
      {
        junk = 2;
        diveEndTime = now.unixtime();
        diveStartFlag = true;
        diveEndFlag = false;
        diveDuration = diveEndTime - diveStartTime;
        unix2HMS(diveEndTime);
        //Serial.println();
        //Serial.println("End of Dive");
        //Serial.print("Time:      ");
        //Serial.print(hourFromUnix);
        //Serial.print(":");
        //if(minuteFromUnix < 10) Serial.print("0");
        //Serial.print(minuteFromUnix);
        //Serial.print(":");
        //if(secondFromUnix < 10) Serial.print("0");
        //Serial.print(secondFromUnix);
        //if(now.isPM())
        //{
          //Serial.print(" PM");
        //}
        //else
        //{
          //Serial.print(" AM");
        //}
        //Serial.println();
        //Serial.print("Duration:  ");
        //unix2HMS(diveDuration);
        //Serial.print(hourFromUnix);
        //Serial.print(":");
        //if(minuteFromUnix < 10) Serial.print("0");
        //Serial.print(minuteFromUnix);
        //Serial.print(":");
        //if(secondFromUnix < 10) Serial.print("0");
        //Serial.println(secondFromUnix);
        //Serial.print("Max:       ");
        //Serial.print(maxDepth);
        //Serial.println(" ft");
        //Serial.println();
        
        
        unix2HMS(diveDuration);
        if(secondFromUnix > 0 && minuteFromUnix == 0)
        {
          diveDurationMin = 1;
        }
        else
        {
          if(secondFromUnix > 0)
          {
            diveDurationMin = minuteFromUnix + 1;
          }
          else
          {
            diveDurationMin = minuteFromUnix;
          }          
        }       
          
        if(startGroup == ' ')
        {
            
        }           
        else
        {
           file = SD.open("group.txt", FILE_READ);
           resNTime = getRNT(maxDepth, startGroup);
           //Serial.println(resNTime);
           file.close();           
        }
        long adjResNTime = resNTime + diveDurationMin;  
        file = SD.open("group.txt", FILE_READ);   
        endGroup = readGroup(maxDepth, adjResNTime);
        file.close();
        //Serial.println(endGroup);
        currentGroup = endGroup;
        
        file2 = SD.open("surfint.txt", FILE_READ);
        int nextInterval = readInterval(endGroup, currentWaitTime);
        currentWaitTime = nextInterval - durationMem;
        durationMem = nextInterval;
        file2.close();
        
        //Serial.print("End Group: ");
        //Serial.println(endGroup);
        //
        //Serial.print("Wait Time: ");
        //Serial.print(currentWaitTime);
        
        oled.set2X();
        oled.setCursor(67, 0);
        oled.println(endGroup);
        oled.set1X();
        
        //Serial.println();
        //Serial.println();
        
        File dataFile = SD.open("datalog.txt", FILE_WRITE);
        // if the file is available, write to it:
        if (dataFile)
        {
          dataFile.println();
          dataFile.println("End of Dive");
          dataFile.print("Time:     ");
          unix2HMS(diveEndTime);
          dataFile.print(hourFromUnix);
          dataFile.print(":");
          dataFile.print(minuteFromUnix);
          dataFile.print(":");
          dataFile.print(secondFromUnix);
          if(now.isPM())
          {
            dataFile.print(" PM");
          }
          else
          {
            dataFile.print(" AM");
          }
          dataFile.println();
          dataFile.print("Duration  ");
          unix2HMS(diveDuration);
          dataFile.print(hourFromUnix);
          dataFile.print(":");
          if(minuteFromUnix < 10) dataFile.print("0");
          dataFile.print(minuteFromUnix);
          dataFile.print(":");
          if(secondFromUnix < 10) dataFile.print("0");
          dataFile.println(secondFromUnix);
          dataFile.print("Max       ");
          dataFile.print(maxDepth);
          dataFile.println(" ft");
          dataFile.println();
          dataFile.close();
        }
      }
      else
      {
        junk = 2;
        if(surfaceInterval <= 0 && currentGroup > 'A')
        {          
          currentGroup--;
          oled.set2X();
          oled.setCursor(67, 0);
          oled.println(currentGroup);
          oled.set1X();
          file2 = SD.open("surfint.txt", FILE_READ);
          int nextInterval = readInterval(endGroup, durationMem);
          //Serial.println(nextInterval);
          currentWaitTime = nextInterval - durationMem;
          durationMem = nextInterval;
          file2.close();
          runOnce = true;
        }
        else if(surfaceInterval <= 0 && currentGroup == 'A')
        {
            currentGroup = ' ';
            endGroup = ' ';
            currentWaitTime = 0;
            durationMem = 0;
            resNTime = 0;
            oled.set2X();
            oled.setCursor(67, 0);
            oled.println(currentGroup);
            oled.set1X();            
        }          
      }
    }
  }

  if(! digitalRead(BUTTON_A))
  {
    oled.clear();
    //oled.println("End of Dive");
    //oled.print("Time:     ");
    //unix2HMS(diveEndTime);
    //oled.print(hourFromUnix);
    //oled.print(":");
    //oled.print(minuteFromUnix);
    //oled.print(":");
    //oled.print(secondFromUnix);
    //if(now.isPM())
    //{
    //oled.print(" PM");
    //}
    //else
    //{
    //oled.print(" AM");
    //}
    //oled.println();
    oled.print("Duration  ");
    unix2HMS(diveDuration);
    oled.print(hourFromUnix);
    oled.print(":");
    if(minuteFromUnix < 10) oled.print("0");
    oled.print(minuteFromUnix);
    oled.print(":");
    if(secondFromUnix < 10) oled.print("0");
    oled.println(secondFromUnix);
    oled.print("Max Depth ");
    oled.print(maxDepth);
    oled.println(" ft");
    oled.print("Group     ");
    oled.println(endGroup);
    oled.print("Wait Time ");
    oled.print(currentWaitTime);
    oled.print(" Min");
    delay(4000);
    
    oled.clear();
    oled.setCursor(12, 0);
    oled.print("FEET SW");
    oled.setCursor(ClockPosX,ClockPosY);
    if(now.hour() <= 9)
    {
      oled.print(" ");
    }
    oled.print(now.hour(), DEC);
    oled.print(':');
    oled.setCursor(ClockPosX + 17, ClockPosY);
    if(now.minute() <= 9)
    {
      oled.print("0");
    }
    oled.print(now.minute(), DEC);
    oled.print(':');

    oled.set2X();
    oled.setCursor(67, 0);
    oled.println(endGroup);
    oled.set1X();
    
    //oldTime =newTime;
    ////Serial.println(F("A"));
    //oled.setCursor(0,1);
    //oled.print("A");
    //unix2HMS(10000);
  }
  //if(! digitalRead(BUTTON_B))
  //{
    //oldTime = newTime;
    ////Serial.println(F("B"));
    //oled.setCursor(0,2);
    //oled.print("B");
    //unix2HMS(800);
  //}
  //if(! digitalRead(BUTTON_C))
  //{
    //oldTime = newTime;
    ////Serial.println(F("C"));
    //oled.setCursor(0,3);
    //oled.print("C");
    //unix2HMS(1234);
  //}

  //if ((newTime - oldTime) > timeout)  // Blanking for A, B, C
  //{
    //oldTime = newTime;
    //oled.setCursor(0, 1);
    //oled.print(" ");
    //oled.setCursor(0, 2);
    //oled.print(" ");
    //oled.setCursor(0, 3);
    //oled.print(" ");
  //}

  if ((newTime - oldTime2) > rtcQueryInterval || runOnce)   // How often to query the real time clock (RTC)
  {
    updateClock();
    oldTime2 = newTime;
    
    if(junk == 1)
    {
      runOnce = true;
      oldTime2 = newTime;
      currentDiveTime = now.unixtime() - diveStartTime;
      unix2HMS(currentDiveTime);
      oled.setCursor(80,0);
      if(hourFromUnix <= 9) oled.print(" ");
      oled.print(hourFromUnix);
      oled.print(":");
      if(minuteFromUnix < 10) oled.print("0");
      oled.print(minuteFromUnix);
      oled.print(":");
      if(secondFromUnix < 10) oled.print("0");
      oled.print(secondFromUnix);
    }
    if(junk == 2)
    {
      oldTime2 = newTime;
      if(runOnce)
      {
        runOnce = false;
        timeNow = now.unixtime();       
      }
      //Serial.print(garbage); Serial.print(" + "); Serial.print(currentWaitTime * 60); Serial.print(" - "); Serial.print(now.unixtime()); Serial.print(" = ");
      surfaceInterval = (timeNow + (currentWaitTime * 60) - now.unixtime());
      if(surfaceInterval <= 0)
      {
        surfaceInterval = 0;
      }
      //Serial.println(surfaceInterval);
      unix2HMS(surfaceInterval);
      oled.setCursor(80,0);
      if(hourFromUnix <= 9) oled.print(" ");
      oled.print(hourFromUnix);
      oled.print(":");
      if(minuteFromUnix < 10) oled.print("0");
      oled.print(minuteFromUnix);
      oled.print(":");
      if(secondFromUnix < 10) oled.print("0");
      oled.print(secondFromUnix);
    }
  }

  if ((newTime - oldTime3) > timeout3)
  {
    
    oldTime3 = newTime;
    
    //int value = analogRead(A6);
    //float batteryVoltage = mapfloat(value, 0, 648, 0, 4.20);
    //oled.setCursor(105, 0);
    //oled.print(batteryVoltage);
    
    //if(logging)
    //{
      //// make a string for assembling the data to log:
      //String dataString = "";
      //dataString += String(now.month());
      //dataString += String("/");
      //if(now.day() <= 9)
      //{
        //dataString += String("0");
      //}
      //dataString += String(now.day());
      //dataString += String("/");
      //dataString += String(now.year());
      //dataString += String('\t');
      //dataString += String(now.hour());
      //dataString += String(":");
      //if(now.minute() <= 9)
      //{
        //dataString += String("0");
      //}
      //dataString += String(now.minute());
      //dataString += String(":");
      //if(now.second() <= 9)
      //{
        //dataString += String("0");
      //}
      //dataString += String(now.second());
      //dataString += String('\t');
      ////dataString += String("Battery Voltage = ");
      ////dataString += String(batteryVoltage, 4);
      //dataString += String('\t');
      //dataString += String(pressure_PSI, 4);
      //dataString += String('\t');
      //dataString += String(feet_SW, 4);
      //
      //// open the file. Note that only one file can be open at a time,
      //// so you have to close this one before opening another.
      //File dataFile = SD.open("datalog.txt", FILE_WRITE);
      //// if the file is available, write to it:
      //if (dataFile)
      //{
        //dataFile.println(dataString);
        //dataFile.close();
        //// print to the serial port too:
        ////Serial.println(dataString);
        //oled.setCursor(85, 1);
        //oled.print("log");
        //flag = 1;
      //}
      //// if the file isn't open, pop up an error:
      //else
      //{
        //Serial.println(F("error opening datalog.txt"));
      //}
    //}
    //
  }

  if(flag)  // Blanking of "log"
  {
    oldTime4++;
    if(oldTime4 > timeout4)
    {
      flag = 0;
      oldTime4 = 0;
      oled.setCursor(85, 1);
      oled.print("   ");
    }
  }


}  // end void loop


///////////////////////////////  Functions  /////////////////////////////
void updateClock()
{

  now = rtc.now();
  
  if(now.hour() != oldHour)
  {
    oldHour = now.hour();
    oled.setCursor(ClockPosX,ClockPosY);
    if(now.hour() <= 9)
    {
      oled.print(" ");
    }
    oled.print(now.hour(), DEC);
    oled.print(':');
  }

  if(now.minute() != oldMinute)
  {
    oldMinute = now.minute();
    oled.setCursor(ClockPosX + 17, ClockPosY);
    if(now.minute() <= 9)
    {
      oled.print("0");
    }
    oled.print(now.minute(), DEC);
    oled.print(':');
  }

  if(now.second() != oldSeconds)
  {
    oldSeconds = now.second();
    oled.setCursor(ClockPosX + 35, ClockPosY);
    if(now.second() <= 9)
    {
      oled.print("0");
    }
    oled.print(now.second(), DEC);
  }
} // end void updateClock()


/////////////////////////////////////////////////////////////
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


/////////////////////////////////////////////////////////////
void countDownTimer(int hour, int min, int sec)
{
  now = rtc.now();
  
  futureHour = now.hour() + hour;
  
  futureMinute = now.minute() + min;
  
  futureSecond = now.second() + sec;
}


/////////////////////////////////////////////////////////////
void unix2HMS(long unixTime)
{
  secondFromUnix = unixTime%60; unixTime /= 60;
  minuteFromUnix = unixTime%60; unixTime /= 60;
  hourFromUnix   = unixTime%24; unixTime /= 24;
}


/////////////////////////////////////////////////////////////
void HMS2Unix(byte hour, byte minute, byte second)
{
  unixFromHMS = (hour * 3600) + (minute * 60) + second;
}


/////////////////////////////////////////////////////////////
bool readLine(File &f, char* line, size_t maxLen) {
  for (size_t n = 0; n < maxLen; n++) {
    int c = f.read();
    if ( c < 0 && n == 0) return false;  // EOF
    if (c < 0 || c == '\n') {
      line[n] = 0;
      return true;
    }
    line[n] = c;
  }
  return false; // line too long
}


/////////////////////////////////////////////////////////////
char readGroup(float maxDepth, long* diveDuration)
{
  long maxDepthLong = roundNum(maxDepth);
  char line[46], *ptr, *str;
  char letterGroup = '@';
  readLine(file, line, sizeof(line));
  while(strtol(line, &ptr, 10) != maxDepthLong)
  {
    readLine(file, line, sizeof(line));
  }
  while(strtol(ptr, &str, 10) < diveDuration)
  {
    while (*ptr)
    {
      if (*ptr++ == ',') break;
    }
    letterGroup++;
  }
  return letterGroup;
}


/////////////////////////////////////////////////////////////
int readInterval(char groupMem, int durationMem)
{
  int number = int(groupMem - 64);
  
  int waitTime = 0;
  char line[30], *ptr, *str;
  while(number > 0)
  {
    readLine(file2, line, sizeof(line));
    number--;
  }
  long temp = strtol(line, &ptr, 10);
  
  while(temp <= durationMem)
  {
    while (*ptr)
    {
      if (*ptr++ == ',') break;
    }
    temp = strtol(ptr, &str, 10);
  }
  
  return temp;
}


/////////////////////////////////////////////////////////////
long roundNum(float maxDepth)
{
  // Serial.print(maxDepth);
  long D = 0;
  if(maxDepth > 90) D = 100;
  else if(maxDepth > 80) D = 90;
  else if(maxDepth > 70) D = 80;
  else if(maxDepth > 60) D = 70;
  else if(maxDepth > 50) D = 60;
  else if(maxDepth > 40) D = 50;
  else if(maxDepth > 35) D = 40;
  else
  {
    D = 35;
  }
  maxDepth = D;
  return maxDepth;
}

long getRNT(float maxDepth, char sGroup )
{
  long maxDepthLong = roundNum(maxDepth);
  char line[32], *ptr, *str;
  long num2return = 0;
  readLine(file, line, sizeof(line));
  while(strtol(line, &ptr, 10) != maxDepthLong)
  {
    readLine(file, line, sizeof(line));
  }
  
  while( sGroup != '@')
  {
    
    while (*ptr)
    {
      if (*ptr++ == ',') break;
    }
    num2return = strtol(ptr, &str, 10);
    sGroup--;
  }
  return num2return;
}


bool getSafetyStopReqd(long maxDepth, long currentDiveTime)
{
  long depth = roundNum(maxDepth);
  if(depth < 80)
  {
    
  }
  else if(depth == 80)
  {
    if(currentDiveTime >= 26) return true;
    //if(currentDiveTime >= 1) return true;
  }
  else if(depth == 90)
  {
    if(currentDiveTime >= 22) return true;
    //if(currentDiveTime >= 1) return true;
  }
  else
  {
    return true;
  }
  return false;
}

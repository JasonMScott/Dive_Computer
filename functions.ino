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


void setup()
{

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
		Serial.println("OLED Initialized");
		oled.println("OLED Initialized");
	}
	else{
		Serial.println("OLED not found");
		oled.println("OLED not found");
	}

	////////////////////// SD CARD /////////////////////////////////////////
	if (!SD.begin(chipSelect)) {
		Serial.println("SD card not found");
		oled.println("SD card not found");
	}
	else{
		Serial.println("SD card initialized");
		oled.println("SD card initialized");
	}
	
	////////////////////// RTC /////////////////////////////////////////
	if (! rtc.begin()) {
		Serial.println("RTC not found");
		oled.println("RTC not found");
	}
	else{
		Serial.println("RTC initialized");
		oled.println("RTC initialized");
	}




}

void loop()
{
	file = SD.open("group.txt", FILE_READ);
    Serial.println(getRNT(55,'C'));
	file.close();
	delay(500);

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
//Header file to define settings and functions for the LCD display
#ifndef LCDDISPLAY_HPP
#define LCDDISPLAY_HPP

#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

//------------------- DISPLAY SETTINGS--------------------------
hd44780_I2Cexp lcd; // declare lcd object: auto locate & config exapander chip
const uint8_t LCDcolumns = 16;
const uint8_t LCDrows = 2;

//--------------------LCD Display Functions----------------------
//Initialises the LCD Display
void initDisplay()
{

    lcd.begin(LCDcolumns, LCDrows);
    lcd.noLineWrap();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
}

void LCDPrintSetAngle(int angle)
{
	lcd.setCursor(11,0);
	lcd.print("   ");
	lcd.setCursor(11,0);
	lcd.print(angle);
}

void LCDPrintCurrentAng(int angle)
{
	lcd.setCursor(13,1);
	lcd.print("   ");
	lcd.setCursor(13,1);
	lcd.print(angle);
}

void LCDPrint(int cursor_x, int cursor_y, String msg)
{
	lcd.setCursor(cursor_x, cursor_y);
	lcd.print(msg);
}

void LCDResetToHomeScreen()
{
	lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Angle:     ");
    lcd.setCursor(0, 1);
    lcd.print("Current Ang:   ");
}


#endif
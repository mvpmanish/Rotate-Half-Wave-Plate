#ifndef PHOTOINTERRUPT_HPP
#define PHOTOINTERRUPT_HPP

#include <Arduino.h>

class PhotoInterrupt
{
private:
bool last_pin_value;
bool current_pin_value;
int pin_number;
public:
	//Paramterised Constructor
	PhotoInterrupt(int input_pin_number)
	{
		pin_number = input_pin_number;
	}

	//-------------------Getter functions----------------------
	bool getStoredPinValue(){return current_pin_value;}
	bool getLastPinValue(){return last_pin_value;}
	int getPinNumber(){return pin_number;}

	//------------------Setter functions----------------------
	void readPin()
	{
		current_pin_value = digitalRead(pin_number);
	}

	//----------------Helper functions---------------------------
	//Reads the current pin value, saves it, and returns it
	bool readPinAndReturnValue()
	{
		current_pin_value = digitalRead(pin_number);
		return current_pin_value;
	}
	//Returns true if a falling edge has been read by the input pin
	bool fallingEdge()
	{
		last_pin_value = current_pin_value;  //Save last pin value
		current_pin_value = readPinAndReturnValue();
		return (current_pin_value == 0) && (last_pin_value == 1); 
	}
};

#endif
/***************//**
* @file PushButton.h
* This file contains a class definition and functions for a push button.
* @author Manish Patel
* @date 09/09/2019
*******************/
#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include "MilliTimer.h"

/**
 * @brief      Button press has three states: a long held press, short press, or not pressed.
 */
enum PressState{LONG_PRESS, SHORT_PRESS, NOT_PRESS};

/*-------------------------------------------------------------------------*//**
 * @brief      PushButton is a class to create an instance of a push button. The
 *             button has two actions: a short press or a long press.
 */
class PushButton
{
private:
	uint8_t button_pin;
	MilliTimer timer;  /**< Calculates times. Look in MilliTimer.h for details. */
	uint16_t long_press_time;  
	uint16_t bounce_time; 
public:

	/**
	 * @brief      Parameterised constructor. Initialises the timer with a time
	 *             out threshold of long_press_time.
	 *
	 * @param[in]  button_pin_in  The digital pin number which button connected
	 *                            to.
	 * @param[in]  long_t         The long press time threshold.
	 * @param[in]  bounce_t       The bounce time. Eliminates button bounce.
	 */
	PushButton(uint8_t button_pin_in, uint16_t long_t, uint16_t bounce_t)
	{
		button_pin = button_pin_in;
		long_press_time = long_t;
		bounce_time = bounce_t;
		timer.init(long_press_time);
	}

	/**
	 * @brief      Initialises the button. Sets the button pin mode to input pullup.
	 */
	void initButton()
	{
		pinMode(button_pin, INPUT_PULLUP);
	}
	/**
	 * @brief      Returns the state of the button press
	 *
	 * @return     An enum type PressState which indicates whether the press is short or long
	 */
	PressState press()
	{
		// If button state is low then poll whether it has timed out
		while(digitalRead(button_pin) == false) /** < Button presssed down is */
		{
			if(timer.timedOut())
			{
				return LONG_PRESS;
			}
			if((timer.elapsed() > bounce_time) && (!timer.timedOut()))
			{
				return SHORT_PRESS;
			}
		}
		if(digitalRead(button_pin == true))
		{
			timer.reset();
			return NOT_PRESS; 
		}
	}
};


#endif
/*
Flash to an Arduino Mega
*/

#include "MilliTimer.h"
#include "SerialChecker.h"
#include "PhotoInterrupt.hpp"
#include "LCDDisplay.hpp"
#include "PushButton.h"

#include <string.h>
#include <Wire.h>

//----------------- MOTOR SETTINGS------------------------------
int set_angle{0};
int current_angle;
int count_angle{0};
uint32_t five_deg_rotation_time{1846};  //initial estimate from Wiki (in milliseconds) 
uint32_t times_between_10_deg [36] = {};  //array to store times between interrupts for calibration
int array_count{0};  //accessor for array elements
bool moving_state;  /*< True if moving and false if not moving*/

//------------------DIGITAL PINS----------------------------
int motor_switch_pin = 13;
int button_pin=6;

//----------------BUTTON SETTINGS---------------------------
uint16_t long_press_time{1000};  /*< Threshold time (ms) for a long press of the push button*/
uint16_t bounce_time{200}; /*< Bounce time of push button (ms)*/
PushButton button(button_pin, long_press_time, bounce_time);  //Create the button
bool screen_reset{true};	/*< Variable to control whether the screen should be reset*/

//Photo interrupt pins
PhotoInterrupt zero_pin{52};
PhotoInterrupt ten_degree_pin{50};

//---------------------------------SERIAL SETTINGS---------------------------------
SerialChecker sc;  //Defaults the baud rate to 250000

//----------------------------------STATES----------------------------------------
//Set up the states which the HWP can be run in
enum State {IDLE, MOVING, ZEROING, CALIBRATE};
State s{CALIBRATE};  //Set the first state to Initialise

//--------------------------------TIMERS-------------------------------------
MilliTimer timer(300);  //A timer which has a threshold time of 300 ms
MilliTimer rotation_timer; //A timer used to get time taken to rotate 5 deg or 10 deg

//-------------------------------MAIN PROGRAM-----------------------------------
void setup()
{
	//Set pin modes 
   	pinMode(motor_switch_pin, OUTPUT);
   	digitalWrite(motor_switch_pin, LOW);
   	
   	pinMode(zero_pin.getPinNumber(), INPUT_PULLUP);
   	pinMode(ten_degree_pin.getPinNumber(), INPUT_PULLUP);
   	button.initButton();  //Sets the button pin mode

	initDisplay();
   	sc.init();  //Initialises the serial
   	sc.enableACKNAK();  //Have the Arduino send a "N" if does not send a message properly over serial
	//sc.enableChecksum();

   //Initialise the photo interrupts
   	zero_pin.readPin();
   	ten_degree_pin.readPin();
   	//Serial.println("Initialising...");
}


void loop()
{
	checkSerial();
	checkButton();
	switch(s)
	{
		case IDLE:
		{
			moving_state = false;
			break;
		}
		case MOVING:
		{
			moving_state = true;
			digitalWrite(motor_switch_pin, HIGH);  //turns the motor on

			//If photointerrupter has a falling edge +10 deg to current angle
			if(ten_degree_pin.fallingEdge() && timer.timedOut())
			{
				//If in middle of interrrupts and hit interrupt add 5 deg
				if((!(current_angle%5)) && (current_angle%10))
				{
					current_angle += 5;
					rotation_timer.reset();
				}
				//If moving between interrupts add 10 deg
				else
				{
					current_angle += 10;
					rotation_timer.reset();
				}
				LCDPrintCurrentAng(current_angle);
			}
			//If want to move to middle of interrupters move for the calibrated and stored
			//time taken to move by 5 degrees
			if(!(current_angle%10) && (set_angle - current_angle == 5))
			{
				if(rotation_timer.elapsed() >= five_deg_rotation_time)
				{
					//delay(five_deg_rotation_time); 
					current_angle += 5;
					LCDPrintCurrentAng(current_angle);
				}
				
			}
			
			//If set angle = current angle then set status to Idle
			if(set_angle == current_angle)
			{
				digitalWrite(motor_switch_pin, LOW);
				moving_state = false;
				s = IDLE;
			}
			break;
		}
		case ZEROING:
		{	
			digitalWrite(motor_switch_pin, HIGH);  //turns the motor on
			moving_state = true;
			if(zero_pin.fallingEdge())  //If falling edge found
			{
				digitalWrite(motor_switch_pin, LOW);
				current_angle = 0;
				moving_state = false;
				LCDResetToHomeScreen(); //Reset LCD screen
				LCDPrintSetAngle(0);
				LCDPrintCurrentAng(0);
				s = IDLE;
			}
			break;
		}
		case CALIBRATE:
		{
			digitalWrite(motor_switch_pin, HIGH);  //turns the motor on
			moving_state = true;

			//Store time between interrupts firing
			if(ten_degree_pin.fallingEdge())
			{
				uint32_t t = rotation_timer.elapsed();
				times_between_10_deg[array_count] = t;  //Store times in array
				if(t > 6000)  //If time between interrupts larger than 6 seconds then belt may need tensioning
				{
					LCDPrint(0,1, "Belt Slip Error!");
				}
				array_count++;
				rotation_timer.reset(); 
			}
			//Go through a full rotation by 360 degrees i.e. 36 interrupts
			if(array_count == 36)
			{
				digitalWrite(motor_switch_pin, LOW);  //Stop the motor
				five_deg_rotation_time = 0;  //Clear the value
				//Average the times stored ignoring the first value as this could be between 10 degrees
				for(int i{1}; i < array_count; i++)
				{
					five_deg_rotation_time += times_between_10_deg[i];
				}
				//Average time taken to rotate through five degrees is half average time taken
				//to rotate through ten degrees
				five_deg_rotation_time = five_deg_rotation_time/(2*(array_count-1)); //Average over 35 values and divide by 2
				array_count = 0;  //Re-initialise the array count

				moving_state = false;
				lcd.clear();
    			lcd.setCursor(0, 0);
    			lcd.print("Zeroing...");
				s = ZEROING;
			}
			break;
		}
	}
}

//------------------------SERIAL COMMANDS------------------------------------------

//Checks the serial and performs various functions depending on the serial code
void checkSerial()
{
    if(sc.check())	//Executes the if statement if there is a number in the serial
    {	//Set the angle
        if(sc.contains("SA") && !moving_state)
        {
            set_angle = sc.toInt16();
            //If set angle in the range 0 to 360 and a multiple of 5
            if(0 < set_angle && set_angle < 360 && !(set_angle%5))
            {
            	//If current angle != set angle
            	if(set_angle != current_angle)
            	{
            		LCDPrintSetAngle(set_angle);
            		//Set state to moving
            		s = MOVING;
            		//When half-wave plate zeroed the zero does not align
            		//with the ten degree interrupter so after zeroing and setting
            		//an angle the ten degree interrupter fires at zero degrees.
            		//Start a timer and only increment 10 degrees if this timer has timed 
            		//out and interruptor has fired.
            		timer.reset();  
            		rotation_timer.reset();
            	}
            }
        }
        
        //Get the current angle
        else if(sc.contains("GA"))
        {
        	Serial.println(current_angle);
        }
        
        //Check to see if the HWP is moving currently
        else if(sc.contains("GM"))
        {
        	Serial.println(moving_state);
        }
        
        //Zero the HWP
        else if(sc.contains("SZ") && !moving_state)
        {
        	if(zero_pin.readPinAndReturnValue() == true)  //Digital pin is high when blocked i.e. not at zero
        	{
        		lcd.clear();
        		LCDPrint(0, 0, "Set Angle: 0");
        		LCDPrint(0, 1, "Zeroing...");
        		set_angle = 0;
        		s = ZEROING;
        	}
        }
        
        //Calibrate the HWP rotator (calculate the time taken to rotate 5 degrees)
        else if(sc.contains("CA") && !moving_state)
        {
        	lcd.clear();
        	LCDPrint(0, 0, "Calibrating...");
        	s = CALIBRATE;
        	rotation_timer.reset();
        }
        
        else if(sc.contains("GZ"))	//What is value of zero pin
        {
        	Serial.println(zero_pin.readPinAndReturnValue());

        }
        
        else if(sc.contains("ID"))	//ID of the device
        {
        	Serial.println("HP");  //Half-wave Plate
        }
        
        else if(sc.contains("ST"))	//Emergency stop
        {
        	s = IDLE;
        }

        else if(sc.contains("SC0"))  //Disable checksum
        {
        	sc.disableChecksum();
        }

        else if(sc.contains("SC1"))  //Enable checksum
        {
        	sc.enableChecksum();
        }
    }
}

/**
 * @brief      Checks the state of the button. If long pressed then zero the HWP
 *             and if short pressed then increment the set angle by 5 degrees.
 *
 * @param[in]  button  The button of type PushButton. See 'PushButton.h' for
 *                     details.
 */
void checkButton()
{
	PressState press_state{button.press()};  //Press state is an enum
	if((press_state == SHORT_PRESS))
	{
		set_angle += 5;
		if(set_angle >= 360)
		{
			set_angle -= 360;
		}
		LCDResetToHomeScreen();
		LCDPrintSetAngle(set_angle);
		LCDPrintCurrentAng(current_angle);
		s = MOVING;
	}
	else if((press_state == LONG_PRESS) && (screen_reset == true))
	{
		lcd.clear();
		LCDPrint(0, 0, "Set Angle: 0");
        LCDPrint(0, 1, "Zeroing...");
        screen_reset == false;
        set_angle = 0;
        s = ZEROING;
	}
	else if(press_state == NOT_PRESS)
	{
		screen_reset = true;
	}
}

/*
* Lab_01_Blinky_Blinky.cpp
*
* Created: 1/13/2025 3:20:21 PM
* Author : James Nippa
*/
#include <avr/io.h>
int main(void)
{
	//Configure data direction registers
	DDRD=(1<<PIND7);
	while (1)
	{
		//check for button press
		if (!(PIND&(1<<PIND0))){ //button is pressed (PIND0 = 0)
			for (uint32_t i=0; i<80000; i++){
				//do nothing
			}
		}
		else{ //button is not pressed pressed (PIND0 = 1)
			for (uint32_t i=0; i<400000; i++){
				//do nothing
			}
		}
		PORTD^=(1<<PIND7); //toggle PORTD7
	}
}



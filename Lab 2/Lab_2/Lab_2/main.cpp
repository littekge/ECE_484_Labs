/*
 * Lab_2.cpp
 *
 * Created: 2/5/2026 11:27:22 AM
 * Authors: James Nippa and Gabe Litteken
 */ 

#include <avr/io.h>
void setSevenSegments(uint8_t switchVal) {
	//decode switch values and output to PORTD0-PORTD7
	switch(switchVal) {
		case 0: PORTD = 0b01111110; break;
		case 1: PORTD = 0b00110000; break;
		case 2: PORTD = 0b01101101; break;
		case 3: PORTD = 0b01111001; break;
		case 4: PORTD = 0b00110011; break;
		case 5: PORTD = 0b01011011; break;
		case 6: PORTD = 0b01011111; break;
		case 7: PORTD = 0b01110000; break;
		case 8: PORTD = 0b01111111; break;
		case 9: PORTD = 0b01110011; break;
		case 10: PORTD = 0b01110111; break;
		case 11: PORTD = 0b00011111; break;
		case 12: PORTD = 0b01001110; break;
		case 13: PORTD = 0b00111101; break;
		case 14: PORTD = 0b01001111; break;
		case 15: PORTD = 0b01000111; break;
		default: PORTD = 0b00000000; break;
	}
}

int main(void) {
	//Declare variables and configure I/O registers here
	DDRD = 0b01111111;
	DDRB = 0b00000001;
	
	uint8_t count = 0;
	bool button_pressed = false;
	
	while (1) {
		//write main code here
		// check for button press
		if (!(PINB & (1 << PINB1))) {
			if (button_pressed == false) {
				button_pressed = true;
				count++;
			}
		} else {
			button_pressed = false;
		}
		
		if (count == 15) {
			PORTB |= (1 << PORTB0);
		} else {
			PORTB &= ~(1 << PORTB0);
		}
		
		if (count > 15) {
			count = 0;
		}
		
		setSevenSegments(count);
	}
}


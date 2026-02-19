#ifndef SEG
#define SEG

#include <avr/io.h>

void setSevenSegments(uint8_t switchVal) {
	switch(switchVal) {
		case 0: PORTD = 0b11111010; break;
		case 1: PORTD = 0b01100000; break;
		case 2: PORTD = 0b11011001; break;
		case 3: PORTD = 0b11110001; break;
		case 4: PORTD = 0b01100011; break;
		case 5: PORTD = 0b10110011; break;
		case 6: PORTD = 0b10111011; break;
		case 7: PORTD = 0b11100000; break;
		case 8: PORTD = 0b11111011; break;
		case 9: PORTD = 0b11100011; break;
		case 10: PORTD = 0b11101011; break;
		case 11: PORTD = 0b00111011; break;
		case 12: PORTD = 0b10011010; break;
		case 13: PORTD = 0b01111001; break;
		case 14: PORTD = 0b10011011; break;
		case 15: PORTD = 0b10001011; break;
		default: PORTD = 0b00000000; break;
	}
}
#endif
/*
 * 7seg.h
 *
 * Created: 3/6/2026 11:25:20 AM
 *  Author: Gabe Litteken
 */ 

#ifndef SEVSEG
#define SEVSEG

#include <avr/io.h>

namespace SEG {
	volatile uint8_t digit = 0x00;
	volatile uint16_t displayNum = 0x0000;
	
	void decode(uint8_t switchVal) {
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
	
	void nextNum() {
		switch (digit) {
			case 0:
				decode((displayNum & 0xF000) >> 12);
				PORTB ^= (1 << PORTB0);
				PORTB &= ~((1 << PORTB1) | (1 << PORTB2) | (1 << PORTB3));
				break;
			case 1:
				decode((displayNum & 0x0F00) >> 8);
				PORTB ^= (1 << PORTB1);
				PORTB &= ~((1 << PORTB0) | (1 << PORTB2) | (1 << PORTB3));
				break;
			case 2:
				decode((displayNum & 0x00F0) >> 4);
				PORTB ^= (1 << PORTB2);
				PORTB &= ~((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB3));
				break;
			case 3:
				decode(displayNum & 0x000F);
				PORTB ^= (1 << PORTB3);
				PORTB &= ~((1 << PORTB0) | (1 << PORTB1) | (1 << PORTB2));
				break;
			default:
				digit = 0;
				return;
				break;
		}
		digit++;
	}
}



#endif /* INCFILE1_H_ */
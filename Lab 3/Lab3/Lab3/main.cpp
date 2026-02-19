/*
 * Lab3.cpp
 *
 * Created: 2/19/2026 10:12:46 AM
 * Author: James Nippa and Gabe Litican
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t count = 0;
bool seg_switch = true;

void setSevenSegments(uint8_t switchVal) {
	switch(switchVal) {
		case 0: PORTD = 0b11111010; break;
		case 1: PORTD = 0bshee01100000; break;
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

// Count button interrupt
ISR(INT0_vect) {
	count++;
}

// Clear button pin change interrupt
ISR(PCINT0_vect) {
	count = 0;
}

// Timer 1 Interrupt
ISR(TIMER1_COMPA_vect) {
	if (seg_switch) {
		setSevenSegments((count & 0b11110000) >> 4);
		PORTB ^= (1 << PORTB1);
		PORTB &= ~(1 << PORTB2);
		seg_switch = false;
	} else {
		setSevenSegments(count & 0b00001111);
		PORTB ^= (1 << PORTB2);
		PORTB &= ~(1 << PORTB1);
		seg_switch = true;
	}
}

int main(void)
{
	// Data direction register setup
	DDRB = (1 << PINB1)|(1 << PINB2); // PINB1 controls seg1 and PINB0 controls seg2
	DDRD = (1 << PIND7)|(1 << PIND6)|(1 << PIND5)|(1 << PIND4)|(1 << PIND3)|(0 << PIND2)|(1 << PIND1)|(1 << PIND0);// Seven segment outputs and int0 interrupt input
	PORTB = (1 << PORTB1); // Initialize PORTB1 to be on so that the clock can toggle it
	
	// External interrupt INT0 setup
	EIMSK = (1 << INT0); // Sets the external interrupt INT0 flag 
	EICRA = (1 << ISC01)|(0 << ISC00); // Falling edge on INT0 triggers the interrupt request
	
	// Pin change interrupt PINB5 setup 
	PCICR = (1 << PCIE0); // Pin change interrupt enable bit for PORTB
	PCMSK0 = (1 << PCINT5); // Pin change mask interrupt enable for PORTB5
	
	// Timer 1 Configuration
	TCCR1A = (0 << COM1A1)|(0 << COM1A0)|(0 << COM1B1)|(0 << COM1B0)|(0 << WGM11)|(0 << WGM10); // Toggle OC1A and OC1B on compare match, Setting waveform generation to CTC mode
	TCCR1B = (0 << WGM13)|(1 << WGM12)|(1 << CS12)|(0 << CS11)|(0 << CS10); // Setting wavefrom generation to CTC mode. Prescaler set to 256
	TIMSK1 = (1 << OCIE1A); // Enables the OCIE1A match interrupt
	
	// OCR1A Set
	OCR1A = 31249; // for 500ms
	
    sei(); // enable global interrupt bit
	
	// System while loop
    while (1) 
    {
    }
}


/*
 * Lab_5_Client.cpp
 *
 * Created: 3/12/2026 10:21:06 AM
 * Author : Gabe Litteken, James Nippa
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

//inputs from encoder and button
volatile uint8_t tactileInputs = 0x00;

//alarm counts
volatile uint16_t alarmMultiplexCount = 0x00;
volatile uint8_t alarmCountdown = 10;

ISR(TIMER0_COMPA_vect) {
	//every one second, increment the alarm countdown
	if (alarmMultiplexCount > 999) {
		if (alarmCountdown != 0) {
			alarmCountdown--;
		} else {
			TIMSK2 = (1 << OCIE2A);
		}
		alarmMultiplexCount = 0;
	} else {
		alarmMultiplexCount++;
	}
}

ISR(TIMER1_COMPA_vect) {
	tactileInputs = PINB & 0b00000111; //getting pinb inputs
}

ISR(TIMER2_COMPA_vect) {
	PORTB ^= (1 << PINB5);
}

int main(void)
{
	//DDR config
	DDRD = 0xFF; //all outputs
	DDRB = (1 << PINB5);
		
	//Timer 0 config
	TCCR0A = (0 << COM0A1)|(0 << COM0A0)|(0 << COM0B1)|(0 << COM0B0)|(1 << WGM01)|(0 << WGM00); //both COM0s in normal mode, WGM CTC
	TCCR0B = (0 << FOC0A)|(0 << FOC0B)|(0 << WGM02)|(0 << CS02)|(1 << CS01)|(1 << CS00); //clock prescaler 64, WGM CTC
	TIMSK0 = (1 << OCIE0A);
	OCR0A = 250; //1 ms interrupts
    
	//Timer 1 config
	TCCR1A = (0 << COM1A1)|(0 << COM1A0)|(0 << COM1B1)|(0 << COM1B0)|(1 << WGM11)|(0 << WGM10); //both COM1s in normal mod, WGM CTC
	TCCR1B = (0 << WGM13)|(1 << WGM12)|(0 << CS12)|(0 << CS11)|(1 << CS10); //clock prescaler 256, WGM CTC
	TIMSK1 = (1 << OCIE1A);
	OCR1A = 2000;
	
	//Timer 2 config
	TCCR2A = (0 << COM2A1)|(0 << COM2A0)|(0 << COM2B1)|(0 << COM2B0)|(1 << WGM21)|(0 << WGM20); //COM2A normal mode, COM2B normal mode, WGM CTC
	TCCR2B = (0 << FOC2A)|(0 << FOC2B)|(0 << WGM22)|(1 << CS22)|(1 << CS21)|(1 << CS20); //clock prescaler 1024, WGM CTC
	OCR2A = 25; //default frequency
	
	sei();
    while (1) 
    {
    }
}


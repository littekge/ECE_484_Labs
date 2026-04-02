/*
 * Lab5_HQ_Subsystem.cpp
 *
 * Created: 3/12/2026 10:29:10 AM
 * Author: James Nippa
 */ 
#define F_CPU 16000000 // 16 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
using namespace std;

// Global Variables
volatile uint8_t count = 0;
bool alarm_status = false; // false for alarm off and true for alarm on

uint8_t lcd_num_code_dec(uint8_t num) {
	switch(num) {
		case 0b00000000: return 0b00110000; break;
		case 0b00000001: return 0b00110001; break;
		case 0b00000010: return 0b00110010; break;
		case 0b00000011: return 0b00110011; break;
		case 0b00000100: return 0b00110100; break;
		case 0b00000101: return 0b00110101; break;
		case 0b00000110: return 0b00110110; break;
		case 0b00000111: return 0b00110111; break;
		case 0b00001000: return 0b00111000; break;
		case 0b00001001: return 0b00111001; break;
	}
};

void wait_4_Done(){
	PORTB|=(1<<PINB2);//trigger command with falling edge on E
	_delay_us(10);//wait to toggle
	PORTB&=~(1<<PINB2);
	_delay_ms(2);//wait for command to complete
}
void init_LCD(){
	//CLEAR AND HOME
	PORTB=0x00;
	PORTD=0x01;
	wait_4_Done();
	//FUNCTION SET TO 8-BIT MODE, TWO LINE MODE, 5x7 DIGITS
	PORTB=0x00;
	PORTD=0b00111000;
	wait_4_Done();
	//DISPLAY OFF
	PORTB=0x00;
	PORTD=0b00001000;
	wait_4_Done();
	//DISPLAY ON WITH CURSOR
	PORTB=0x00;
	PORTD=0b00001110;
	wait_4_Done();
	//ENTRY MODE SET TO NORMAL MODE W/ AUTO INCREMENT
	PORTB=0x00;
	PORTD=0b00000110;
	wait_4_Done();
}
void write_2_LCD(char outputChar){
	PORTB=0b00000001;
	PORTD=outputChar;
	wait_4_Done();
}
void set_DDRAM(uint8_t address){
	PORTB=0x00;//RS=RW=0
	PORTD=address|(1<<PORTD7);//D7-D0 = 1ADDRESS
	wait_4_Done();
}
void return_Home(){
	PORTB=0x00; //RS=RW=0
	PORTD=0b00000010; //Home command
	wait_4_Done();
}

ISR(PCINT1_vect) {
	set_DDRAM(0x00); // sets cursor at line one column 1
	if (!(PINC & (1 << PINC0))) { // just flipped so read as off

		char word1[9] = {'A', 'L', 'A', 'R', 'M', ' ', 'O', 'F', 'F'};
		for (uint8_t i = 0; i < sizeof(word1); i++) {
			write_2_LCD(word1[i]);
		}
	} else if (PINC & (1 << PINC0)) { // just flipped so read as on
		count++;
		char word1[9] = {'A', 'L', 'A', 'R', 'M', ' ', 'O', 'N', ' '};
		for (uint8_t i = 0; i < sizeof(word1); i++) {
			write_2_LCD(word1[i]);
		}
		uint8_t ones = count % 10;
		uint8_t tens = (count / 10) % 10;
		uint8_t hundreds = (count / 100) % 10;
		set_DDRAM(0x42);
		//write_2_LCD('0' + ones);
		write_2_LCD(lcd_num_code_dec(ones));
		set_DDRAM(0x41);
		write_2_LCD(lcd_num_code_dec(tens));
		set_DDRAM(0x40);
		write_2_LCD(lcd_num_code_dec(hundreds));
	} else {
		char word1[9] = {'E', 'R', 'R', 'O', 'R', ' ', ' ', ' ', ' '};
		for (uint8_t i = 0; i < sizeof(word1); i++) {
			write_2_LCD(word1[i]);
		}
	}
	
	
};


int main(void)
{
	//CONFIGURE IO
	DDRD = 0xFF;
	DDRB = (1 << PINB2)|(1 << PINB1)|(1 << PINB0); 
	DDRC = (0 << PINC0);// PINC0 is alarm status bit
	
	// Configuring pin change interrupt
	PCICR = (1 << PCIE1); // Enables pin change interrupt for PCIE2. PCINT7-0
	PCMSK1 = (1 << PCINT8); // Enables
	
	// set global interrupt bit
	sei(); 
	
	//INITIALIZE LCD
	init_LCD();
	//WRITE FIRST LINE
	char word1[9] = {'A', 'L', 'A', 'R', 'M', ' ', 'O', 'F', 'F'};
	for (uint8_t i = 0; i < sizeof(word1); i++) {
		write_2_LCD(word1[i]);
	}

	while(1) {
		//do nothing
	}
}


/*
 * Lab4.cpp
 *
 * Created: 3/5/2026 10:09:25 AM
 * Author : James Nippa & Gabe Littican
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

////////////////////////////////////////////////////////////////////
//Table storing all possible encoder state transitions
const int16_t encoder_table[16]={ //ENCODER LOOKUP TABLE
	0,1,-1,255,
	-1,0,255,1,
	1,255,0,-1,
255,-1,1,0};

//encoder states and count
volatile uint8_t newChannels = 0x00;
volatile uint8_t oldChannels = 0x00;
volatile int16_t encoderCount = 0x00; //POSITIONAL COUNT VARIABLE

//declare debouncing variables
volatile uint8_t rising_edges = 0x00;
volatile uint8_t falling_edges = 0x00;
volatile uint8_t switch_states = 0x00;

//declare buffer array
const uint8_t kBufferLength = 8;
volatile uint8_t switch_array[kBufferLength]={0};
	
////////////////////////////////////////////////////////////////////
/* YOUR GLOBAL VARIABLES HERE */
volatile uint16_t count_encode = 0x00;
volatile bool seg_switch = false;

void setSevenSegments(uint8_t switchVal) {
	switch(switchVal) {
		case 0: PORTD = 0b11111010; break;
		case 1: PORTD = 0b10100000; break;
		case 2: PORTD = 0b11011001; break;
		case 3: PORTD = 0b11110001; break;
		case 4: PORTD = 0b10100011; break;
		case 5: PORTD = 0b01110011; break;
		case 6: PORTD = 0b01111011; break;
		case 7: PORTD = 0b11100000; break;
		case 8: PORTD = 0b11111011; break;
		case 9: PORTD = 0b11100011; break;
		case 10: PORTD = 0b11101011; break;
		case 11: PORTD = 0b00111011; break;
		case 12: PORTD = 0b01011010; break;
		case 13: PORTD = 0b10111001; break;
		case 14: PORTD = 0b01011011; break;
		case 15: PORTD = 0b01001011; break;
		default: PORTD = 0b00000000; break;
	}
}

void softwareDebounce(uint8_t sampleData){
	static uint8_t sample_index= 0; //Index used to store switch samples in array
	uint8_t stable_high = 0xFF; //Initialize temporary stable_high all high
	uint8_t stable_low = 0; //Initialize temporary stable_low all low
	switch_array[sample_index] = sampleData; //Store switch sample from SPI new_data in array
	//Loop through all historical switch samples to check for stable highs and lows
	for (uint8_t i=0;i<kBufferLength;i++){
		//"And" for stable high (all 1's will produce "1" for stable high)
		stable_high &= switch_array[i];
		//"Or" for stable low (all 0's will produce "0" for stable low)
		stable_low |= switch_array[i];
	}
	rising_edges = (~switch_states)&stable_high;
	//Detect Rising Edges
	falling_edges = switch_states&(~stable_low);
	//Detect Falling Edges
	switch_states = stable_high|(switch_states&stable_low);
	//Update switch states
	//Update sample index and wrap if necessary
	if(++sample_index>=kBufferLength)
	sample_index = 0;//wrap
}

void encoder_read(){
	
	softwareDebounce((PINB & ((1 << PINB4) | (1 << PINB5))) >> PINB4);
	//save proper switch_states bits as newChannels
	newChannels = switch_states;
	//////////////////////////////////////////////////
	//DETERMINE ROTATION DIRECTION
	uint8_t tempChannels = oldChannels|newChannels;
	int16_t direction = encoder_table[tempChannels];
	if (direction == 255){
		encoderCount+=0;
	}
	else{
		encoderCount+=direction;
	}
	//shift and save channels
	oldChannels = newChannels << 2;
	//////////////////////////////////////////////////
}

ISR(TIMER1_COMPA_vect){
	//read encoder channels and call softwareDebounce function//
	encoder_read();
	//handle overflows
	/* YOUR CODE HERE */
	//Toggle seven segment digits every 10ms
	if (count_encode > 80) {
		if (seg_switch) {
			setSevenSegments(encoderCount & 0b00001111);
			PORTB ^= (1 << PORTB1);
			PORTB &= ~(1 << PORTB2);
			seg_switch = false;
			} else {
			setSevenSegments((encoderCount & 0b11110000) >> 4);
			PORTB ^= (1 << PORTB2);
			PORTB &= ~(1 << PORTB1);
			seg_switch = true;
		}
		count_encode = 0;
	} else {
		count_encode++;
	}
	//Output encoder positional count to seven-segments
}
int main(void)
{
	//IO CONFIG
	DDRB = (1 << PINB1)|(1 << PINB2)|(0 << PINB4)|(0 <<PINB5); // PINB1 controls seg1 and PINB0 controls seg2 via transistors, PINB4 and PINB5 are quadrature encoder inputs
	DDRD = (1 << PIND7)|(1 << PIND6)|(1 << PIND5)|(1 << PIND4)|(1 << PIND3)|(0 << PIND2)|(1 << PIND1)|(1 << PIND0);// Seven segment outputs and int0 interrupt input
	PORTB = (1 << PORTB1); // Initialize PORTB1 to be on so that the clock can toggle it
	//TIMER 1 CONFIG
	TCCR1A = (0 << COM1A1)|(0 << COM1A0)|(0 << COM1B1)|(0 << COM1B0)|(0 << WGM11)|(0 << WGM10); // Toggle OC1A and OC1B on compare match, Setting waveform generation to CTC mode
	TCCR1B = (0 << WGM13)|(1 << WGM12)|(0 << CS12)|(0 << CS11)|(1 << CS10); // Setting wavefrom generation to CTC mode. Prescaler set to 256
	TIMSK1 = (1 << OCIE1A); // Enables the OCIE1A match interrupt
	OCR1A = 200; // for 500ms
	sei();
	/* Replace with your application code */
	while (1)
	{
	}
}


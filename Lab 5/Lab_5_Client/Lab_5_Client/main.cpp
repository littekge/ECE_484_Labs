/*
 * Lab_5_Client.cpp
 *
 * Created: 3/12/2026 10:21:06 AM
 * Author : Gabe Litteken, James Nippa
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

//encoder table
const int16_t encoder_table[16]={ //ENCODER LOOKUP TABLE
	0,1,-1,255,
	-1,0,255,1,
	1,255,0,-1,
255,-1,1,0};

//encoder states and count
volatile uint8_t newChannels = 0x00;
volatile uint8_t oldChannels = 0x00;
volatile int16_t encoderCount = 0x0000; //POSITIONAL COUNT VARIABLE
volatile int16_t previousEncoderCount = 0x0000;

//alarm counts
volatile uint16_t alarmMultiplexCount = 0x00;
volatile uint8_t alarmCountdown = 10;

//7 segment values
volatile bool seg_switch = false;

//declare debouncing variables
volatile uint8_t rising_edges = 0x00;
volatile uint8_t falling_edges = 0x00;
volatile uint8_t switch_states = 0x00;

//declare buffer array
const uint8_t kBufferLength = 8;
volatile uint8_t switch_array[kBufferLength]={0};
	
//frequencies array
uint8_t frequencies[5] = {166, 142, 124, 110, 99};
volatile int8_t freqIndex = 0;

//setting 7 segment
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

//software debounce function
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

//encoder read
void encoder_read(){
	//save proper switch_states bits as newChannels
	newChannels = switch_states & 0b00000011;
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

ISR(TIMER0_COMPA_vect) {
	//every one second, increment the alarm countdown
	if (alarmMultiplexCount > 999) {
		if (alarmCountdown != 0) {
			alarmCountdown--;
		} else {
			TIMSK2 = (1 << OCIE2A);
			PORTC |= (1 << PINC0)|(1 << PINC1);
		}
		alarmMultiplexCount = 0;
	} else {
		alarmMultiplexCount++;
	}
}

ISR(TIMER1_COMPA_vect) {
	//7 segment display code
	if (seg_switch) {
		setSevenSegments((alarmCountdown / 10) % 10);
		PORTB |= (1 << PORTB3);
		PORTB &= ~(1 << PORTB4);
		seg_switch = false;
		} else {
		setSevenSegments(alarmCountdown % 10);
		PORTB |= (1 << PORTB4);
		PORTB &= ~(1 << PORTB3);
		seg_switch = true;
	}
	
	softwareDebounce(PINB & 0b00000111); //debouncing inputs
	
	//reset button code
	if (falling_edges & (1 << PINB2)) {
		alarmCountdown = 10;
		TIMSK2 = (0 << OCIE2A);
		PORTC &= ~((1 << PINC0)|(1 << PINC1));
	}
	
	//rotary encoder code
	encoder_read();
	if (encoderCount > (previousEncoderCount + 0x0003)) {
		freqIndex++;
		previousEncoderCount = encoderCount;
	} else if (encoderCount < (previousEncoderCount - 0x0003)) {
		freqIndex--;
		previousEncoderCount = encoderCount;
	}


	if (freqIndex > 4) {
		freqIndex = 0;
	} else if (freqIndex < 0) {
		freqIndex = 4;
	}
	
	OCR2A = frequencies[freqIndex];
	
}
 
ISR(TIMER2_COMPA_vect) {
	PORTB ^= (1 << PINB5);
}

int main(void)
{
	//DDR config
	DDRD = 0xFF; //all outputs
	DDRB = (1 << PINB5)|(1 << PINB3)|(1 << PINB4); 
	DDRC = (1 << PINC0)|(1 << PINC1);
		
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
	TCCR2B = (0 << FOC2A)|(0 << FOC2B)|(0 << WGM22)|(1 << CS22)|(0 << CS21)|(0 << CS20); //clock prescaler 64, WGM CTC
	OCR2A = frequencies[0]; //default frequency
	
	
	
	sei();
    while (1) 
    {
    }
}


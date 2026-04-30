/*
 * ECE484_FinalProject.cpp
 *
 * Created: 4/30/2026 10:37:24 AM
 * Author : james
 */ 

// Includes
#define F_CPU 16000000 // 16 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// LCD Signals
// D7 D6 D5 D4 BL E RW RS
#define LCD_ADDRESS 0x4E //device address (0x27) in 7 MSB + W (0) in LSB
#define LCD_DISABLE 0x04
#define LCD_ENABLE 0x00
#define LCD_WRITE 0x00
#define LCD_READ 0x01
#define LCD_RS 0x01
#define LCD_BL 0x08
// LCD Commands (D0-D7 in Datasheet)
#define CLEAR_DISPLAY 0x01 //need to wait 2ms after this call (per datasheet)
#define CURSOR_HOME 0x02 //need to wait 2ms after this call (per datasheet)
#define SET_ADDRESS 0x80

// Software Debounce Variables
volatile uint8_t rising_edges = 0x00;
volatile uint8_t falling_edges =0x00;
volatile uint8_t switch_states =0x00;
//declare buffer array
const uint8_t kBufferLength = 8;
volatile uint8_t switch_array[kBufferLength]={0};
	
//stepper state machine variable
int8_t stepperState=0;
int8_t stepperDirection=0;

// 

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

void TWI(unsigned char address, unsigned char data)
{
	TWCR = TWI_START; //send start condition
	while(!(TWCR & (1<<TWINT))){} //wait for start condition to transmit
	TWDR = address; //send address to get device attention
	TWCR = TWI_SEND; //Set TWINT to send address
	while(!(TWCR & (1<<TWINT))){} //wait for address to go out
	TWDR = data; //send data to address
	TWCR = TWI_SEND; //Set TWINT to send address
	while(!(TWCR & (1<<TWINT))){} //wait for data byte to transmit
	TWCR = TWI_STOP; //finish transaction
}
void LCD_Display(unsigned char data)
{
	// Put in character data upper bits while keeping enable bit high
	TWI(LCD_ADDRESS,(data & 0xF0)|LCD_DISABLE|LCD_WRITE|LCD_BL|LCD_RS);
	// Pull enable bit low to make LCD display the data
	TWI(LCD_ADDRESS,(data & 0xF0)|LCD_ENABLE|LCD_WRITE|LCD_BL|LCD_RS);
	// Put in character data lower bits while keeping enable bit high
	TWI(LCD_ADDRESS,((data<<4) & 0xF0)|LCD_DISABLE|LCD_WRITE|LCD_BL|LCD_RS);
	// Pull enable bit low to make LCD display the data
	TWI(LCD_ADDRESS,((data<<4) & 0xF0)|LCD_ENABLE|LCD_WRITE|LCD_BL|LCD_RS);
}
void LCD_Command(unsigned char command)
{
	// Put in command data upper bits while keeping enable bit high
	TWI(LCD_ADDRESS,(command &0xF0)|LCD_DISABLE|LCD_WRITE|LCD_BL);
	// Pull enable bit low to make LCD process the command
	TWI(LCD_ADDRESS,(command &0xF0)|LCD_ENABLE|LCD_WRITE|LCD_BL);
	// Put in command data lower bits while keeping enable bit high
	TWI(LCD_ADDRESS,((command<<4) & 0xF0)|LCD_DISABLE|LCD_WRITE|LCD_BL);
	// Pull enable bit low to make LCD process the command
	TWI(LCD_ADDRESS,((command<<4) & 0xF0)|LCD_ENABLE|LCD_WRITE|LCD_BL);
}

void dispText(uint8_t address, bool clear, char* text, uint8_t size) {
	//set LCD
	if (clear) {
		LCD_Command(CLEAR_DISPLAY); //clear display
	}
	_delay_ms(2);
	LCD_Command(SET_ADDRESS|address); //set address
	for (uint8_t i = 0; i < size-1; i++) {
		LCD_Display(text[i]);
	}
}

void reset() {
	char text[] = "   ";
	dispText(0x46, false, text, sizeof(text));
}

void stepperStateMachine(){
	//STATE MACHINE FOR UNL2003 FULL STEP MODE
	switch(stepperState){
		//STEP AB
		case 0:
		PORTC=0x03;
		stepperState=stepperState+stepperDirection;
		break;
		//STEP BC
		case 1:
		PORTC=0x06;
		stepperState=stepperState+stepperDirection;
		break;
		//STEP CD
		case 2:
		PORTC=0x0C;
		stepperState = stepperState + stepperDirection;
		break;
		//STEP DA
		case 3:
		PORTC=0x09;
		stepperState=stepperState+stepperDirection;
		break;
	}
	//CHECK STEPPER STATE OVERFLOW
	if (stepperState>3) {
		stepperState  =0;
	}
	else if (stepperState<0) {
		stepperState = 3;
	}
}

enum State {
	DISSARMED,
	PASS_A,
	PASS_AB,
	PASS_ABC,
	PASS_C,
	PASS_CB,
	PASS_CBA,
	ARMED
};

enum State state = DISSARMED;

void stateMachine() {
	switch(state) {
		case DISSARMED:
		
			break;
		case PASS_A:
		
			break;
		case PASS_AB:
		
			break;
		case PASS_ABC:
		
			break;
		case PASS_C:
		
			break;
		case PASS_CB:
		
			break;
		case PASS_CBA:
		
			break;
		case ARMED:
		
			break;
	}
}

ISR(INT0_vect) {
	softwareDebounce(PINB & ((1 << PINB0) | (1 << PINB1) | (1 << PINB2)));
	stateMachine();
}

int main(void)
{
	// Data direction register declaration
	DDRD = (1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7); // PIND4-7 stepper motor output GPIO
	DDRB = (0 << PINB0) | (0 << PINB1) | (0 << PINB2) | (1 << PINB3) | (1 << PINB4); // PINB0-2 button input GPIO, PINB3 active buzzer output GPIO, PINB4 LED output GPIO
	
	// LCD code
	//Configure Bit Rate (TWBR and TWSR)
	TWBR = 18; //TWBR=18
	TWSR = (0<<TWPS1)|(1<<TWPS0); //PRESCALER = 1
	//Configure TWI Interface (TWCR)
	TWCR = (1<<TWEN);
	//INITIALIZE LCD
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL); // (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	_delay_ms(15);
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL); // (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	_delay_ms(4.1);
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL); // (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	_delay_ms(4.1);
	TWI(LCD_ADDRESS,0x20|LCD_DISABLE|LCD_WRITE|LCD_BL); // (data length of 4, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x20|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	TWI(LCD_ADDRESS,0x20|LCD_DISABLE|LCD_WRITE|LCD_BL); // (load data)
	TWI(LCD_ADDRESS,0x20|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	//HOME CURSOR
	TWI(LCD_ADDRESS,0x80|LCD_DISABLE|LCD_WRITE|LCD_BL); // (load data)
	TWI(LCD_ADDRESS,0x80|LCD_ENABLE|LCD_WRITE|LCD_BL); // (clock in data)
	//CLEAR DISPLAY AND WAIT TO FINSIH
	LCD_Command(CLEAR_DISPLAY);
	_delay_ms(2);
	LCD_Command(0x0C); //Display no cursor
	LCD_Command(0x06); //Automatic Increment

	// Timer 1 Initialization
	//Configure Timer 1
	TCCR1A = (0 << WGM11) | (0 << WGM10); // Configuring for CTC mode
	TCCR1B = (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10); // Prescaler of 1
	TIMSK1 = (1 << OCIE1A);
	OCR1A = 15999; // 1 ms
	sei();
	
    /* Replace with your application code */
    while (1) 
    {
    }
}


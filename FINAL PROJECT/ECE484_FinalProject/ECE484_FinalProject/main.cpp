/*
 * ECE484_FinalProject.cpp
 *
 * Created: 4/30/2026 10:37:24 AM
 * Author : James Nippa
 */ 

// Includes
#define F_CPU 16000000 // 16 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// LCD Signals
//TWI constants
#define TWI_START 0xA4 //(enable=1, start=1, interrupt flag=1)
#define TWI_STOP 0x94 //(enable=1, stop=1, interrupt flag=1)
#define TWI_SEND 0x84 //(enable=1, start=0, interrupt flag=1)
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

// system state machine variables
uint8_t ones;
uint8_t tens;
uint16_t count = 0;
uint8_t countdown_var = 1;
bool blink_red = false;

 uint8_t softwareDebounce(uint8_t sampleData){
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
	
	return(falling_edges);
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

/*
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
*/

void dispText(uint8_t address, bool clear, const char* text) {
	//set LCD
	if (clear) {
		LCD_Command(CLEAR_DISPLAY); //clear display
	}
	_delay_ms(2);
	LCD_Command(SET_ADDRESS|address); //set address
	uint8_t i = 0;
	while (text[i] != '\0') {
		LCD_Display(text[i]);
		i++;
	}
}

void stepperStateMachine(){
	//STATE MACHINE FOR UNL2003 FULL STEP MODE
	switch(stepperState){
		//STEP AB
		case 0: {
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTD |= ((1 << PIND4) | (1 << PIND5) | (0 << PIND6) | (0 << PIND7));
			stepperState = stepperState + stepperDirection;
			break;
		}
		//STEP BC
		case 1: {
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTD |= ((0 << PIND4) | (1 << PIND5) | (1 << PIND6) | (0 << PIND7));
			stepperState = stepperState + stepperDirection;
			break;
		}
		//STEP CD
		case 2: {
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTD |= ((0 << PIND4) | (0 << PIND5) | (1 << PIND6) | (1 << PIND7));
			stepperState = stepperState + stepperDirection;
			break;
		}
		//STEP DA
		case 3: {
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTD |= ((1 << PIND4) | (0 << PIND5) | (0 << PIND6) | (1 << PIND7));
			stepperState = stepperState + stepperDirection;
			break;
		}
	}
	//CHECK STEPPER STATE OVERFLOW
	if (stepperState > 3) {
		stepperState = 0;
	}
	else if (stepperState < 0) {
		stepperState = 3;
	}
}

enum State {
	DISSARMED,
	PASS_A,
	PASS_AB,
	PASS_ABC,
	COUNTDOWN,
	LOCK,
	ARMED,
	PASS_C,
	PASS_CB,
	PASS_CBA,
	ALARM,
	UNLOCK
};

enum State state = DISSARMED;
enum State previous_state;
bool first;
bool first_override;

void stateMachine(uint8_t falling) {
	first = !(previous_state == state);
	if (first_override) {
		first = true;
		first_override = false;
	}
	previous_state = state;
	switch(state) {
		case DISSARMED: {
			if (first) {
				dispText(0x00, true, "DISSARMED");
				dispText(0x40, false, "CODE:    ");
			}

			if (falling & (1 << PINB0)) {
				dispText(0x46, false, "A");
				state = PASS_A;
			} else {
				state = DISSARMED;  // just wait
			}
			break;
		}
		case PASS_A: {
			if (first) {
				dispText(0x00, true, "DISSARMED");
				dispText(0x40, false, "CODE: A");
			}
			
			if (falling & (1 << PINB1)) {
				dispText(0x47, false, "B");
				state = PASS_AB;
			} else if (falling & ((1 << PINB0) | (1 << PINB2))) {
				state = DISSARMED;
			} else {
				state = PASS_A;
			}
			break;
		}
		case PASS_AB: {
			if (first) {
				dispText(0x00, true, "DISSARMED");
				dispText(0x40, false, "CODE: AB");
			}

			if (falling & (1 << PINB2)) {
				dispText(0x48, false, "C");
				state = PASS_ABC;
			} else if (falling & ((1 << PINB0) | (1 << PINB1))) {
				state = DISSARMED;
			} else {
				state = PASS_AB;
			}
			break;
		}
		case PASS_ABC: {
			if (first) {
				dispText(0x00, true, "DISSARMED");
				dispText(0x40, false, "CODE: ABC Cd:");
			}
			state = COUNTDOWN;
			break;
		}
		case COUNTDOWN: {
			if (first) {
				dispText(0x00, true, "DISSARMED");
				dispText(0x40, false, "CODE: ABC Cd:");
			}
			if (count >= 1000) {
				ones = countdown_var % 10;
				tens = (countdown_var / 10) % 10;
				char text[] = {char(0b00110000 | tens), char(0b00110000 | ones), '\0'};
				dispText(0x4E, false, text);		
				count = 0;
				countdown_var++;
			} else {
				count++;
			}
			
			if (countdown_var >= 11) {
				countdown_var = 0;
				count = 0;
				state = LOCK;
			}
			break;
		}
		case LOCK: {
			dispText(0x00, first, "LOCKING...");
			stepperDirection = 1;
			for (uint16_t i = 0; i < 512; i++) {
				_delay_ms(10);
				stepperStateMachine();
			}
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTB |= (1 << PINB4);
			PORTB &= ~(1 << PINB5);
			state = ARMED;
			break;
		}
		case ARMED: {
			if (first) {
				dispText(0x00, true, "ARMED");
				dispText(0x40, false, "CODE:    ");
			}
			
			if (falling & (1 << PINB2)) {
				dispText(0x46, false, "C");		
				state = PASS_C;
			} else {			
				state = ARMED;
			}
			break;
		}
		case PASS_C: {
			if (first) {
				dispText(0x00, true, "ARMED");
				dispText(0x40, false, "CODE: C");
			}
			
			if (falling_edges & (1 << PINB1)) {
				dispText(0x47, false, "B");	
				state = PASS_CB;
			} else if (falling_edges & ((1 << PINB0) | (1 << PINB2))) {
				state = ALARM;
			} else {
				state = PASS_C;
			}
			break;
		}
		case PASS_CB: {
			if (first) {
				dispText(0x00, true, "ARMED");
				dispText(0x40, false, "CODE: CB");
			}

			if (falling_edges & (1 << PINB0)) {
				dispText(0x48, false, "A");
				state = PASS_CBA;
			} else if (falling_edges & ((1 << PINB1) | (1 << PINB2))) {
				state = ALARM;
			} else {		
				state = PASS_CB;
			}
			break;
		}
		case PASS_CBA: {
			if (first) {
				dispText(0x00, true, "ARMED");
				dispText(0x40, false, "CODE: CBA");
			}

			blink_red = false;
			count = 0;
			PORTB &= ~(1 << PINB3); // turning off the alarm
			PORTB |= (1 << PINB4);
			state = UNLOCK;
			break;
		}
		case ALARM: {
			PORTB |= (1 << PINB3); // turning on the alarm
			blink_red = true;
			state = ARMED;
			break;
		}
		case UNLOCK: {
			dispText(0x00, first, "UNLOCKING...");
			stepperDirection = -1;
			for (uint16_t i = 0; i < 512; i++) {
				_delay_ms(10);
				stepperStateMachine();
			}
			PORTD &= ~((1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7));
			PORTB &= ~(1 << PINB4);
			PORTB |= 1 << PINB5;
			state = DISSARMED;
			break; 
		}
		default: {
		
			break;
		
		}
	}
	
	if (blink_red) {
		if (count >= 250) {
			PORTB ^= (1 << PINB4);
			count = 0;
		}
		count++;
	}
	
}

ISR(TIMER1_COMPA_vect) {
	//uint8_t falling = softwareDebounce(PINB & ((1 << PINB0) | (1 << PINB1) | (1 << PINB2)));
	//stateMachine(falling);
    uint8_t falling = softwareDebounce(PINB & ((1 << PINB0) | (1 << PINB1) | (1 << PINB2)));
	stateMachine(falling);
}


/*---------- MODULE E FUNCTIONS ----------*/
ISR(INT1_vect) {
	if (UCSR0A & (1 << UDRE0)) { //check if transmit buffer is empty
		UDR0 = 0b11110000; //send code
		//UDR0 = 0b01010100; //send code
		//dispText(0x00, true, "TEST TEST");
		//_delay_ms(2000);
	}
	
}

ISR(USART_TX_vect) {
	//change mode to receiver
	UCSR0B = (0 << TXCIE0)|(0 << TXEN0)|(1 << RXCIE0)|(1 << RXEN0); //enabling reciever mode and interrupt
}

ISR(USART_RX_vect) {
	uint8_t recievedData = UDR0;
	if (recievedData == 0b00001111) {
		UCSR0B = (1 << TXCIE0)|(1 << TXEN0)|(0 << RXCIE0)|(0 << RXEN0); //enabling transmitter mode, and interrupt
		dispText(0x00, true, "OK");
		_delay_ms(2000);
		//INSERT CURRENT STATUS CODE HERE
		first_override = true;
	}
}
/*---------- END MODULE E FUNCTIONS ----------*/


int main(void)
{
	// Data direction register declaration
	DDRD = (1 << PIND4) | (1 << PIND5) | (1 << PIND6) | (1 << PIND7); // PIND4-7 stepper motor output GPIO
	DDRB = (0 << PINB0) | (0 << PINB1) | (0 << PINB2) | (1 << PINB3) | (1 << PINB4) | (1 << PINB5); // PINB0-2 button input GPIO, PINB3 active buzzer output GPIO, PINB4-5 LED output GPIO
	
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
	
	// LCD initial message
	dispText(0x00, true, "DISSARMED");
	dispText(0x40, false, "CODE:    ");
	PORTB |= (1 << PINB5);

	// Timer 1 Initialization
	//Configure Timer 1
	TCCR1A = (0 << WGM11) | (0 << WGM10); // Configuring for CTC mode
	TCCR1B = (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10); // Prescaler of 1
	TIMSK1 = (1 << OCIE1A);
	OCR1A = 15999; // 1 ms
	
	/*---------- MODULE E SETUP CODE ----------*/
	//IO config
	DDRD &= ~(1 << PIND3);
	
	//USART config
	UCSR0A = (0 << U2X0)|(1 << MPCM0); //standard data prescaler, enable multiprocessor communication mode
	UCSR0B = (1 << TXCIE0)|(1 << TXEN0)|(0 << RXCIE0)|(0 << RXEN0); //enabling transmitter mode, and interrupt
	UCSR0A = (1 << UCSZ01)|(1 << UCSZ00); //setting frame size to 8 bits
	UBRR0 = 7; //setting baud rate to 250k

	//external interrupt 1 config
	EIMSK |= (1 << INT1); //enabling int 1
	EICRA |= (1 << ISC11); //setting trigger on falling edge
	EICRA &= ~(1 << ISC10);
	/*---------- END MODULE E SETUP CODE ----------*/
	
	sei();
	
	
	
    /* Replace with your application code */
    while (1) 
    {
    }
}


/*
 * Lab 6.cpp
 *
 * Created: 4/9/2026 10:11:21 AM
 * Author : Gabe Litteken
 */ 

#define  F_CPU 16000000  // 16 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

//Table storing all possible encoder state transitions
const int16_t encoder_table[16]={
	0,1,-1,255,
	-1,0,255,1,
	1,255,0,-1,
255,-1,1,0};

//encoder states and count
volatile uint8_t newChannels = 0x00;
volatile uint8_t oldChannels = 0x00;
volatile int16_t encoderCount =0x00;

//interrupt count
volatile uint16_t intCount = 0x00;

//TWI constants
#define TWI_START 0xA4 //(enable=1, start=1, interrupt flag=1)
#define TWI_STOP 0x94 //(enable=1, stop=1, interrupt flag=1)
#define TWI_SEND 0x84 //(enable=1, start=0, interrupt flag=1)

//LCD Signals
//D7 D6 D5 D4 BL E RW RS
#define LCD_ADDRESS 0x4E	//device address (0x27) in 7 MSB + W (0) in LSB
#define LCD_DISABLE 0x04
#define LCD_ENABLE 0x00
#define LCD_WRITE 0x00
#define LCD_READ 0x01
#define LCD_RS 0x01
#define LCD_BL 0x08

//LCD Commands (D0-D7 in Datasheet)
#define CLEAR_DISPLAY 0x01		//need to wait 2ms after this call (per datasheet)
#define CURSOR_HOME 0x02		//need to wait 2ms after this call (per datasheet)
#define SET_ADDRESS 0x80

volatile uint8_t rising_edges = 0x00;
volatile uint8_t falling_edges =0x00;
volatile uint8_t switch_states =0x00;

//declare buffer array
const uint8_t kBufferLength = 8;
volatile uint8_t switch_array[kBufferLength]={0};

//Sample switches at particular interval
void softwareDebounce(uint8_t sampleData){
	static uint8_t sample_index= 0;			//Index used to store switch samples in array
	uint8_t stable_high = 0xFF;				//Initialize temporary stable_high all high
	uint8_t stable_low = 0;					//Initialize temporary stable_low all low
	switch_array[sample_index] = sampleData;	//Store switch sample from SPI new_data in array
	
	//Loop through all historical switch samples to check for stable highs and lows
	for (uint8_t i=0;i<kBufferLength;i++){
		//"And" for stable high (all 1's will produce "1" for stable high)
		stable_high &= switch_array[i];
		//"Or" for stable low (all 0's will produce "0" for stable low)
		stable_low |= switch_array[i];
	}
	rising_edges = (~switch_states)&stable_high;					//Detect Rising Edges
	falling_edges = switch_states&(~stable_low);					//Detect Falling Edges
	switch_states = stable_high|(switch_states&stable_low);			//Update switch states
	
	//Update sample index and wrap if necessary
	if(++sample_index>=kBufferLength)
	sample_index = 0;//wrap
}

void TWI(unsigned char address, unsigned char data)
{
	TWCR = TWI_START;				//send start condition
	while(!(TWCR & (1<<TWINT))){}	//wait for start condition to transmit
	TWDR = address;					//send address to get device attention
	TWCR = TWI_SEND;				//Set TWINT to send address
	while(!(TWCR & (1<<TWINT))){}	//wait for address to go out
	TWDR = data;					//send data to address
	TWCR = TWI_SEND;				//Set TWINT to send address
	while(!(TWCR & (1<<TWINT))){}	//wait for data byte to transmit
	TWCR = TWI_STOP;				//finish transaction
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
		default: return 0b01000101; break;
	}
};

ISR(TIMER1_COMPA_vect){
	//read in inputs and call softwareDebounce
	softwareDebounce(PINB & ((1<<PINB0)|(1<<PINB1)));

	//read new debounced state
	newChannels = switch_states & 0b00000011;

	//determine turn direction
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


	//handle encoderCount overflows
	if (encoderCount < 0) {encoderCount = 0;}
	if (encoderCount > 249) {encoderCount = 249;}

	//output to LCD and change PWM duty cycle
	OCR0B = encoderCount;
	
	if (intCount > 1000) {
		//set LCD
		LCD_Command(CLEAR_DISPLAY); //clear display
		_delay_ms(2);
		LCD_Command(SET_ADDRESS|0x00); //set address
		
		char dispText[] = "DUTY CYCLE: "; //display text
		for (uint8_t i = 0; i < sizeof(dispText)-1; i++) {
			LCD_Display(dispText[i]);
		}
		
		uint8_t displayNum = (encoderCount*100) / 249;
		
		LCD_Display(lcd_num_code_dec((displayNum/100) % 10));
		LCD_Display(lcd_num_code_dec((displayNum/10) % 10));
		LCD_Display(lcd_num_code_dec(displayNum % 10));
		LCD_Display('%');
		
		intCount = 0x00; //reset count
	} else {
		intCount++; //increment
	}
	
}

int main(void)
{
	//Configure IO
	DDRD = (1 << PIND5); //PWM pin
	DDRB = (0 << PINB0)|(0 << PINB1); //Encoder inputs
	DDRC = (1<<PORTC4)|(1<<PORTC5); //TWI config
	
	//Configure Bit Rate (TWBR and TWSR)
	TWBR = 18;	//TWBR=18
	TWSR = (0<<TWPS1)|(1<<TWPS0);	//PRESCALER = 1
	
	//Configure TWI Interface (TWCR)
	TWCR = (1 << TWEN);

	//INITIALIZE LCD
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	_delay_ms(15);
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	_delay_ms(4.1);
	TWI(LCD_ADDRESS,0x30|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (data length of 8, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x30|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	_delay_ms(4.1);
	TWI(LCD_ADDRESS,0x20|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (data length of 4, number of lines=2, 5x8 digit space, load data)
	TWI(LCD_ADDRESS,0x20|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	TWI(LCD_ADDRESS,0x20|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (load data)
	TWI(LCD_ADDRESS,0x20|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	
	//HOME CURSOR
	TWI(LCD_ADDRESS,0x80|LCD_DISABLE|LCD_WRITE|LCD_BL);	// (load data)
	TWI(LCD_ADDRESS,0x80|LCD_ENABLE|LCD_WRITE|LCD_BL);	// (clock in data)
	
	//CLEAR DISPLAY AND WAIT TO FINSIH
	LCD_Command(CLEAR_DISPLAY);
	_delay_ms(2);
	
	LCD_Command(0x0C); //Display no cursor
	LCD_Command(0x06); //Automatic Increment

	//Configure Timer 1
	TCCR1A = 0x00;
	TCCR1B = (1 << WGM12)|(1 << CS10);
	OCR1A = 159; //1 us interrupts
	TIMSK1 = (1 << OCIE1A); //enabling interrupts

	//Configure Timer 0
	TCCR0A = (1 << COM0B1)|(0 << COM0B0)|(1 << WGM01)|(1 << WGM00);
	TCCR0B = (1 << WGM02)|(1 << CS01)|(1 << CS00); //1kHz frequency
	OCR0A = 249; //1kHz value
	OCR0B = 0; //0 initially, adjusted in ISR
	
	sei();
	
	while (1)
	{

	}
}


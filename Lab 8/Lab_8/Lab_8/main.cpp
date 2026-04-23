/*
 * Lab_8.cpp
 *
 * Created: 4/23/2026 10:22:57 AM
 * Author : Gabe Litteken
 */ 

#define  F_CPU 16000000  // 16 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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
}

ISR(ADC_vect) {
	uint16_t ADCVal = ADCL;
	ADCVal |= ADCH << 8; 
	
	//clearing display section
	LCD_Command(SET_ADDRESS|0x04);
	LCD_Display(' ');
	LCD_Display(' ');
	LCD_Display(' ');
	LCD_Display(' ');
	
	//displaying number
	LCD_Command(SET_ADDRESS|0x04);
	LCD_Display(lcd_num_code_dec((ADCVal/1000) % 10));
	LCD_Display(lcd_num_code_dec((ADCVal/100) % 10));
	LCD_Display(lcd_num_code_dec((ADCVal/10) % 10));
	LCD_Display(lcd_num_code_dec(ADCVal % 10));
	_delay_ms(2);
	
	//setting LEDs based on ADC values
	if (ADCVal > 964) {
		PORTD = 0b00000111;
	} else if (ADCVal > 682) {
		PORTD = 0b00000110;
	} else if (ADCVal > 342) {
		PORTD = 0b00000100;
	} else {
		PORTD = 0b00000000;
	}
	
	ADCSRA |= (1 << ADSC); //starting the ADC conversion again
}

int main(void)
{
	//Configure IO
	//YOUR CODE HERE
	DDRD = 0b00000111;
	
	//Configure Bit Rate (TWBR and TWSR)
	TWBR = 18;	//TWBR=18
	TWSR = (0<<TWPS1)|(1<<TWPS0);	//PRESCALER = 1
	
	//Configure TWI Interface (TWCR)
	TWCR = (1<<TWEN);
	
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
	
	LCD_Command(SET_ADDRESS|0x00);
	LCD_Display('V');
	LCD_Display('D');
	LCD_Display(':');
	
	//CONFIGURE A/D MODULE
	//YOUR CODE HERE
	ADMUX = (0 << REFS1)|(1 << REFS0);
	ADCSRA = (1 << ADEN)|(1 << ADSC)|(1 << ADIE)|(1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);
	sei();
		
	/* Replace with your application code */
	while (1)
	{
	}
}


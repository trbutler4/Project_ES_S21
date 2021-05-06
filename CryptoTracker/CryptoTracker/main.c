/*
 * CryptoTracker.c
 *
 * Created: 4/27/2021 5:32:56 PM
 * Author : Cole Brooks, Thomas Butler
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

// USART stuff
#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) -1)

// Alarm definitions
#define alarm_port		PORTB					// alarm connection
#define alarm_bit		PORTB0
#define alarm_ddr		DDRB

// LCD interface definitions
#define lcd_D7_port     PORTC                   // lcd D7 connection
#define lcd_D7_bit      PORTC3
#define lcd_D7_ddr      DDRC

#define lcd_D6_port     PORTC                   // lcd D6 connection
#define lcd_D6_bit      PORTC2
#define lcd_D6_ddr      DDRC

#define lcd_D5_port     PORTC                   // lcd D5 connection
#define lcd_D5_bit      PORTC1
#define lcd_D5_ddr      DDRC

#define lcd_D4_port     PORTC                   // lcd D4 connection
#define lcd_D4_bit      PORTC0
#define lcd_D4_ddr      DDRC

#define lcd_E_port      PORTB                   // lcd Enable pin
#define lcd_E_bit       PORTB3
#define lcd_E_ddr       DDRB

#define lcd_RS_port     PORTB                   // lcd Register Select pin
#define lcd_RS_bit      PORTB5
#define lcd_RS_ddr      DDRB

// LCD specific definitions
#define lcd_LineOne     0x00                    // start of line 1
#define lcd_LineTwo     0x40                    // start of line 2

// LCD instructions
#define lcd_Clear           0b00000001          // clears the screen
#define lcd_Home            0b00000010          // return cursor to first position on first line
#define lcd_EntryMode       0b00000110          // move cursor from left to right on read/write
#define lcd_DisplayOff      0b00001000          // turns display off
#define lcd_DisplayOn       0b00001100          // display on, cursor off, blink off
#define lcd_reset           0b00110000          // reset 
#define lcd_setTo4Bit       0b00101000          // sets to 4 bit data entry mode
#define lcd_SetCursor       0b10000000          // sets cursor position

// Prototypes
const char* get_string(char input_str[]);
char* store_prices(const char str[], char price_array[10][10]);
uint16_t usart_rx(void);
void usart_init(void);
void lcd_write(uint8_t);
void lcd_write_instruction(uint8_t);
void lcd_write_char(uint8_t);
void lcd_write_str(uint8_t *);
void lcd_init(void);
void move_to_line_2(void);
void alarm();

int currentCrypto = 0; // 0 denotes bitcoin, 1 denotes eth. Global for interrupt access

/////////////////////////////////////////////////
// Function: move_to_line_2
// Purpose: moves cursor to line 2
/////////////////////////////////////////////////
int main(void)
{	
	// storage variables
	int alarmPercent = 10; // default alarm to 10 percent change
	char cryptos[2][10] = { "Bitcoin",		// supported crypto names. Index = current crypto int
					"Ethereum"};
	char prices[10][10];
	
	// configure the data lines for output to LCD
    lcd_D7_ddr |= (1<<lcd_D7_bit);
    lcd_D6_ddr |= (1<<lcd_D6_bit);
    lcd_D5_ddr |= (1<<lcd_D5_bit);
    lcd_D4_ddr |= (1<<lcd_D4_bit);
	
	// configure data lines for output to alarm
	alarm_ddr |= (1<<alarm_bit);
	

	// configure the data lines for controlling the LCD
    lcd_E_ddr |= (1<<lcd_E_bit);        // Enable
    lcd_RS_ddr |= (1<<lcd_RS_bit);    // Register Select

	// init lcd and usart
    lcd_init();
	usart_init();

	// Type welcome message
    lcd_write_str("Welcome to");
    move_to_line_2();
    lcd_write_str("CryptoTicker");
	
	// display welcome message for 5 seconds
	// and then clear the screen
	_delay_ms(5000);
	lcd_write_instruction(lcd_Clear);
	_delay_ms(80);
	
	// set INT0 to trigger on ANY logic change
	EICRA |= (1 << ISC00);
	EIMSK |= (1 << INT0);
	sei();

    // main program loop
    while(1){
		// wait for prices to update
		char input_str[50];
		get_string(input_str);
		store_prices(input_str,prices);
		
		lcd_write_instruction(lcd_Clear);
		_delay_ms(80);
		
		lcd_write_str(cryptos[currentCrypto]);
		move_to_line_2();
		_delay_ms(80);
		lcd_write_str(prices[currentCrypto]);
    }
    return 0;
} ///////////////////// END OF MAIN //////////////////////////

/////////////////////////////////////////////////
// function: ISR()
// purpose: toggles the current crypto via interrupt
/////////////////////////////////////////////////
ISR(INT0_vect)
{
	switch(currentCrypto){
		case 0:
			currentCrypto = 1;
			break;
		case 1:
			currentCrypto = 0;
			break;
	}
}

/////////////////////////////////////////////////
// function: alarm
// purpose: sounds the alarm that the price dropped 
//			> 10 percent
/////////////////////////////////////////////////
void alarm()
{
	int i = 0;
	for(i; i < 10; i+=1){
		alarm_port |= (1<<alarm_bit);// turn alarm on
		_delay_ms(150);
		alarm_port &= ~(1<<alarm_bit);// turn alarm off
		_delay_ms(150);
	}
}


/////////////////////////////////////////////////
// function: usart_init
// purpose: initialize usart.
// - set baud rate
// - enable rx and tx
// - set frame format 8 data, 2 stop bit
/////////////////////////////////////////////////
void usart_init(void)
{
	// Set baud rate
	UBRR0H = (uint8_t)(UBRR_VALUE>>8);
	UBRR0L = (uint8_t)UBRR_VALUE;
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);
	//enable transmission and reception
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0);
}

/////////////////////////////////////////////////
// function: usart_rx
// purpose: receives data from bluetooth module
/////////////////////////////////////////////////
uint16_t usart_rx(void)
{
	// Wait for byte to be received
	while(!(UCSR0A&(1<<RXC0))){};
	// Return received data
	return UDR0;
}


/////////////////////////////////////////////////
// function: get_string
// purpose: gets string from bluetooth module
/////////////////////////////////////////////////
const char* get_string(char input_str[]){
	char buffer[10];
	uint16_t input = usart_rx();
	
	int i = 0;
	while (input != '\n'){
		itoa(input, buffer, 10);
		input_str[i] = atoi(buffer);
		i = i + 1;
		input = usart_rx();
	}
	
	return input_str;
}

//////////////////////////
// function: store_prices
// purpose: gets price string and converts string to array of prices
char* store_prices(const char str[], char price_array[10][10]){
	const char s[2] = ",";
	char *token;
	int i = 0;
	char buffer[10];
	/* get the first token */
	token = strtok(str, s);
	
	/* walk through other tokens */
	while( token != NULL ) {
		sprintf(price_array[i], " %s", token );
		
		i = i + 1;
		token = strtok(NULL, s);
	}
	
	return price_array;
}

/////////////////////////////////////////////////
// function: lcd_init
// purpose: initializes the LCD in 4-bit data mode
/////////////////////////////////////////////////
void lcd_init(void)
{
  // delay for a bit so hardware can do it's thing
    _delay_ms(100);    
                               
    // note we start in 8 bit mode, so we gotta change that
    // Set up the RS and E lines for the 'lcd_write' subroutine.
    lcd_RS_port &= ~(1<<lcd_RS_bit);
    lcd_E_port &= ~(1<<lcd_E_bit);                  

    // Setup the LCD
    lcd_write(lcd_reset); // first part of reset sequence
    _delay_ms(10);

    lcd_write(lcd_reset); // second part of reset sequence
    _delay_us(200);

    lcd_write(lcd_reset); // third part of reset sequence
    _delay_us(200); 
 
    lcd_write(lcd_setTo4Bit); // set 4-bit mode
    _delay_us(80);

    // Function Set instruction
    lcd_write_instruction(lcd_setTo4Bit); 
    _delay_us(80);  

    // Display On
    lcd_write_instruction(lcd_DisplayOff);        
    _delay_us(80); 

    // Clear Display
    lcd_write_instruction(lcd_Clear);            
    _delay_ms(4);                                  

    // Entry Mode Set instruction
    lcd_write_instruction(lcd_EntryMode);  
    _delay_us(80);                                  
 
    // Display On/Off Control instruction
    lcd_write_instruction(lcd_DisplayOn);         
    _delay_us(80);                                 
}

/////////////////////////////////////////////////
// Function: move_to_line_2
// Purpose: moves cursor to line 2
/////////////////////////////////////////////////
void move_to_line_2(void){
  lcd_write_instruction(lcd_SetCursor | lcd_LineTwo);
  _delay_us(80);                                  // 40 uS delay (min)
}

/////////////////////////////////////////////////
// function: lcd_write_str
// purpose: sends a string to the LCD to be 
// displayed
/////////////////////////////////////////////////
void lcd_write_str(uint8_t theString[])
{
    volatile int i = 0;                             // character counter*/
    while (theString[i] != 0)
    {
        lcd_write_char(theString[i]);
        i++;
        _delay_us(80);                              // 40 uS delay (min)
    }
}

/////////////////////////////////////////////////
// function: lcd_write_char
// purpose: send a byte nibble by nibble as
// a character
/////////////////////////////////////////////////
void lcd_write_char(uint8_t theData)
{
    lcd_RS_port |= (1<<lcd_RS_bit);                 // select the Data Register (RS high)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write(theData);                           // write the upper 4-bits of the data
    lcd_write(theData << 4);                      // write the lower 4-bits of the data
}

/////////////////////////////////////////////////
// function: lcd_write_instruction
// purpose: send a byte nibble by nibble to
// the LCD as an instruction
/////////////////////////////////////////////////
void lcd_write_instruction(uint8_t theInstruction)
{
    lcd_RS_port &= ~(1<<lcd_RS_bit);                // select the Instruction Register (RS low)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // make sure E is initially low
    lcd_write(theInstruction);                    // write the upper 4-bits of the data
    lcd_write(theInstruction << 4);               // write the lower 4-bits of the data
}

/////////////////////////////////////////////////
// function: lcd_write
// purpose: send a byte nibble by nibble to the LCD
/////////////////////////////////////////////////
void lcd_write(uint8_t theByte)
{
    lcd_D7_port &= ~(1<<lcd_D7_bit);                        // assume that data is '0'
    if (theByte & 1<<7) lcd_D7_port |= (1<<lcd_D7_bit);     // make data = '1' if necessary

    lcd_D6_port &= ~(1<<lcd_D6_bit);                        // repeat for each data bit
    if (theByte & 1<<6) lcd_D6_port |= (1<<lcd_D6_bit);

    lcd_D5_port &= ~(1<<lcd_D5_bit);
    if (theByte & 1<<5) lcd_D5_port |= (1<<lcd_D5_bit);

    lcd_D4_port &= ~(1<<lcd_D4_bit);
    if (theByte & 1<<4) lcd_D4_port |= (1<<lcd_D4_bit);

    lcd_E_port |= (1<<lcd_E_bit);                   // Enable pin high
    _delay_us(1);                                   // implement 'Data set-up time' (80 nS) and 'Enable pulse width' (230 nS)
    lcd_E_port &= ~(1<<lcd_E_bit);                  // Enable pin low
    _delay_us(1);                                   // implement 'Data hold time' (10 nS) and 'Enable cycle time' (500 nS)
}
#ifndef F_CPU
#define F_CPU 16000000UL //set clock speed to 16MHz

#endif

#include <avr/io.h> //AVR header
#include <util/delay.h> //delay header

#define LCD_DATA PORTL //PORTH selected as LCD DATA port
#define ctrl PORTB //PORTB selected as command port

#define en PB5 //enable command port
#define rs PB6 //resistor select command port

//declarations
void LCD_cmd(unsigned char cmd);
void init_LCD(void);
void LCD_write(unsigned char data);
void LCD_clear(void);
void LCD_Print (char *str);
void LCD_Printpos (char row, char pos, char *str);


// initializing main parameters
int menu = 1;
int subMenu = 0;
int fileCount = 0;

int main(void){
	DDRL = 0xF0; //set data port as output
	DDRB = 0b01100000; //set command port as output
	
	//Buttons
	//DDRC = DDRC & ~(1<<6); //OK button
	//DDRC = DDRC & ~(1<<4); //BACK button
	//DDRC = DDRC & ~(1<<7); //UP button
	//DDRC = DDRC & ~(1<<5); //DOWN button
	//PORTC = 0xF0; //enable pull up resistors
	init_LCD();
	LCD_cmd(0x80);
	LCD_Print("welcome");
}
void init_LCD(void){
	LCD_cmd(0x28); //initializing 4 bit mode of 20x4 LCD*************
	_delay_ms(2);
	LCD_cmd(0x01); //make clear LCD
	_delay_ms(1);
	LCD_cmd(0x02); //return Home
	_delay_ms(1);
	LCD_cmd(0x06); // make increment in cursor
    _delay_ms(1);
    LCD_cmd(0x0c); // cursor off
    _delay_ms(1);
	//initialization complete	
}

//function to send commands
void LCD_cmd(unsigned char cmd){
   LCD_DATA = (LCD_DATA & 0x0F) | (cmd & 0xF0); //last 4 bits
   LCD_DATA &= ~ (1<<rs); //set for commands
   LCD_DATA |= (1<<en);
   _delay_us(1);
   LCD_DATA &= ~ (1<<en);
   _delay_us(200);
   LCD_DATA = (LCD_DATA & 0x0F) | (cmd << 4); // first 4 bits of the cmd
   LCD_DATA |= (1<<en);
   _delay_us(1);
   LCD_DATA &= ~ (1<<en);
   _delay_ms(2);
   return;

}

//function to send data
void LCD_write(unsigned char data){
	   LCD_DATA = (LCD_DATA & 0x0F) | (data & 0xF0); //last 4 bits
	   LCD_DATA |= (1<<rs); //set for data
	   LCD_DATA |= (1<<en);
	   _delay_us(1);
	   LCD_DATA &= ~ (1<<en);
	   _delay_us(200);
	   LCD_DATA = (LCD_DATA & 0x0F) | (data << 4); // first 4 bits of the cmd
	   LCD_DATA |= (1<<en);
	   _delay_us(1);
	   LCD_DATA &= ~ (1<<en);
	   _delay_ms(2);
	return;
}

void LCD_clear(void){
   LCD_cmd (0x01);		//Clear LCD
   _delay_ms(2);			//Wait to clean LCD
   LCD_cmd (0x80);		//Move to Position Line 1, Position 1

}

void LCD_Print (char *str)
{
	int i;
	for(i=0; str[i]!=0; i++)// loop to print the string
	{
		LCD_DATA = (LCD_DATA & 0x0F) | (str[i] & 0xF0);
		LCD_DATA |= (1<<rs);
		LCD_DATA|= (1<<en);
		_delay_us(1);
		LCD_DATA &= ~ (1<<en);
		_delay_us(200);
		LCD_DATA = (LCD_DATA & 0x0F) | (str[i] << 4);
		LCD_DATA |= (1<<en);
		_delay_us(1);
		LCD_DATA &= ~ (1<<en);
		_delay_ms(2);
	}
}

//Write on a specific location
void LCD_Printpos (char row, char pos, char *str)
{
	if (row == 0 && pos<16)
	LCD_cmd((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_cmd((pos & 0x0F)|0xC0);
	else if (row==2 && pos<16)
	LCD_cmd((pos&0x0F)|0x94);
	else if(row==3 && pos<16)
	LCD_cmd((pos & 0x0F)|0xD4);
	LCD_Print(str);
	
	





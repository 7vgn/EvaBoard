/*
 * Testing the LCD of the Evaluation Board
 *
 * Connect the LCD (J15) to Port B (J12) with an 8-pole cable (twisted). That
 * is, connect R/W to Port B6, EN to Port B5, RS to Port B4, DB7 to Port B3,
 * DB6 to Port B2, DB5 to Port B1, and DB4 to Port B0. Attach a 2x16 LCD to J16. 
 */

#include<avr/io.h>
#include<util/delay.h>
#include"lcd.h"

void main(void)
{
	// Initialisation
	lcd_init();

	// 1. Print welcome message
	printf("Hello world!");
	_delay_ms(2000);

	// 2. Bar graph filling up from 0% to 100%
	for(uint8_t percent = 0; percent <= 100; percent++)
	{
		// Show bar graph
		lcd_drawBar(percent);

		// Write percentage in line 2
		lcd_line2();
		printf("%d%%", percent);

		// Wait a little
		_delay_ms(100);
	}
	_delay_ms(2000);

	// 3. Try some special characters
	lcd_clear();
	printf("Tilde: ~\nBackslash: \\");
	_delay_ms(1000);
	lcd_clear();
	printf("Left Arrow: ←\nRight Arrow: →");
	_delay_ms(1000);
	lcd_clear();
	printf("Umlaut: äöü\nGreek: αβεμσρθπ");
	_delay_ms(1000);
	lcd_clear();
	printf("Misc: ÷√⅟°∃□¢∞");
	_delay_ms(2000);

	// 4. Animation
	lcd_clear();
	printf("Animation:");
	lcd_line2();
	lcd_writeProgString(PSTR(""));
	for(uint8_t i = 0; i < 20; i++)
	{
		switch(i % 8)
		{
		case 0: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0)); break;
		case 1: lcd_registerCustomChar(7, CUSTOM_CHAR(0b01000, 0b01000, 0b00100, 0b00100, 0b00100, 0b00010, 0b00010, 0)); break;
		case 2: lcd_registerCustomChar(7, CUSTOM_CHAR(0b10000, 0b10000, 0b01000, 0b00100, 0b00010, 0b00001, 0b00001, 0)); break;
		case 3: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00000, 0b00000, 0b11000, 0b00100, 0b00011, 0b00000, 0b00000, 0)); break;
		case 4: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000, 0)); break;
		case 5: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00000, 0b00000, 0b00011, 0b00100, 0b11000, 0b00000, 0b00000, 0)); break;
		case 6: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00001, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b10000, 0)); break;
		case 7: lcd_registerCustomChar(7, CUSTOM_CHAR(0b00010, 0b00010, 0b00100, 0b00100, 0b00100, 0b01000, 0b01000, 0)); break;
		}
		_delay_ms(250);
	}
	_delay_ms(2000);

	// 5. Line and page break
	lcd_clear();
	for(uint8_t i = 0; i < 104; i++)
	{
		lcd_writeChar('a' + (i % 26));
		_delay_ms(100);
	}
	_delay_ms(2000);

	// 6. Numbers
	lcd_clear();
	lcd_writeString("Hex:");
	lcd_line2();
	lcd_writeString("Dec:");
	for(uint8_t nibble = 8; nibble < 15; nibble++)
	{
		lcd_goto(1, 6);
		lcd_writeHexNibble(nibble);
		lcd_goto(2, 6);
		lcd_writeDec(nibble);
		_delay_ms(500);
	}
	lcd_clear();
	lcd_writeString("Hex:");
	lcd_line2();
	lcd_writeString("Dec:");
	for(uint8_t byte = 125; byte < 132; byte++)
	{
		lcd_goto(1, 6);
		lcd_writeHexByte(byte);
		lcd_goto(2, 6);
		lcd_writeDec(byte);
		_delay_ms(500);
	}
	lcd_clear();
	lcd_writeString("Hex:");
	lcd_line2();
	lcd_writeString("Dec:");
	for(uint16_t word = 4093; word < 4100; word++)
	{
		lcd_goto(1, 6);
		lcd_writeHexWord(word);
		lcd_goto(2, 6);
		lcd_writeDec(word);
		_delay_ms(500);
	}
	lcd_clear();
	lcd_writeString("Hex:");
	lcd_line2();
	lcd_writeString("Dec:");
	for(uint16_t word = 4093; word < 4100; word++)
	{
		lcd_goto(1, 6);
		lcd_writeHex(word);
		lcd_goto(2, 6);
		lcd_writeDec(word);
		_delay_ms(500);
	}
	_delay_ms(2000);

	// 7. Finished
	lcd_clear();
	lcd_writeString("  ~ Finished ~  ");
	while(1);
}


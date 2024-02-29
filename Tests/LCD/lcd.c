/**
 * \file lcd.c
 * \brief AVR driver for HD44780-compatible 2x16 LCDs with 5x7 characters
 * \see https://cdn-shop.adafruit.com/datasheets/HD44780.pdf
 *
 * This driver can use either delays or read the busy flag to determine whether
 * the LCD can accept new commands or data. In order to work without delays,
 * the R/W line must be connected. 
 * It operates in 4-bit mode, meaning the DB[3:0] lines are not used. All lines
 * can be connected to arbitrary GPIO pins of the AVR. The following pins are
 * used:
 * - RS
 * - EN
 * - R/W
 * - DB[7:4]
 */

#include<avr/io.h>
#include<avr/pgmspace.h>
#include<util/atomic.h>
#include"lcd.h"

//=============================================================================
// Driver configuration

/*
 * For microsecond delays, the driver makes use of the _delay_us() function
 * from util/delay.h which means F_CPU must be defined and have the correct
 * value (CPU frequency in Mhz). 
 */
#ifdef F_CPU
#include<util/delay.h>
#else
#error "F_CPU is not defined"
#endif

/*
 * For millisecond delays, _delay_ms() is used by default but if another option
 * (e.g. timer-based) has been defined, that is preferred. 
 */
#ifndef delayMs
#define delayMs(TIME) _delay_ms(TIME)
#endif

/*
 * If ports and pins have been selected in the header file, use those. 
 * Otherwise they can be chosen individually. 
 */
#if defined LCD_PORT_DDR && defined LCD_PORT_DATA && defined LCD_PIN
// Use predefined port and default pins
#define RS_REG_DDR LCD_PORT_DDR
#define RS_REG_PORT LCD_PORT_DATA
#define RS_PIN 4
#define RW_REG_DDR LCD_PORT_DDR
#define RW_REG_PORT LCD_PORT_DATA
#define RW_PIN 6
#define EN_REG_DDR LCD_PORT_DDR
#define EN_REG_PORT LCD_PORT_DATA
#define EN_PIN 5
#define DB4_REG_DDR LCD_PORT_DDR
#define DB4_REG_PORT LCD_PORT_DATA
#define DB4_REG_PIN LCD_PIN
#define DB4_PIN 0
#define DB5_REG_DDR LCD_PORT_DDR
#define DB5_REG_PORT LCD_PORT_DATA
#define DB5_REG_PIN LCD_PIN
#define DB5_PIN 1
#define DB6_REG_DDR LCD_PORT_DDR
#define DB6_REG_PORT LCD_PORT_DATA
#define DB6_REG_PIN LCD_PIN
#define DB6_PIN 2
#define DB7_REG_DDR LCD_PORT_DDR
#define DB7_REG_PORT LCD_PORT_DATA
#define DB7_REG_PIN LCD_PIN
#define DB7_PIN 3
#endif

// Make sure everything is defined
#if !(defined RS_REG_DDR) || !(defined RS_REG_PORT) || !(defined RS_PIN)
#error "The RS port and/or pin was not defined"
#endif

#if (defined LCD_BUSY_TIMEOUT) && (!(defined RW_REG_DDR) || !(defined RW_REG_PORT) || !(defined RW_PIN))
#error "The RW port and/or pin was not defined"
#endif

#if !(defined EN_REG_DDR) || !(defined EN_REG_PORT) || !(defined EN_PIN)
#error "The EN port and/or pin was not defined"
#endif

#if !(defined DB4_REG_DDR) || !(defined DB4_REG_PORT) || !(defined DB4_PIN)
#error "The DB4 port and/or pin was not defined"
#endif

#if !(defined DB5_REG_DDR) || !(defined DB5_REG_PORT) || !(defined DB5_PIN)
#error "The DB5 port and/or pin was not defined"
#endif

#if !(defined DB6_REG_DDR) || !(defined DB6_REG_PORT) || !(defined DB6_PIN)
#error "The DB6 port and/or pin was not defined"
#endif

#if !(defined DB7_REG_DDR) || !(defined DB7_REG_PORT) || !(defined DB7_PIN)
#error "The DB7 port and/or pin was not defined"
#endif

//=============================================================================
// Internal functions and variables

/**
 * \brief Buffer for UTF-8-encoded multi-byte characters
 */
uint32_t utf8Buffer = 0;

/**
 * \brief Sends a nibble (half byte) to the LCD
 * \param regSel Selects the instruction register (0) or the data register (1).
 * \param nibble Contains the nibble to be sent in its lower 4 bits
 */
static void sendNibble(uint8_t regSel, uint8_t nibble)
{
	// Register select
	RS_REG_PORT = (RS_REG_PORT & ~(1 << RS_PIN)) | (regSel << RS_PIN);
	// Put n[4:0] on DB[7:4]
	DB4_REG_PORT = (DB4_REG_PORT & ~(1 << DB4_PIN)) | (((nibble >> 0) & 1) << DB4_PIN);
	DB5_REG_PORT = (DB5_REG_PORT & ~(1 << DB5_PIN)) | (((nibble >> 1) & 1) << DB5_PIN);
	DB6_REG_PORT = (DB6_REG_PORT & ~(1 << DB6_PIN)) | (((nibble >> 2) & 1) << DB6_PIN);
	DB7_REG_PORT = (DB7_REG_PORT & ~(1 << DB7_PIN)) | (((nibble >> 3) & 1) << DB7_PIN);
	// Address setup time (min. 40 ns)
	_delay_us(1);
	// Drive EN high
	EN_REG_PORT |= (1 << EN_PIN);
	// Enable pulse width (min. 230 ns)
	_delay_us(1);
	// Pull EN low
	EN_REG_PORT &= ~(1 << EN_PIN);
	// Hold time (min. 10 ns) and (in parallel) min. 270 ns to get to 500 ns
	// total enable cycle time
	_delay_us(1);
}

/**
 * \brief Sends a whole byte to the LCD when it is in 4-bit mode
 * \param regSel Must be 0 for commands, 1 for data
 * \param c The byte to be sent
 * \param delay Number of microseconds to delay after sending the byte. 
 * Ignored if busy flag polling is enabled. 
 */
#ifdef LCD_BUSY_TIMEOUT
#define SEND_BYTE(regSel, c, delay) sendByte(regSel, c)
#else
#define SEND_BYTE(regSel, c, delay) sendByte(regSel, c); _delay_us(delay)
#endif

/**
 * \brief Sends a whole byte to the LCD when it is in 4-bit mode
 * \param regSel Must be 0 for commands, 1 for data
 * \param c The byte to be sent
 */
static void sendByte(uint8_t regSel, uint8_t c)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Send upper nibble
		sendNibble(regSel, c >> 4);
		// Send lower nibble
		sendNibble(regSel, c & 0x0f);

		// Poll busy flag
#ifdef LCD_BUSY_TIMEOUT
		// Pull RS low to read the busy flag
		RS_REG_PORT &= ~(1 << RS_PIN);
		// Configure DB[7:4] as inputs with pull-up
		// It is important to de this now, since some LCD controllers drive the
		// data lines immediately after R/W goes high. Others wait until they
		// get a pulse on EN. And still others drive the pins immediately but
		// the value is only valid after an EN pulse. 
		DB4_REG_PORT |= (1 << DB4_PIN);
		DB4_REG_DDR &= ~(1 << DB4_PIN);
		DB4_REG_PORT |= (1 << DB5_PIN);
		DB4_REG_DDR &= ~(1 << DB5_PIN);
		DB4_REG_PORT |= (1 << DB6_PIN);
		DB4_REG_DDR &= ~(1 << DB6_PIN);
		DB4_REG_PORT |= (1 << DB7_PIN);
		DB4_REG_DDR &= ~(1 << DB7_PIN);
		// Now drive R/W high
		RW_REG_PORT |= (1 << RW_PIN);
		// Address setup time (min. 60 ns)
		_delay_us(1);

		uint16_t attempts = 0;
		while(attempts++ < LCD_BUSY_TIMEOUT)
		{
			// Drive EN high
			EN_REG_PORT |= (1 << EN_PIN);
			// Enable pulse width (min. 230 ns)
			_delay_us(1);
			// Read busy flag from DB7
			uint8_t busy = (DB7_REG_PIN >> DB7_PIN) & 1;
			// Pull EN low
			EN_REG_PORT &= ~(1 << EN_PIN);
			// Hold time (min. 10 ns) and (in parallel) min. 270 ns to get to 500 ns
			// total enable cycle time
			_delay_us(1);

			// The same again for the second nibble, which we ignore entirely. 
			// This might be unnecessary for some controllers but it can't hurt. 
			EN_REG_PORT |= (1 << EN_PIN);
			_delay_us(1);
			EN_REG_PORT &= ~(1 << EN_PIN);
			_delay_us(1);

			// Exit loop if LCD not busy anymore
			if(!busy)
				break;
		}
	
		// Pull R/W low again
		RW_REG_PORT &= ~(1 << RW_PIN);
		// Configure data pins as outputs
		DB4_REG_DDR |= (1 << DB4_PIN);
		DB4_REG_DDR |= (1 << DB5_PIN);
		DB4_REG_DDR |= (1 << DB6_PIN);
		DB4_REG_DDR |= (1 << DB7_PIN);
		// Address setup time (min. 60 ns)
		_delay_us(1);
#endif
	}
}

/**
 * \brief Tracks the position of the (invisible) cursor, i.e. where the next
 * character will be displayed. 
 * 
 * Values are 0..15 for the first line and 16..31 for the second line. The
 * value 32 indicates position 0 except that we got there by rolling around. 
 * This means that the next write must clear the LCD first. 
 * 
 * In terms of actual addresses in DDRAM, the first line corresponds to
 * 0x00..0x0f and the second line to 0x40..0x4f. 
 */
uint8_t lcdCursor = 0;

/**
 * \brief Update the LCD's internal cursor after modifying lcdCursor
 */
static inline void updateCursor()
{
	// Calculate DDRAM address
	uint8_t address;
	if(lcdCursor < 16)
		address = lcdCursor;
	else if(lcdCursor < 32)
		address = 0x40 | (lcdCursor & 0x0f);
	else
		address = 0x00;
	// "Set DDRAM address" command: 1 A6 A5 A4 A3 A2 A1 A0
	// with A[6:0] being the address in DDRAM
	SEND_BYTE(0, 0b10000000 | address, 42);
}

/**
 * \brief Helper function for stdio
 */
static int lcd_putchar(const char c, FILE* stream)
{
	lcd_writeChar(c);
	return 0;
}

/**
 * \brief Create a FILE through which stdio can write to the LCD
 * 
 * The first call to FDEV_SETUP_STREAM with _FDEV_SETUP_WRITE will
 * automatically assign the result to stdout and stderr. This might not be
 * desired. Additionally, if other drivers (e.g. UART) also create FILEs, it is
 * unclear which one gets to become stdout/stderr. For that reason, we assign
 * stdout and/or stderr manually in lcd_init(), depending on settings in lcd.h.
 */
static FILE lcdOut = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

//=============================================================================
// Public functions and variables

//-----------------------------------------------------------------------------
// Initialisation

void lcd_init(void)
{
	// Configure all pins as output, low
#if (defined RW_REG_PORT) && (defined RW_REG_DDR) && (defined RW_PIN)
	RW_REG_PORT &= ~(1 << RW_PIN);
	RW_REG_DDR |= (1 << RW_PIN);
#endif
	RS_REG_PORT &= ~(1 << RS_PIN);
	RS_REG_DDR |= (1 << RS_PIN);
	EN_REG_PORT &= ~(1 << EN_PIN);
	EN_REG_DDR |= (1 << EN_PIN);
	DB4_REG_PORT &= ~(1 << DB4_PIN);
	DB4_REG_DDR |= (1 << DB4_PIN);
	DB5_REG_PORT &= ~(1 << DB5_PIN);
	DB5_REG_DDR |= (1 << DB5_PIN);
	DB6_REG_PORT &= ~(1 << DB6_PIN);
	DB6_REG_DDR |= (1 << DB6_PIN);
	DB7_REG_PORT &= ~(1 << DB7_PIN);
	DB7_REG_DDR |= (1 << DB7_PIN);

	// Power on delay: The LCD needs up to 15ms to complete its reset
	delayMs(15);

	//-------------------------------------------------------------------------
	// Start of homing sequence
	// The goal is to put the LCD reliably into 4-bit mode regardless of its
	// current state. Keep in mind the LCD does not necessarily reset when the
	// uC does. 
	// Since we're not yet synchronised, we can't read the busy bit and have to
	// do everything via timing. 
	//
	// The relevant command is "Function set": 0 0 1 DL N F * * (order DB7:0)
	// DL=1 turns the interface to 8-bit mode and DL=0 to 4-bit mode. 
	// N and F control 1/2-line mode and 5x8/5x11 character size, respectively.
	// N and F don't matter for now, we can set them later once we're synced. 
	// The *'s are don't cares. 
	//
	// There are three states the LCD could potentially be in:
	// a) 8-bit mode
	// b) 4-bit mode with the next nibble being the upper half of a byte
	// c) 4-bit mode with the next nibble being the lower half of a byte. This
	// might happen if the uC was reset after sending only one of two nibbles.
	// 
	// The following comments describe what happens in each of these 3 cases.

	// Send 0b0011 on DB7:4. This causes the following to happen:
	// a) 0b0011**** is received and executed. The LCD remains in 8-bit mode. 
	// b) 0b0011 is received and stored as the first half of a command. 
	// c) 0b0011 is received and together with the last transmission, a command
	//    0b****0011 is executed. We have no idea what that does. 
	sendNibble(0, 0b0011);
	// Wait 4.1ms (enough time for any kind of command to finish)
	delayMs(5);

	// Send 0b0011 on DB7:4. This causes the following to happen:
	// a) 0b0011**** is received and executed. The LCD remains in 8-bit mode. 
	// b) 0b0011 is received and together with the last transmission, the
	//    command 0b00110011 is executed, putting the LCD into 8-bit mode. 
	// c) 0b0011 is received and stored as the first half of a command. 
	sendNibble(0, 0b0011);
	// Wait 100 us (enough time for 0b0011**** command to finish)
	_delay_us(100);

	// Send 0b0011 on DB7:4. This causes the following to happen:
	// a) 0b0011**** is received and executed. The LCD remains in 8-bit mode. 
	// b) 0b0011**** is received and executed. The LCD remains in 8-bit mode. 
	// c) 0b0011 is received and together with the last transmission, the
	//    command 0b00110011 is executed, putting the LCD into 8-bit mode. 
	sendNibble(0, 0b0011);
	// Wait 100 us (enough time for 0b0011**** command to finish)
	_delay_us(100);

	// Send 0b0010. Since the LCD is now in 8-bit mode, the command 0b0010****
	// is executed, putting the LCD into 4-bit mode. 
	sendNibble(0, 0b0010);
	// Wait 42 us
	_delay_us(42);
	// End of homing sequence. The LCD is now in 4-bit mode. 
	//-------------------------------------------------------------------------

	// "Function set" command: 0 0 1 DL N F * *
	// with DL=0 (4 bit mode), N=1 (2 lines), F=0 (5x8 characters)
	SEND_BYTE(0, 0b00101000, 42);
	// "Display on/off" command: 0 0 0 0 1 D B C
	// with D=0 (Display off), B=0 (no blinking), C=0 (cursor off)
	SEND_BYTE(0, 0b00001000, 42);
	// Clear display
	lcd_clear();
	// "Entry mode set" command: 0 0 0 0 0 1 I/D S
	// with I/D=1 (cursor moving right), S=0 (no shifting)
	SEND_BYTE(0, 0b00000110, 42);
	// "Display on/off" command: 0 0 0 0 1 D B C
	// with D=1 (Display on), B=0 (no blinking), C=0 (cursor off)
	SEND_BYTE(0, 0b00001100, 42);
	
    // Register custom characters
#ifdef LCD_CC_IXI
    lcd_registerCustomChar(LCD_CC_IXI, LCD_CC_IXI_BITMAP);
#endif
#ifdef LCD_CC_TILDE
    lcd_registerCustomChar(LCD_CC_TILDE, LCD_CC_TILDE_BITMAP);
#endif
#ifdef LCD_CC_BACKSLASH
    lcd_registerCustomChar(LCD_CC_BACKSLASH, LCD_CC_BACKSLASH_BITMAP);
#endif
	
	// Redirect stdout and/or stderr to LCD
#ifndef LCD_NO_STDOUT_REDIRECT
	stdout = &lcdOut;
#endif
#ifndef LCD_NO_STDERR_REDIRECT
	stderr = &lcdOut;
#endif
}

//-----------------------------------------------------------------------------
// Cursor movement

void lcd_line1(void)
{
	lcdCursor = 0;
	updateCursor();
}

void lcd_line2(void)
{
	lcdCursor = 16;
	updateCursor();
}

void lcd_goto(unsigned char row, unsigned char column)
{
	// Boundary checks on row and column
	if(row < 1) row = 1;
	if(row > 2) row = 2;
	if(column < 1) column = 1;
	if(column > 16) column  = 16;
	lcdCursor = ((row - 1) << 4) | (column - 1);
	updateCursor();
}

void lcd_move(char row, char column)
{
	// Add row and column to current cursor (row mod 2, column mod 16)
	uint8_t newRow = ((lcdCursor >> 4) + (row + 1)) & 1;
	uint8_t newCol = (lcdCursor + (column + 15)) & 0x0f;
	lcdCursor = (newRow << 4) | newCol;
	updateCursor();
}

void lcd_back(void)
{
	if(lcdCursor == 0)
		lcdCursor = 31;
	else
		lcdCursor--;
	updateCursor();
}

void lcd_home(void)
{
	lcdCursor &= 0x10;
	updateCursor();
}

void lcd_forward(void)
{
	if(lcdCursor == 31)
		lcdCursor = 0;
	else
		lcdCursor++;
	updateCursor();
}

//-----------------------------------------------------------------------------
// Erasing

void lcd_clear(void)
{
	// "Clear Display" command (also returns cursor to 0): 0 0 0 0 0 0 0 1
	SEND_BYTE(0, 0b00000001, 1640);
	lcdCursor = 0;
}

void lcd_erase(uint8_t line)
{
	// Save current cursor position
	uint8_t cursorBackup = lcdCursor;
	// Erase the given line
	lcd_goto(line, 1);
	lcd_writeProgString(PSTR("                "));
	// Set cursor back to original position
	lcdCursor = cursorBackup;
	updateCursor();
}

//-----------------------------------------------------------------------------
// Writing

void lcd_writeChar(char character)
{
	// Add to UTF-8 buffer
	utf8Buffer = (utf8Buffer << 8) | (uint8_t)character;
	// Check if the buffer now holds a complete UTF-8 character
	uint32_t codePoint = 0x0000fffd; // Default for characters the LCD cannot display
	if((utf8Buffer & 0xf8000000) == 0xf0000000)
	{
		// 4-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
		if((utf8Buffer & 0x00c0c0c0) == 0x00808080)
			codePoint = ((utf8Buffer & 0x07000000) >> 6)
			          | ((utf8Buffer & 0x003f0000) >> 4)
					  | ((utf8Buffer & 0x00003f00) >> 2)
					  | (utf8Buffer & 0x0000003f);
	}
	else if((utf8Buffer & 0xfff00000) == 0x00e00000)
	{
		// 3-byte character (1110xxxx 10xxxxxx 10xxxxxx)
		if((utf8Buffer & 0x0000c0c0) == 0x00008080)
			codePoint = ((utf8Buffer & 0x000f0000) >> 4)
					  | ((utf8Buffer & 0x00003f00) >> 2)
					  | (utf8Buffer & 0x0000003f);
	}
	else if((utf8Buffer & 0xffffe000) == 0x0000c000)
	{
		// 2-byte character (110xxxxx 10xxxxxx)
		if((utf8Buffer & 0x000000c0) == 0x00000080)
			codePoint = ((utf8Buffer & 0x00001f00) >> 2)
					  | (utf8Buffer & 0x0000003f);
	}
	else if((utf8Buffer & 0xffffff80) == 0x00000000)
	{
		// 1-byte character (0xxxxxxx)
		codePoint = utf8Buffer;
	}
	else
		// Incomplete character, wait for more before writing
		return;
	utf8Buffer = 0;
	
	// Handle '\n' character
	if(codePoint == '\n')
	{
		// When in line 1, go to line 2
		if(lcdCursor < 16)
			lcdCursor = 16;
		// When in line 2, roll over
		else
			lcdCursor = 32;
		updateCursor();
	}
	else
	{
		// Map codePoint to a single byte that the LCD can display
		// Most HD44780s have a CGROM with Japanese characters in addition to
		// an almost complete set of ASCII characters (with only backslash and
		// tilde missing). Some come with a fully western CGROM (i.e. more
		// Umlauts and accented characters), but those are pretty rare. 
		uint8_t lcdCode;
		switch(codePoint)
		{
#ifdef LCD_CC_BACKSLASH
		case 0x0000005c: lcdCode = LCD_CC_BACKSLASH; break; // Backslash (\)
#endif
#ifdef LCD_CC_TILDE
		case 0x0000007e: lcdCode = LCD_CC_TILDE; break; // Tilde ~
#endif
#ifdef LCD_CC_IXI
		case 0x0000217a: lcdCode = LCD_CC_IXI; break; // IXI department logo (ⅺ)
#endif
		case 0x0000009d: lcdCode = 0x5c; break; // The Yen sign (¥) is where the backslash is supposed to be
		case 0x00002192: lcdCode = 0x7e; break; // The right arrow (→) is where the tilde is supposed to be
		case 0x00002190: lcdCode = 0x7f; break; // Left arrow (←)
		case 0x00002092: lcdCode = 0xa1; break; // Subscript small o (ₒ)
		case 0x000000da: lcdCode = 0xa2; break; // Single up and left (┘)
		case 0x000000d9: lcdCode = 0xa3; break; // Single down and right (┌)
		case 0x000000b7: lcdCode = 0xa5; break; // Middle dot (·)
		case 0x00002203:
		case 0x0000018e: lcdCode = 0xae; break; // Existential quantifier (∃)
		case 0x000025af:
		case 0x000025a1: lcdCode = 0xdb; break; // Vertical white rectangle (▯) or white square (□)
		case 0x000000b0: lcdCode = 0xdf; break; // Degree sign (°)
		case 0x000003b1: lcdCode = 0xe0; break; // Lowercase alpha (α)
		case 0x000000e4: lcdCode = 0xe1; break; // Lowercase umlaut a (ä)
		case 0x000003b2:
		case 0x000000df: lcdCode = 0xe2; break; // Lowercase beta (β) or German Eszett (ß)
		case 0x000003b5: 
		case 0x00000190: lcdCode = 0xe3; break; // Lowercase epsilon (ε)
		case 0x000003bc:
		case 0x000000b5: lcdCode = 0xe4; break; // Lowercase mu (μ) or micro sign (µ)
		case 0x000003c3: lcdCode = 0xe5; break; // Lowercase sigma (σ)
		case 0x000003c1: lcdCode = 0xe6; break; // Lowercase rho (ρ)
		case 0x0000221a: lcdCode = 0xe8; break; // Square root symbol (√)
		case 0x0000215f: lcdCode = 0xe9; break; // Inverse Symbol (no unicode equivalent, we'll use ⅟ instead)
		case 0x000000a2: lcdCode = 0xec; break; // Cent sign (¢)
		case 0x000000f1: lcdCode = 0xee; break; // Lowercase n with tilde (ñ)
		case 0x000000f6: lcdCode = 0xef; break; // Lowercase umlaut o (ö)
		case 0x000003b8: lcdCode = 0xf2; break; // Lowercase theta (θ)
		case 0x0000221e: lcdCode = 0xf3; break; // Infinity symbol (∞)
		case 0x000003a9: lcdCode = 0xf4; break; // Uppercase omega (Ω)
		case 0x000000fc: lcdCode = 0xf5; break; // Lowercase umlaut u (ü)
		case 0x000003a3: lcdCode = 0xf6; break; // Uppercase sigma (Σ)
		case 0x000003c0: lcdCode = 0xf7; break; // Lowercase pi (π)
		case 0x000000f7: lcdCode = 0xfd; break; // Division sign (÷)
		case 0x000025ae:
		case 0x000025a0: lcdCode = 0xff; break; // Vertical black rectangle (▮) or black square (■)
		default: if(codePoint <= 0x00000080) lcdCode = (uint8_t)codePoint; else lcdCode = 0xff;
		}

		// If current line is full, break automatically
		if(lcdCursor == 32)
			lcd_clear();
		else if(lcdCursor == 16)
			lcd_line2();

		// Write character
		SEND_BYTE(1, lcdCode, 46);
		lcdCursor++;
	}
}

void lcd_writeHexNibble(uint8_t number)
{
	number &= 0x0f;
	lcd_writeChar(number <= 9 ? '0' + number : 'a' + number - 10);
}

void lcd_writeHexByte(uint8_t number)
{
	lcd_writeHexNibble(number >> 4);
	lcd_writeHexNibble(number & 0x0f);
}

void lcd_writeHexWord(uint16_t number)
{
	lcd_writeHexByte(number >> 8);
	lcd_writeHexByte(number & 0x00ff);
}

void lcd_writeHex(uint16_t number)
{
	if(number == 0)
		// The only number where a leading zero is ok
		lcd_writeChar('0');
	else
	{
		int8_t shift = 8 * sizeof(number) - 4;
		while((number >> shift) == 0)
			shift -= 4;
		while(shift >= 0)
		{
			lcd_writeHexNibble((number >> shift) & 0xf);
			shift -= 4;
		}
	}
}

void lcd_write32bitHex(uint32_t number)
{
	lcd_writeProgString(PSTR("0x"));
	lcd_writeHexWord(number >> 16);
	lcd_writeHexWord(number & 0x0000ffff);
}

void lcd_writeDec(uint16_t number)
{
	if(number == 0)
		lcd_writeChar('0');
	uint16_t mask = 10000;
	while(number / mask == 0)
		mask /= 10;
	while(mask)
	{
		lcd_writeChar('0' + number / mask);
		number %= mask;
		mask /= 10;
	}
}

void lcd_writeString(const char* text)
{
	while(*text)
		lcd_writeChar(*text++);
}

void lcd_writeProgString(const char* string)
{
	char c;
	while((c = pgm_read_byte(string++)))
		lcd_writeChar(c);
}

void lcd_writeErrorProgString(const char* string)
{
	fprintf_P(stderr, string);
}

void lcd_drawBar(uint8_t percent)
{
	// Transform linearly from [0;100] to [0;16]
	if(percent > 100) percent = 100;
	percent = (uint8_t)((uint16_t)percent * 16 / 100);
	// Clear screen and draw bar in first line
	lcd_line1();
	while(percent--)
		lcd_writeProgString(PSTR("▮"));
	while(lcdCursor < 16)
		lcd_writeChar(' ');
	lcd_erase(2);
}

void lcd_writeVoltage(uint16_t voltage, uint16_t valueUpperBound, uint8_t voltUpperBound)
{
	// Calculate the voltage in millivolts
	uint16_t millivolts = (uint16_t)((uint32_t)voltage * 1000 * voltUpperBound / valueUpperBound);

	// Split the number into integer part and fractional digits
	uint16_t integer = millivolts / 1000;
	millivolts -= integer * 1000;
	uint8_t fraction1 = (uint8_t)(millivolts / 100);
	millivolts -= 100 * fraction1;
	uint8_t fraction2 = (uint8_t)(millivolts / 10);
	millivolts -= 10 * fraction2;
	uint8_t fraction3 = (uint8_t)millivolts;
	
	// Write to display
	lcd_writeDec(integer);
	lcd_writeChar('.');
	lcd_writeChar('0' + fraction1);
	lcd_writeChar('0' + fraction2);
	lcd_writeChar('0' + fraction3);
	lcd_writeChar('V');
}

//-----------------------------------------------------------------------------
// Custom characters

void lcd_registerCustomChar(uint8_t addr, uint64_t chr)
{
	// "Set CGRAM address" command: 0 1 A5 A4 A3 A2 A1 A0
	// with A[5:0]=the byte address in CGRAM (each character takes 8 bytes)
	SEND_BYTE(0, 0b01000000 | (8 * addr), 42);
	// Write 8 bytes of data
	for(uint8_t i = 0; i < 8; i++)
	{
		SEND_BYTE(1, (uint8_t)chr, 46);
		chr >>= 8;
	}
	// Move address pointer back to DDRAM, otherwise all following data writes
	// would go into CGRAM. 
	updateCursor();
}

//-----------------------------------------------------------------------------
// Miscellaneous

/**
 * \brief Pointer to FILE through which stdio functions can write to the LCD
 */
FILE* lcdout = &lcdOut;

void lcd_command(uint8_t command)
{
	SEND_BYTE(0, command, 1640 /* maximum delay for safety */);
}


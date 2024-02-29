/**
 * \file lcd.c
 * \brief AVR driver for HD44780-compatible 2x16 LCDs with 5x7 characters
 * 
 * This driver was written for the evaluation board used in the lab course
 * "Praktikum Systemprogrammierung" in the computer science curriculum at
 * RWTH Aachen. 
 * It is (as much as possible) compatible with the university-provided driver.
 * In particular, it can be used with its header file instead of this one. 
 * 
 * The original driver has a potential issue when used with some
 * HD44780-compatible LCD controllers: while querying the LCD's busy flag, it
 * configures both the LCD's data pins as well as the corresponding AVR pins as
 * outputs, causing a short. Although this lasts only for a single clock
 * period, it can still cause damage to the AVR or the LCD (or both). The risk
 * is even greater if the AVR runs at a slower clock speed than the usual
 * 20MHz, e.g., when the user forgets to set the fuses. 
 * 
 * This driver disables interrupts while sending a command to the LCD but
 * otherwise does nothing to ensure synchronisation. Make sure to use the
 * appropriate mechanisms if you use it in an environment where interruptions
 * could lead to race conditions. 
 */

#ifndef _LCD_H
#define _LCD_H

#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>

//=============================================================================
// Configuration

/**
 * \brief Configure delaying vs. polling busy flag
 * 
 * If LCD_BUSY_TIMEOUT is not defined, the driver uses sufficiently long delays
 * in between sending commands to the LCD. This is somewhat inefficient. 
 * In order to read the LCD's busy flag instead, define LCD_BUSY_TIMEOUT and
 * set it to the number of attempts that should be made to read the busy flag
 * before giving up. 
 */
//#define LCD_BUSY_TIMEOUT 2000

/**
 * \brief Port and pin definitions
 * 
 * Each line of the LCD can be assigned individually to a port pin of the AVR.
 * R/W is not necessary if the driver is configured to use delays instead of
 * polling the busy flag. In this case, R/W needs to be connected to ground. 
 */

// RS pin
#define RS_REG_DDR DDRA
#define RS_REG_PORT PORTA
#define RS_PIN 4

// R/W pin (If this is defined even though delays are used, it is pulled low)
#define RW_REG_DDR DDRA
#define RW_REG_PORT PORTA
#define RW_PIN 6

// EN pin
#define EN_REG_DDR DDRA
#define EN_REG_PORT PORTA
#define EN_PIN 5

// DB4 pin
#define DB4_REG_DDR DDRA
#define DB4_REG_PORT PORTA
#define DB4_REG_PIN PINA
#define DB4_PIN 0

// DB5 pin
#define DB5_REG_DDR DDRA
#define DB5_REG_PORT PORTA
#define DB5_REG_PIN PINA
#define DB5_PIN 1

// DB6 pin
#define DB6_REG_DDR DDRA
#define DB6_REG_PORT PORTA
#define DB6_REG_PIN PINA
#define DB6_PIN 2

// DB7 pin
#define DB7_REG_DDR DDRA
#define DB7_REG_PORT PORTA
#define DB7_REG_PIN PINA
#define DB7_PIN 3

/**
 * \brief Redirect stdout and/or stderr to the LCD
 * 
 * By default, both stdout and stderr are redirected to the LCD, so that you
 * can use stdio functions like printf(). 
 * If you do not want that, disable it here. 
 */
//#define LCD_NO_STDOUT_REDIRECT
#define LCD_NO_STDERR_REDIRECT

//=============================================================================
// Public functions

//-----------------------------------------------------------------------------
// Initialisation

/**
 * \brief This function must be called before any other of this driver
 * 
 * Configures the pins and initialises the LCD. This takes in the order of
 * dozens of milliseconds. 
 */
void lcd_init(void);

//-----------------------------------------------------------------------------
// Cursor movement (Cursor determines where the next character is displayed)

/**
 * \brief Sets the cursor to the beginning of the first line
 */
void lcd_line1(void);

/**
 * \brief Sets the cursor to the beginning of the second line
 */
void lcd_line2(void);

/**
 * \brief Sets the cursor to a given position
 * 
 * \param row The row in which the cursor is placed. Must be 1 or 2. 
 * \param column The position within the row where the cursor is placed. Must
 * be between 1 and 16. 
 */
void lcd_goto(unsigned char row, unsigned char column);

/**
 * \brief Moves the cursor to a position relative to the current one
 * 
 * \param row Added to the current row. Must be between -1 and 1. If the
 * resulting row is outside of the screen, it is wrapped around (e.g. calling
 * this function with row=-1 when the cursor is in the first row will move it
 * to the row second one). 
 * \param column Added to the current position within the row. Must be between
 * -15 and +15. If the resulting position is outside of the screen, it is
 * wrapped around (e.g. calling this function with column=5 when the cursor is
 * in the 13th position will move it to the 2nd position in the same row). 
 */
void lcd_move(char row, char column);

/**
 * \brief Move the cursor to the preceeding position
 * 
 * This function uses wrapping: If the cursor is in the first position of a
 * row, it will be moved to the last position of the other row. 
 */
void lcd_back(void);

/**
 * \brief Moves the cursor to the first position of the current row
 */
void lcd_home(void);

/**
 * \brief Move the cursor to the following position
 * 
 * This function uses wrapping: If the cursor is in the last position of a row,
 * it will be moved to the first position of the other row. 
 */
void lcd_forward(void);

//-----------------------------------------------------------------------------
// Erasing

/**
 * \brief Clears the LCD
 * 
 * All characters are replaced by a space ( ) and the cursor is moved to the
 * first position of the first row. 
 */
//! Clear all data from display
void lcd_clear(void);

/**
 * \brief Erases one line of the display but does not change the current cursor
 * position
 * 
 * \param line The number of the line to be erased. Must be 1 or 2. 
 */
void lcd_erase(uint8_t line);

//-----------------------------------------------------------------------------
// Writing

/**
 * \brief Writes a single character
 * 
 * The character is written to the current position of the cursor and the
 * cursor is moved to the next position. At the end of the first line, it wraps
 * around to the second line. When the end of the second line is reached, it
 * wraps around to the first one and before the next time a character is
 * written, the LCD is cleared automatically. 
 * This goes for all writing functions. 
 * \param character The character to be written. There is rudimentary support
 * for UTF-8-encoded multi-byte characters. 
 */
void lcd_writeChar(char character);

/**
 * \brief Writes a string
 * 
 * See lcd_writeChar() for details. 
 * \param text The string to be written. 
 */
void lcd_writeString(const char *text);

/**
 * \brief Writes a string from program memory
 * 
 * See lcd_writeChar() for details. 
 * \param string The string to be written. 
 */
void lcd_writeProgString(const char *string);

/**
 * \brief Writes a string from program memory via stderr
 * 
 * Works the same as lcd_writeProgString() except it uses stderr. Hence if
 * stderr was not redirected, nothing happens. 
 * \param string The string to be written. 
 */
void lcd_writeErrorProgString(const char *string);

/**
 * \brief Writes a half byte (nibble) as a hexadecimal digit
 * 
 * \param number The half byte to be written. Must be between 0 and 15. 
 */
void lcd_writeHexNibble(uint8_t number);

/**
 * \brief Writes one byte as two hexadecimal digits
 * 
 * \param number The byte to be written. 
 */
void lcd_writeHexByte(uint8_t number);

/**
 * \brief Writes a two-byte unsigned integer using four hexadecimal digits
 * 
 * \param number The bytes to be written. 
 */
void lcd_writeHexWord(uint16_t number);

/**
 * \brief Writes a two-byte unsigned integer using up to four hexadecimal
 * digits
 * 
 * Same as lcd_writeHexWord() except leading zeros are omitted. The number zero
 * is written as '0'. 
 * \param number The bytes to be written. 
 */
void lcd_writeHex(uint16_t number);

/**
 * \brief Writes a two-byte unsigned integer using up to five decimal digits
 * 
 * \param number The integer to be written. 
 */
void lcd_writeDec(uint16_t number);

/**
 * \brief Writes a four-byte unsigned integer using eight hexadecimal digits
 * 
 * \param number The bytes to be written. 
 */
void lcd_write32bitHex(uint32_t number);

/**
 * \brief Writes a non-negative voltage value with three fractional digits
 * 
 * The output is the number voltage / valueUpperBound * voltUpperBound with
 * three fractional digits followed by the letter 'V'. This would typically be
 * used to display the result of an ADC voltage measurement. 
 * \param voltage A value representing the voltage on the scale
 * 0..(valueUpperBound-1). 
 * \param valueUpperBound Strict upper bound for voltage. 
 * \param voltUpperBound Reference voltage. value=0 represents 0 volts, whereas
 * value=valueUpperBound-1 represents voltUpperBound volts. 
 */
void lcd_writeVoltage(uint16_t voltage, uint16_t valueUpperBound, uint8_t voltUpperBound);

/**
 * \brief Draws a bar graph in line 1 and erases line 2
 * 
 * \param percent Percentage of the bar to be filled
 */
void lcd_drawBar(uint8_t percent);

//-----------------------------------------------------------------------------
// Custom characters

/**
 * \brief Macro for creating custom characters
 * 
 * The result of this can be passed to lcd_registerCustomChar(). 
 * A 5x8-pixel character bitmap is stored in an 8-byte unsigned integer. Each
 * row is one byte, with the top row corresponding to the lowest 8 bits. Inside
 * each byte, only the 5 least significant bits are used. Most characters leave
 * the last row empty since that is where a cursor could be placed. However,
 * this driver disables the cursor, so use it if you want. 
 */
#define CUSTOM_CHAR(cc0, cc1, cc2, cc3, cc4, cc5, cc6, cc7) \
	(0 | (((uint64_t)(cc0)) << 0 * 8) | (((uint64_t)(cc1)) << 1 * 8) | \
	(((uint64_t)(cc2)) << 2 * 8) | (((uint64_t)(cc3)) << 3 * 8) | \
	(((uint64_t)(cc4)) << 4 * 8) | (((uint64_t)(cc5)) << 5 * 8) | \
	(((uint64_t)(cc6)) << 6 * 8) | (((uint64_t)(cc7)) << 7 * 8))

/**
 * \brief Pre-define the tilde (~) and backslash (\) characters
 * 
 * These are the only printable ASCII characters the LCD does not already have.
 * They will automatically be registered during initialisation and the usual
 * UTF-8 characters for ~ and \ will be mapped to them. 
 * If you'd rather use the 8 custom character slots for something else and
 * don't tilde and/or backslash, remove them. 
 */
#define LCD_CC_TILDE 1
#define LCD_CC_TILDE_BITMAP (CUSTOM_CHAR( \
	0x00,                                 \
	0x08,                                 \
	0x15,                                 \
	0x02,                                 \
	0x00,                                 \
	0x00,                                 \
	0x00,                                 \
	0x00                                  \
))

#define LCD_CC_BACKSLASH 2
#define LCD_CC_BACKSLASH_BITMAP (CUSTOM_CHAR( \
	0x00,                                     \
	0x10,                                     \
	0x08,                                     \
	0x04,                                     \
	0x02,                                     \
	0x01,                                     \
	0x00,                                     \
	0x00                                      \
))

/**
 * \brief Registers a custom character
 * 
 * \param addr The address of the new character. Must be between 0 and 7. 
 * If another custom character occupies this space, it is replaced. 
 * In order to print that character, use the address as its "ASCII" code. 
 * If the character (or rather its address) is currently shown on the screen,
 * it changes in real time. You can use this to create crude animations. 
 * \param chr 5x8-pixel character bitmap. See CUSTOM_CHAR() for details. 
 */
void lcd_registerCustomChar(uint8_t addr, uint64_t chr);


//-----------------------------------------------------------------------------
// Miscellaneous

/**
 * \brief Pointer to FILE through which stdio functions can write to the LCD
 * 
 * You can use this with stdio functions even if you chose not to redirect
 * stdout or stderr to the LCD. 
 */
extern FILE* lcdout;

/**
 * \brief Directly send a command to the LCD. You shouldn't use this under
 * normal circumstances. 
 * 
 * \param command 8-bit command to be sent to the LCD
 */
void lcd_command(uint8_t command);

#endif


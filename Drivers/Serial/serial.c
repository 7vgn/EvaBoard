/**
 * \file serial.c
 * \brief See serial.h for details. 
 */

#include<avr/io.h>
#include"serial.h"

// Check if F_CPU is defined
#ifndef F_CPU
#error "F_CPU is not defined"
#endif

// Calculate UBBR value (see Table 17-1 of the datasheet)
#define SERIAL_UBBR ((uint16_t)((uint32_t)(F_CPU) / 8 / (uint32_t)(SERIAL_BAUDRATE) - 1))

// Warn if error is >0.5%
#if (F_CPU) * 1000 / 8 / ((F_CPU) / 8 / (SERIAL_BAUDRATE)) / (SERIAL_BAUDRATE) > 1005
#warning "Serial baud rate approximation has error >0.5%"
#endif

void serialInit()
{
	UBRR0 = SERIAL_UBBR;					// Set baud rate
	UCSR0A = (1 << U2X0);					// Run in 2X mode (divide by 8 instead of 16)
	UCSR0C = (0b00 << UMSEL00)				// Asynchronous operation
	       | (0b00 << UPM00)				// Disable parity checking
	       | (0 << USBS0)					// 1 stop bit
	       | (0b11 << UCSZ00);				// 8 data bits per character
	UCSR0B = (0 << RXCIE0)					// Disable RX complete interrupt
	       | (0 << TXCIE0)					// Disable TX complete interrupt
	       | (0 << UDRIE0)					// Disable data register empty interrupt
	       | (SERIAL_RECEIVE << RXEN0)		// Enable receiver
	       | (SERIAL_TRANSMIT << TXEN0)		// Enable transmitter
	       | (0 << UCSZ02);					// 8 data bits per character

	// Flush receive buffer
	do {UDR0;} while(UCSR0A & (1 << RXC0));

	// Redirect stdin
#if SERIAL_RECEIVE && SERIAL_REDIRECT_STDIN
	stdin = serialIn;
#endif
	// Redirect stdout
#if SERIAL_TRANSMIT && SERIAL_REDIRECT_STDOUT
	stdout = serialOut;
#endif
	// Redirect stderr
#if SERIAL_TRANSMIT && SERIAL_REDIRECT_STDERR
	stderr = serialOut;
#endif
}

#if SERIAL_TRANSMIT

void serialTransmit(char c)
{
	// Wait for UART to be ready
	while(!(UCSR0A & (1 << UDRE0)));

	// Clear TX complete flag
	UCSR0A |= (1 << TXC0);

	// Start transmission
	UDR0 = c;
}

void serialFlush()
{
	// Wait until both the transmit shift register and the transmit buffer
	// registers are empty
	while(!(UCSR0A & (1 << TXC0)));
}

/**
 * \brief Helper function for stdio
 */
static int serial_putchar(const char c, FILE* stream)
{
	serialTransmit(c);
	return 0;
}

static FILE out = FDEV_SETUP_STREAM(serial_putchar, NULL, _FDEV_SETUP_WRITE);
FILE* serialOut = &out;

#endif

#if SERIAL_RECEIVE

char serialReceive()
{
	// Wait for character to be received
	while(!(UCSR0A & (1 << RXC0)));

	// Read and return character
	return UDR0;
}

/**
 * \brief Helper function for stdio
 */
static int serial_getchar(const char c, FILE* stream)
{
	return serialReceive();
}

static FILE in = FDEV_SETUP_STREAM(serial_getchar, NULL, _FDEV_SETUP_READ);
FILE* serialIn = &in;

#endif


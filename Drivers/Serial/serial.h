/**
 * \file serial.h
 * \brief A primitive serial driver for the ATmega644(A)
 * 
 * This driver supports transmitting and receiving data via the ATmega's
 * Universal Asynchronous serial Receiver and Transmitter (UART). 
 * 
 * If you're using this driver to connect to a computer, enter the following
 * settings in your serial terminal program:
 * - The serial port you're connecting to. This depends on your operating
 *   system. On Linux it is typically /dev/ttyS? or /dev/ttyUSB? if you're
 *   using a USB-serial converter. On Windows, it is "COM?". In both cases,
 *   enter the port number in place of the question mark. 
 * - Baudrate: 250000 (unless you've changed the default value below)
 * - Data Bits: 8
 * - Parity: None
 * - Stop bits: 1
 * - Flow Control: None
 * 
 * Copy serial.h and serial.c into your project. Then use it like so:
 * 
 * #include"serial.h"
 * void main(void)
 * {
 *     serialInit();
 *     printf("Hello world!");
 *     while(1);
 * }
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#include<stdio.h>

//=============================================================================
// Configuration

/**
 * \brief Enable serial receiver
 *
 * If this is off (0), the serial receiver will remain disabled, the RX pin
 * (PD0) will not be used, and the serialReceive() function is not available.
 */
#define SERIAL_RECEIVE 1

/**
 * \brief Enable serial transmitter
 *
 * If this is off (0), the serial transmitter will remain disabled, the TX pin
 * (PD1) will not be used, and the serialTransmit() and serialFlush() functions
 * are not available. 
 */
#define SERIAL_TRANSMIT 1

/**
 * \brief Baud rate (bits per second)
 *
 * Depending on the ATmegas clock frequency, not all baud rates can be exactly
 * generated. The driver will issue a warning during compilation if the error
 * is too high. 
 */
#define SERIAL_BAUDRATE 250000

/**
 * \brief Redirect stdin, stdout, and/or stderr to serial
 * 
 * Has no effect if SERIAL_RECEIVE and/or SERIAL_TRANSMIT is not on
 */
#define SERIAL_REDIRECT_STDIN 1
#define SERIAL_REDIRECT_STDOUT 1
#define SERIAL_REDIRECT_STDERR 0

//=============================================================================
// Functions and variables

/**
 * \brief Initialises the UART module
 *
 * This function must be called before any other of the driver.
 */
void serialInit();

#if SERIAL_TRANSMIT

/**
 * \brief Transmits a character via UART
 * 
 * While this function does not wait until the character is transmitted, it
 * does block until the UART transmit buffer is free (i.e. until the previous
 * character has been transmitted). 
 * \param c The character to be transmitted
 */
void serialTransmit(char c);

/**
 * \brief Waits until the transmit buffer is empty, i.e. the last character
 * has been completely transmitted. This function can be used for example
 * before the UART module (or indeed the whole microcontroller) enters sleep
 * mode to prevent aborted transmissions. 
 */
void serialFlush();

/**
 * \brief Pointer to FILE through which stdio functions can write through
 * serial
 * 
 * You can use this with stdio functions even if you chose not to redirect
 * stdout or stderr. 
 */
extern FILE* serialOut;

#endif

#if SERIAL_RECEIVE
/**
 * \brief Receives a character via UART
 * 
 * This function is blocking, it returns only once a character has been
 * received. No buffering takes place. If more than one character is received
 * without this function being called in between, data can get lost. 
 * \return The received character
 */
char serialReceive();

/**
 * \brief Pointer to FILE through which stdio functions can read through serial
 * 
 * You can use this with stdio functions even if you chose not to redirect
 * stdin. 
 */
extern FILE* serialIn;

#endif

#endif // _SERIAL_H


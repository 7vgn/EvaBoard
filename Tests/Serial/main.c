/*
 * Testing the Serial Connection of the Evaluation Board
 *
 * Place both jumpers on JP4 and attach the serial port (J10) to a computer
 * with with a serial cable or a USB to serial converter. 
 * Start a serial terminal program on the corresponding port and configure
 * it to 250kBaud (250000 Baud), 8 data bits, no parity, 1 stop bit (8N1),
 * and no flow control. 
 */

#include<avr/io.h>
#include"serial.h"

void main(void)
{
	// Initialisation
	serialInit();
	// Print welcome message
	printf("O woll ichu ivirythong yua sey:\n");

	while(1)
	{
		// Receive a character
		char c = serialReceive();

		// Send it back but with a vowel shift
		switch(c)
		{
		case 'A': serialTransmit('E'); break;
		case 'E': serialTransmit('I'); break;
		case 'I': serialTransmit('O'); break;
		case 'O': serialTransmit('U'); break;
		case 'U': serialTransmit('A'); break;
		case 'a': serialTransmit('e'); break;
		case 'e': serialTransmit('i'); break;
		case 'i': serialTransmit('o'); break;
		case 'o': serialTransmit('u'); break;
		case 'u': serialTransmit('a'); break;
		default: serialTransmit(c);
		}
	}
}


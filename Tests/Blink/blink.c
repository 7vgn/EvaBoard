#include<avr/io.h>
#include<util/delay.h>

void main(void)
{
	DDRA = 1;
	while(1)
	{
		PORTA = 1;
		_delay_ms(500);
		PORTA = 0;
		_delay_ms(500);
	}
}


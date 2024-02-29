/*
 * Testing the Watch Crystal of the Evaluation Board and using it to measure
 * the CPU clock frequency
 *
 * Wiring: Place two jumpers from J7 (RTC) to the lower left two pins of J13
 * (Port C6 and C7) to connect the watch crystal Y2 to the TOSC[1:2] pins. 
 * Place another jumper on the two top pins of J11 (from Port A0 to LEDA1). 
 * Connect the LCD (J15) to Port B (J12) with an 8-pole cable (twisted). That
 * is, connect R/W to Port B6, EN to Port B5, RS to Port B4, DB7 to Port B3,
 * DB6 to Port B2, DB5 to Port B1, and DB4 to Port B0. Attach a 2x16 LCD to J16. 
 * 
 * Timer2 is configured to run off the watch crystal. With the right prescaler
 * settings, this results in a 1Hz overflow interrupt which is used to toggle
 * LEDA1 on Port A0. In addition to that, Timer2's PWM module is used to
 * generate a 2Hz signal (i.e. one rising and one falling edge every second)
 * on OC2B (Port D6). 
 *
 * Timer1 runs at the CPU's clock speed. Its input capture module records its
 * counter value whenever a falling edge occurs on ICP (which also happens to
 * be Port D6). Since Timer1's counter has only 16 bits - less than there are
 * CPU ticks per second, the number of overflows is also recorded. 
 * From the values of Timer1's counter on two successive captures and the
 * number of overflows in between, we can calculate the number of CPU clock
 * ticks per second, i.e. the CPU frequency, which is displayed on the LCD. 
 * 
 * You can also observe the temperature dependency of the two crystals by
 * carefully placing your finger on one of them. This might give you an idea of
 * the accuracy of the measurements. 
 */

#include<avr/io.h>
#include<avr/interrupt.h>
#include"lcd.h"

// Timer1 uses this flag to signal that a capture has taken place
volatile uint8_t capture = 0;

// Store the last two time points when Timer2 overflowed
volatile struct
{
	// Value of Timer1's counter at that time point
	uint16_t value;
	// Number of Timer1 overflows since then
	uint16_t overflows;
} captures[2];

// Overflow of Timer1's 16-bit counter occurs at <CPU clock> / 2^16
ISR(TIMER1_OVF_vect)
{
	captures[0].overflows++;
}

// Timer1 input capture (rising edge on ICP=OC2B) occurs once every second
ISR(TIMER1_CAPT_vect)
{
	// Shift out the oldest value
	captures[1] = captures[0];
	// Store the new capture point
	captures[0].value = ICR1;
	captures[0].overflows = 0;
	// Signal to the main program that we have a new capture
	capture = 1;
}

// Overflow of Timer2's 8-bit counter occurs once every second (256Hz / 2^8 = 1Hz)
ISR(TIMER2_OVF_vect)
{
	// Flip Port A0
	PORTA = ~PORTA & (1 << 0);
}

void main(void)
{
	// Configure Port A0 as output, D6 as output
	DDRA = 1 << 0;
	DDRD = 1 << 6;

	// Set up Timer 1 (see Section 14.11 of the datasheet), running at
	// CPU speed, for comparison
	TCCR1A = (0b00 << COM1A0)	// Disable PWM output on OC1A
	       | (0b00 << COM1B0)	// Disable PWM output on OC1B
	       | (0b00 << WGM10);	// Normal mode
	TCCR1B = (0b00 << WGM12)
	       | (0 << ICNC1)		// Disable input capture noise canceler
	       | (0 << ICES1)		// Input capture on falling edge of the ICF pin
	       | (0b001 << CS10);	// Prescaler 1:1
	TIMSK1 = (1 << ICIE1)		// Enable input capture interrupt
	       | (1 << TOIE1);		// Enable overflow interrupt

	// Set up Timer 2 (see Section 15.11 of the datasheet), running at
	// the speed of the watch crystal (32768Hz)
	TCCR2A = (0b00 << COM2A0)	// Disable PWM output on OC2A
	       | (0b10 << COM2B0)	// Enable PWM output on OC2B
	       | (0b11 << WGM20);	// Fast PWM mode
	TCCR2B = (0 << WGM22)
	       | (0b101 << CS20);	// Prescaler 1:128 (32768Hz / 128 = 256Hz)
	OCR2B = 1 << 7;
	ASSR   = (0 << EXCLK)		// Use crystal on TOSC1 and TOSC2
	       | (1 << AS2);		// Asynchronous mode
	TIMSK2 = (1 << TOIE2);		// Enable overflow interrupt

	// Enable interrupts globally
	sei();

	// The main loop recomputes and displays the frequency whenever the capture
	// flag gets set
	lcd_init();
	lcd_writeString("CPU Frequency:");
	while(1)
	{
		if(capture)
		{
			// Calculate the number of CPU clock cycles that have taken place
			// between the last two capture events
			uint32_t clocks = ((uint32_t)captures[1].overflows << 16)
			                + (uint32_t)captures[0].value
			                - (uint32_t)captures[1].value;

			// Reset capture flag
			capture = 0;

			// Display the frequency in line 2
			lcd_erase(2);
			lcd_line2();
			printf("%lu Hz", clocks);
		}
	}
}


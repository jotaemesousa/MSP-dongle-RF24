/*
 * main.cpp
 *
 *  Created on: May 6, 2013
 *      Author: joao
 */

#include "RF24.h"
#include <msp430.h>
#include "remote_defines.h"
#include "string.h"
#include "spi.h"
#include "timer_msp.h"
extern "C"
{
#include "conio/conio.h"
#include "serial/serial.h"


}

#define BLINK_RED_LED	P2OUT ^= BIT1;
#define BLINK_GREEN_LED	P2OUT ^= BIT0;
#define GREEN_LED_OFF	P2OUT &= ~BIT0;
#define RED_LED_OFF	P2OUT &= ~BIT1;


// Function prototypes
void setup_adc(void);
void adc_sample(unsigned int *ADC_ptr);
void setup_push_buttons(void);
void setup_leds(void);
void serial_receive(void);
uint8_t serial_parse(char *buffer);

int b = 0;
uint8_t led = 0;
// main loop
int main(void)
{
	WDTCTL = WDT_MDLY_32;                     // Set Watchdog Timer interval to ~30ms
	IE1 |= WDTIE;
	BCSCTL1 = CALBC1_8MHZ;            // Set DCO to 1MHz
	DCOCTL = CALDCO_8MHZ;

	unsigned long int last_millis = 0;
	default_timer();

	// Setup ADC
	//setup_adc();
	unsigned int ADC_values[2];

	// Setup push buttons
	//setup_push_buttons();

	// Setup LEDs
	setup_leds();
	GREEN_LED_OFF

	__bis_SR_register(GIE);       // Enter LPM0, interrupts enabled

	// ____________________________________________________________
	RC_remote ferrari;
	//RC_dongle car_param;
	ferrari.steer = 0;
	ferrari.linear = 0;
	ferrari.buttons = 0;

	cio_printf(":Init UART;\n");
	serial_init(57600);
	cio_printf(":Done;\n");


	RF24 radio = RF24();

	// Radio pipe addresses for the 2 nodes to communicate.
	const uint64_t pipes[3] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL};

	// Setup and configure rf radio
	radio.begin();

	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15,15);

	// optionally, reduce the payload size.  seems to
	// improve reliability
	radio.setPayloadSize(sizeof(RC_remote));

	radio.setDataRate(RF24_250KBPS);

	// Open pipes to other nodes for communication
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1,pipes[0]);

	// Start listening
	radio.startListening();

	// Dump the configuration of the rf unit for debugging
	radio.printDetails();

	for(;;)
	{
		serial_receive();
		// if there is data ready
		if ( radio.available() )
		{
			// Dump the payloads until we've gotten everything
			bool done = false;
			while (!done)
			{
				done = radio.read( &ferrari, sizeof(RC_remote) );


				if(done)
				{
					BLINK_GREEN_LED
					cio_printf(":L %i A %i B %x;\n", ferrari.linear, ferrari.steer, ferrari.buttons);

					if((ferrari.buttons & ASK_BIT) == ASK_BIT)
					{
						radio.stopListening();
						radio.write(&led, sizeof(uint8_t));
						radio.startListening();
					}
				}
			}
		}
	}
	return 0;
}

void serial_receive(void)
{
	char inChar;                        // temporary input char
	static char inData_[20];
	static int index_ = 0;
	static uint8_t receiving_cmd = 0;
	uint8_t n_char = 0;

	while((IFG2 & UCA0RXIFG) && n_char < 5)        //if bytes available at Serial port
	{
		n_char++;

		inChar = serial_recv();                // read from port

		if(index_ < 18)                // read up to 98 bytes
		{
			if(inChar == ':')
			{
				if(receiving_cmd == 0)
				{
					receiving_cmd = 1;

					inData_[index_] = inChar;        // store char
					++index_;                        // increment index
					inData_[index_] = 0;                // just to finish string
				}
			}
			else if(receiving_cmd == 1)
			{
				inData_[index_] = inChar;        // store char
				++index_;                        // increment index
				inData_[index_] = 0;                // just to finish string

			}
		}
		else                        // put end char ";"
		{
			index_ = 0;

		}

		if(receiving_cmd)
		{
			if(inChar == ';')
			{                        // if the last char is ";"

				if(!serial_parse(inData_))        //parse data
				{
					receiving_cmd = 0;
					index_ = 0;

					inData_[index_] = 0;
				}
				else
				{
					// no parse action
					receiving_cmd = 0;
					index_ = 0;

					inData_[index_] = 0;
				}
			}

		}
	}

}

uint8_t serial_parse(char *buffer)
{
	int d1;
	//char f1;last_received_byte_millis

	if(!strncmp(buffer,":led",4))                // motor cmd
	{
		if(buffer[6] == ';')
		{
			if(buffer[5] == '1')
			{
				led = 1;
			}
			else if(buffer[5] == '0')
			{
				led = 0;
			}
		}
		return 0;
	}
	return 1;
}

void setup_adc(void)
{
	// Scan P1.3 and P1.4
	ADC10CTL0 = ADC10SHT_2 + MSC + ADC10ON + ADC10IE;
	ADC10CTL1 = INCH_4 + CONSEQ_3;
	ADC10DTC1 = 0x02;
	ADC10AE0 |= 0x18;
}

void setup_push_buttons(void)
{

}

void setup_leds(void)
{
	P2DIR |= BIT0;
	P2DIR |= BIT1;
}

void adc_sample( unsigned int *ADC_ptr)
{
	ADC10CTL0 &= ~ENC;

	while (ADC10CTL1 & BUSY);               // Wait if ADC10 core is active

	ADC10SA = (unsigned int)ADC_ptr;     	// Data buffer start

	ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
	__bis_SR_register(GIE);        			// LPM0, ADC10_ISR will force exit
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	__bic_SR_register_on_exit(GIE);        // Clear CPUOFF bit from 0(SR)
}

// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
	static int c = 0;

	c++;
	b++;
	if(c > 264)
	{
		BLINK_RED_LED
		c = 0;
	}
}

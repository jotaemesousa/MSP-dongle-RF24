/*
 * main.cpp
 *
 *  Created on: May 6, 2013
 *      Author: joao
 */

#include "RF24.h"
#include <msp430.h>
#include "remote_defines.h"

extern "C"
{
#include "spi.h"
#include "conio/conio.h"
#include "serial/serial.h"
#include "timer_msp.h"
}

// Function prototypes
void setup_adc(void);
void adc_sample(unsigned int *ADC_ptr);
void setup_push_buttons(void);
void setup_leds(void);

int b = 0;

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
	ferrari.steer = 0;
	ferrari.linear = 0;
	ferrari.buttons = 0;

	serial_init(57600);
	cio_printf("Init UART\n");

	RF24 radio = RF24();

	// Radio pipe addresses for the 2 nodes to communicate.
	const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

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


	uint8_t temp = 0;
	for(;;)
	{
		// if there is data ready
		if ( radio.available() )
		{
			// Dump the payloads until we've gotten everything
			uint8_t len;
			bool done = false;
			while (!done)
			{
				done = radio.read( &ferrari, sizeof(RC_remote) );


				if(done)
				{
					BLINK_GREEN_LED
					cio_printf("linear %i angular %i buttons %x\n", ferrari.linear, ferrari.steer, ferrari.buttons);

					if((ferrari.buttons & ASK_BIT) == ASK_BIT)
					{

						if(temp == 0)temp = 1;
						else if(temp == 1)temp = 0;
						radio.stopListening();
						radio.write(&temp, sizeof(uint8_t));
						radio.startListening();
					}
				}
			}
		}

	}



	return 0;


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

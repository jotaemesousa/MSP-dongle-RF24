/*
 * remote_defines.h
 *
 *  Created on: May 9, 2013
 *      Author: joao
 */

#ifndef REMOTE_DEFINES_H_
#define REMOTE_DEFINES_H_

#define L1_BUTTON		0x01
#define L2_BUTTON		0x02
#define R1_BUTTON		0x04
#define R2_BUTTON		0x08
#define ASK_BIT			0x10

typedef struct ROSpberryRemote
{
	int16_t linear;
	int16_t steer;
	uint8_t buttons;

}RC_remote;


#endif /* REMOTE_DEFINES_H_ */

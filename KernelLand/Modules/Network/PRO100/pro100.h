/*
 * Acess2 PRO/100 Driver
 * - By John Hodge (thePowersGang)
 *
 * pro100.h
 * - Hardware header
 */
#ifndef _PRO100_H_
#define _PRO100_H_

#include "pro100_hw.h"

typedef struct sCard	tCard;

struct sCard
{
	Uint16	IOBase;
	struct sCSR	*MMIO;
	
	union {
		Uint8	Bytes[6];
		Uint16	Words[3];	// Used to read the MAC from EEPROM
	} MAC;

	//tIPStackBuffer	*TXBuffers[NUM_TX];
};

#endif


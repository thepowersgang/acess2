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

#include <semaphore.h>
#include <mutex.h>

#define NUM_RX	4
#define RX_BUF_SIZE	((PAGE_SIZE/2)-sizeof(tRXBuffer))
#define NUM_TX	(PAGE_SIZE/sizeof(tTXCommand))

typedef struct sCard	tCard;

struct sCard
{
	Uint16	IOBase;
	struct sCSR	*MMIO;
	
	union {
		Uint8	Bytes[6];
		Uint16	Words[3];	// Used to read the MAC from EEPROM
	} MAC;

	 int	CurRXIndex;
	tRXBuffer	*RXBufs[NUM_RX];
	tSemaphore	RXSemaphore;

	tSemaphore	TXCommandSem;
	tMutex	lTXCommands;
	 int	LastTXIndex;
	 int	CurTXIndex;
	tTXCommand	*TXCommands;
	tIPStackBuffer	*TXBuffers[NUM_TX];
};

#endif


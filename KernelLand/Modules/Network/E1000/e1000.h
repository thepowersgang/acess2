/*
 * Acess2 E1000 Network Driver
 * - By John Hodge (thePowersGang)
 *
 * e1000.h
 * - Driver core header
 */
#ifndef _E1000_H_
#define _E1000_H_

#include "e1000_hw.h"

#include <semaphore.h>
#include <mutex.h>
#include <IPStack/include/buffer.h>

#define NUM_TX_DESC	(PAGE_SIZE/sizeof(struct sTXDesc))
#define NUM_RX_DESC	(PAGE_SIZE/sizeof(struct sRXDesc))

#define RX_DESC_BSIZE	4096
#define RX_DESC_BSIZEHW	RCTL_BSIZE_4096

typedef struct sCard
{
	tPAddr	MMIOBasePhys;
	 int	IRQ;
	Uint16	IOBase;
	volatile void	*MMIOBase;

	tMutex	lRXDescs;
	 int	FirstUnseenRXD;
	 int	LastUnseenRXD;
	void	*RXBuffers[NUM_RX_DESC];
	volatile tRXDesc	*RXDescs;
	tSemaphore	AvailPackets;
	struct sCard	*RXBackHandles[NUM_RX_DESC];	// Pointers to this struct, offset used to select desc
	
	tMutex	lTXDescs;
	 int	FirstFreeTXD;
	 int	LastFreeTXD;
	volatile tTXDesc	*TXDescs;
	tSemaphore	FreeTxDescs;

	tIPStackBuffer	*TXSrcBuffers[NUM_TX_DESC];

	Uint8	MacAddr[6];

	void	*IPStackHandle;
} tCard;


#endif


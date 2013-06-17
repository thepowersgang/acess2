/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci.h
 * - Driver header
 */
#ifndef _AHCI__AHCI_H_
#define _AHCI__AHCI_H_

#include "ahci_hw.h"

typedef struct sAHCI_Ctrlr	tAHCI_Ctrlr;

struct sAHCI_Ctrlr
{
	 int	PortCount;	

	 int	IRQ;
	tPAddr	PMemBase;
	struct sAHCI_MemSpace	*MMIO;
};

#endif


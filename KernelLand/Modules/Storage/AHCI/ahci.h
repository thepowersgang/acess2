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
#include <Storage/LVM/include/lvm.h>
#include <stdbool.h>
#include <mutex.h>
#include <semaphore.h>
#include <events.h>

typedef struct sAHCI_Ctrlr	tAHCI_Ctrlr;
typedef struct sAHCI_Port	tAHCI_Port;

struct sAHCI_Ctrlr
{
	 int	IRQ;
	tPAddr	PMemBase;
	tAHCI_MemSpace	*MMIO;
	
	bool	Supports64Bit;

	 int	NCS;
	
	 int	PortCount;
	tAHCI_Port	*Ports;
};

struct sAHCI_Port
{
	 int	Idx;	// Hardware index
	tAHCI_Ctrlr	*Ctrlr;
	volatile struct s_port	*MMIO;

	tMutex	lCommandSlots;
	Uint32	IssuedCommands;
	volatile struct sAHCI_CmdHdr	*CmdList;
	struct sAHCI_CmdTable	*CommandTables[32];
	tThread	*CommandThreads[32];
	volatile struct sAHCI_RcvdFIS	*RcvdFIS;

	tSemaphore	InterruptSem;
	Uint32	LastIS;

	bool	bHotplug;
	bool	bPresent;
	bool	bATAPI;

	void	*LVMHandle;
};

#endif


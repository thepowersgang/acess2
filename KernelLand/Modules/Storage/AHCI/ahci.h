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
#include <semaphore.h>

typedef struct sAHCI_Ctrlr	tAHCI_Ctrlr;
typedef struct sAHCI_Port	tAHCI_Port;

struct sAHCI_Ctrlr
{
	 int	IRQ;
	tPAddr	PMemBase;
	tAHCI_MemSpace	*MMIO;

	 int	NCS;
	
	 int	PortCount;
	tAHCI_Port	*Ports;
};

struct sAHCI_Port
{
	 int	Idx;	// Hardware index
	volatile struct s_port	*MMIO;
	bool	bHotplug;
	bool	bPresent;

	tSemaphore	CommandListSem;
	volatile struct sAHCI_CmdHdr	*CmdList;	

	volatile struct sAHCI_RcvdFIS	*RcvdFIS;	

	void	*LVMHandle;
};

#endif


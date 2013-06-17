/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ahci.c
 * - Driver core
 */
#define DEBUG	1
#define VERSION	0x0001
#include <acess.h>
#include <modules.h>
#include <drv_pci.h>
#include "ahci.h"

// === CONSTANTS ===
#define MAX_CONTROLLERS	4

// === PROTOTYPES ===
 int	AHCI_Install(char **Arguments);
 int	AHCI_Cleanup(void);

// === GLOABLS ===
MODULE_DEFINE(0, VERSION, AHCI, AHCI_Install, AHCI_Cleanup, "LVM", NULL);
tAHCI_Ctrlr	gaAHCI_Controllers[MAX_CONTROLLERS];

// === CODE ====
int AHCI_Install(char **Arguments)
{

	LOG("offsetof(struct sAHCI_MemSpace, Ports) = %x", offsetof(struct sAHCI_MemSpace, Ports));
	ASSERT( offsetof(struct sAHCI_MemSpace, Ports) == 0x100 );

	// 0106XX = Standard, 010400 = RAID
	 int	i = 0;
	 int	id = -1;
	while( (id = PCI_GetDeviceByClass(0x010601, 0xFFFFFF, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		tAHCI_Ctrlr *ctrlr = &gaAHCI_Controllers[i];
		ctrlr->PMemBase = PCI_GetBAR(id, 5);
		// 
		if( !ctrlr->PMemBase )
		{
			// ctrlr->PMemBase = PCI_AllocateBAR(id, 5);
			// TODO: Allocate space
			continue ;
		}
		// - IO Address?
		if( (ctrlr->PMemBase & 0x1FFF) ) {
			Log_Warning("AHCI", "Controller %i [PCI %i] is invalid (BAR5=%P)",
				i, id, ctrlr->PMemBase);
			continue;
		}
		
		ctrlr->IRQ = PCI_GetIRQ(id);

		// Prepare MMIO (two pages according to the spec)
		ctrlr->MMIO = (void*)MM_MapHWPages(ctrlr->PMemBase, 2);

		LOG("%i [%i]: %P/IRQ%i mapped to %p",
			i, id, ctrlr->PMemBase, ctrlr->IRQ, ctrlr->MMIO);

		LOG(" CAP = %x, PI = %x, VS = %x",
			ctrlr->MMIO->CAP, ctrlr->MMIO->PI, ctrlr->MMIO->VS);
	
		i ++;
	}
	if( id >= 0 ) {
		Log_Notice("AHCI", "Only up to %i controllers are supported", MAX_CONTROLLERS);
	}
	
	return 0;
}

int AHCI_Cleanup(void)
{
	return 0;
}

int AHCI_InitSys(tAHCI_Ctrlr *Ctrlr)
{
	// 1. Set GHC.AE
	Ctrlr->MMIO->GHC |= AHCI_GHC_AE;
	// 2. Enumerate ports using PI
	// 3. (for each port) Ensure that port is not running
	//  - if PxCMD.(ST|CR|FRE|FR) all are clear, port is idle
	//  - Idle is set by clearing PxCMD.ST and waiting for .CR to clear (timeout 500ms)
	//  - AND .FRE = 0, checking .FR (timeout 500ms again)
	//  > On timeout, port/HBA reset
	// 4. Read CAP.NCS 
	// 5. Allocate PxCLB and PxFB for each port (setting PxCMD.FRE once done)
	// 6. Clear PxSERR with 1 to each implimented bit
	// > Clear PxIS then IS.IPS before setting PxIE/GHC.IE
	// 7. Set PxIE with desired interrupts and set GHC.IE
	
	// Detect present ports using:
	// > PxTFD.STS.BSY = 0, PxTFD.STS.DRQ = 0, and PxSSTS.DET = 3
	return 0;
}


/*
* Acess2 Kernel - AHCI Driver
* - By John Hodge (thePowersGang)
*
* ahci.c
*/
#include <drv_pci.h>
#include "ahci.h"

// === CODE ====
int AHCI_Install(char **Arguments)
{
	// 010601/FFFFFF
	while( (id = PCI_GetDeviceByClass(0x010000, 0xFF0000, id)) >= 0 && i < MAX_CONTROLLERS )
	{
		ctrlr->PMemBase = PCI_GetBAR(id, 5);
		if( !ctrlr->PMemBase )
		{
			// TODO: Allocate space
		}
		
		ctrlr->IRQNum = PCI_GetIRQ(id);
	}
	return 0;
}

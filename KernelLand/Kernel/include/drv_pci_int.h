/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv_pci_int.h
 * - PCI internal definitions
 */
#ifndef _DRV_PCI_INT_H
#define _DRV_PCI_INT_H

#include <acess.h>

extern Uint32	PCI_CfgReadDWord(Uint32 Addr);
extern void	PCI_CfgWriteDWord(Uint32 Addr, Uint32 data);

#endif


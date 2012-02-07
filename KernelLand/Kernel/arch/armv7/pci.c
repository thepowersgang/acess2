/*
 *
 */
#include <acess.h>
#include <drv_pci_int.h>

// Realview
//#define PCI_BASE	0x60000000

//#define PCI_BASE	0xF0400000	// VMM Mapping
#define PCI_BASE	0

// === CODE ===
void PCI_CfgWriteDWord(Uint32 Addr, Uint32 Data)
{
	#if PCI_BASE
	Uint32	address = PCI_BASE | Addr;
	*(Uint32*)(address) = Data;
	#else
	#endif
}

Uint32 PCI_CfgReadDWord(Uint32 Addr)
{
	#if PCI_BASE
	Uint32	address = PCI_BASE | Addr;
	return *(Uint32*)address;
	#else
	return 0xFFFFFFFF;
	#endif
}


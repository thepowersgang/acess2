/*
 * Acess2
 * arch/x86/pci.h - x86 PCI Bus Access
 */
#define DEBUG	0
#include <acess.h>
#include <drv_pci_int.h>

// === CODE ===
Uint32 PCI_CfgReadDWord(Uint32 Address)
{
	Address |= 0x80000000;
	outd(0xCF8, Address);
	return ind(0xCFC);
}

void PCI_CfgWriteDWord(Uint32 Address, Uint32 Data)
{
	Address |= 0x80000000;
	outd(0xCF8, Address);
	outd(0xCFC, Data);
}

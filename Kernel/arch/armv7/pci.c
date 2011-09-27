/*
 *
 */
#include <acess.h>

// Realview
//#define PCI_BASE	0x60000000

//#define PCI_BASE	0xF0400000	// VMM Mapping
#define PCI_BASE	0

// === PROTOTYPES ===
#if 1
void	PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data);
Uint32	PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
Uint16	PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
Uint8	PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
#endif

// === CODE ===
void PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data)
{
	#if PCI_BASE
	Uint32	address = PCI_BASE | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	*(Uint32*)(address) = data;
	#else
	#endif
}

Uint32 PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	#if PCI_BASE
	Uint32	address = PCI_BASE | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	return *(Uint32*)address;
	#else
	return 0xFFFFFFFF;
	#endif
}

Uint16 PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	return PCI_CfgReadDWord(bus, dev, func, offset & ~3) >> (8*(offset&2));
}

Uint8 PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	return PCI_CfgReadDWord(bus, dev, func, offset & ~3) >> (8*(offset&3));
}

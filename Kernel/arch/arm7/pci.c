/*
 *
 */
#include <acess.h>

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
	// TODO: Use PCI_ADDRESS constant
}

Uint32 PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	// TODO: Locate PCI base and read/write data
	return 0;
}

Uint16 PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	return PCI_CfgReadDWord(bus, dev, func, offset & ~3) >> (8*(offset&2));
}

Uint8 PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	return PCI_CfgReadDWord(bus, dev, func, offset & ~3) >> (8*(offset&3));
}

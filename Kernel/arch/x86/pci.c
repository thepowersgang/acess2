/*
 * Acess2
 * arch/x86/pci.h - x86 PCI Bus Access
 */
#define DEBUG	0
#include <acess.h>

// === PROTOTYPES ===
Uint32	PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
void	PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data);
Uint16	PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);
Uint8	PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset);

// === CODE ===
Uint32 PCI_CfgReadDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	address;
	Uint32	data;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 5 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	address = 0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	outd(0xCF8, address);
	
	data = ind(0xCFC);
	//Debug("PCI(0x%x) = 0x%08x", address, data);
	return data;
}
void PCI_CfgWriteDWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset, Uint32 data)
{
	Uint32	address;
	
	bus &= 0xFF;	// 8 Bits
	dev &= 0x1F;	// 5 Bits
	func &= 0x7;	// 3 Bits
	offset &= 0xFF;	// 8 Bits
	
	address = 0x80000000 | ((Uint)bus<<16) | ((Uint)dev<<11) | ((Uint)func<<8) | (offset&0xFC);
	outd(0xCF8, address);
	outd(0xCFC, data);
}
Uint16 PCI_CfgReadWord(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	data = PCI_CfgReadDWord(bus, dev, func, offset);
	
	data >>= (offset&2)*8;	// Allow Access to Upper Word
	
	return (Uint16)data;
}

Uint8 PCI_CfgReadByte(Uint16 bus, Uint16 dev, Uint16 func, Uint16 offset)
{
	Uint32	data = PCI_CfgReadDWord(bus, dev, func, offset);
	
	data >>= (offset&3)*8;	//Allow Access to Upper Word
	return (Uint8)data;
}


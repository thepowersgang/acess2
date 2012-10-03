/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * virtual_pci.h
 * - Virtual PCI bus definitions
 */
#ifndef _VIRTUAL_PCI_H_
#define _VIRTUAL_PCI_H_
#include <acess.h>

typedef struct sVPCI_Device	tVPCI_Device;

struct sVPCI_Device
{
	void	*Ptr;

	// Vendor/Device is defined at runtime as 0x0ACE:0x[ARCH][ARCH][IDX][IDX]
	Uint16	Vendor;
	Uint16	Device;
	// Class code (Class, Subclass, Programming Interface, Revision)
	Uint32	Class;
	
	Uint32	BARs[6];
	Uint8	IRQ;
	
	/**
	 * \param Ptr	Value in sVPCI_Device.Ptr
	 * \param Data	Data to write to the specified address
	 */
	void	(*Write)(void *Ptr, Uint8 DWord, Uint32 Data);
	Uint32	(*Read)(void *Ptr, Uint8 DWord);
};

extern int	giVPCI_DeviceCount;
extern tVPCI_Device	gaVPCI_Devices[];

extern Uint32	VPCI_Read(tVPCI_Device *Dev, Uint8 Offset, Uint8 Size);
extern void	VPCI_Write(tVPCI_Device *Dev, Uint8 Offset, Uint8 Size, Uint32 Data);
#endif


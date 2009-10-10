/*
 * Acess 2
 * PCI Bus Driver
 * drv_pci.h
 */
#ifndef _DRV_PCI_H
#define _DRV_PCI_H

enum e_PciClasses {
	PCI_CLASS_PRE20 = 0x00,
	PCI_CLASS_STORAGE,
	PCI_CLASS_NETWORK,
	PCI_CLASS_DISPLAY,
	PCI_CLASS_MULTIMEDIA,
	PCI_CLASS_MEMORY,
	PCI_CLASS_BRIDGE,
	PCI_CLASS_COMM,
	PCI_CLASS_PREPH,
	PCI_CLASS_INPUT,
	PCI_CLASS_DOCKING,
	PCI_CLASS_PROCESSORS,
	PCI_CLASS_SERIALBUS,
	PCI_CLASS_MISC = 0xFF
};
enum e_PciOverClasses {
	PCI_OC_PCIBRIDGE = 0x0604,
	PCI_OC_SCSI = 0x0100
};


extern int	PCI_CountDevices(Uint16 vendor, Uint16 device, Uint16 fcn);
extern int	PCI_GetDevice(Uint16 vendor, Uint16 device, Uint16 fcn, int idx);
extern int	PCI_GetDeviceByClass(Uint16 class, Uint16 mask, int prev);
extern Uint8	PCI_GetIRQ(int id);
extern Uint32	PCI_GetBAR0(int id);
extern Uint32	PCI_GetBAR1(int id);
extern Uint32	PCI_GetBAR3(int id);
extern Uint32	PCI_GetBAR4(int id);
extern Uint32	PCI_GetBAR5(int id);
extern Uint16	PCI_AssignPort(int id, int bar, int count);

#endif

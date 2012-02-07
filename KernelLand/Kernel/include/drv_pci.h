/**
 * \file drv_pci.h
 * \brief PCI Bus Driver
 * \author John Hodge (thePowersGang)
 */
#ifndef _DRV_PCI_H
#define _DRV_PCI_H

/**
 * \brief PCI Class Codes
 */
enum ePCIClasses
{
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

enum ePCIOverClasses
{
	PCI_OC_PCIBRIDGE = 0x060400,
	PCI_OC_SCSI = 0x010000
};

typedef int	tPCIDev;

/**
 * \brief Count PCI Devices
 * 
 * Counts the number of devices with specified Vendor and Device IDs
 */
extern int	PCI_CountDevices(Uint16 VendorID, Uint16 DeviceID);
extern tPCIDev	PCI_GetDevice(Uint16 VendorID, Uint16 DeviceID, int index);
/**
 * \param ClassCode (Class:SubClass:PI)
 */
extern tPCIDev	PCI_GetDeviceByClass(Uint32 ClassCode, Uint32 Mask, tPCIDev prev);

extern int	PCI_GetDeviceInfo(tPCIDev id, Uint16 *Vendor, Uint16 *Device, Uint32 *Class);
extern int	PCI_GetDeviceVersion(tPCIDev id, Uint8 *Revision);
extern int	PCI_GetDeviceSubsys(tPCIDev id, Uint16 *SubsystemVendor, Uint16 *SubsystemID);

extern Uint32	PCI_ConfigRead(tPCIDev id, int Offset, int Size);
extern void	PCI_ConfigWrite(tPCIDev id, int Offset, int Size, Uint32 Value);

extern Uint8	PCI_GetIRQ(tPCIDev id);
extern Uint32	PCI_GetBAR(tPCIDev id, int BAR);
//extern Uint16	PCI_AssignPort(tPCIDev id, int bar, int count);

#endif

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

typedef enum ePCI_BARTypes	tPCI_BARType;

enum ePCI_BARTypes
{
	PCI_BARTYPE_IO,
	PCI_BARTYPE_MEM,	// Any memory type
	PCI_BARTYPE_MEMP,	// Prefetchable memory
	PCI_BARTYPE_MEMNP,	// Non-prefetchable memory
	PCI_BARTYPE_MEM32,	// 32-bit memory
	PCI_BARTYPE_MEM64	// 64-bit memory
};

#define PCI_CMD_IOENABLE	(1 << 0)
#define PCI_CMD_MEMENABLE	(1 << 1)
#define PCI_CMD_BUSMASTER	(1 << 2)	// Device can behave as a bus master
#define PCI_CMD_SPECIALCYCLES	(1 << 3)	// Can monitor 'Special Cycle' operations
#define PCI_CMD_WRAINVAL	(1 << 4)	// Memory 'Write and Invalidate' can be generated
#define PCI_CMD_VGAPALSNOOP	(1 << 5)	// VGA Palette Snoop enabled
#define PCI_CMD_PARITYERRRESP	(1 << 6)	// Pairity Error Response (suppress PERR# generation)
#define PCI_CMD_SERRENABLE	(1 << 8)	// Enable SERR# Driver
#define PCI_CMD_FASTBACKBACK	(1 << 9)	// Fast Back-Back Enable
#define PCI_CMD_INTDISABLE	(1 <<10)	// Disable generation of INTx# signals

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

extern Uint16	PCI_SetCommand(tPCIDev id, Uint16 SBits, Uint16 CBits);
extern Uint8	PCI_GetIRQ(tPCIDev id);
extern Uint32	PCI_GetBAR(tPCIDev id, int BAR);
extern Uint64	PCI_GetValidBAR(tPCIDev id, int BAR, tPCI_BARType BARType);
//extern Uint16	PCI_AssignPort(tPCIDev id, int bar, int count);

#endif

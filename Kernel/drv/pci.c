/*
 * AcessOS/AcessBasic v0.1
 * PCI Bus Driver
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <drv_pci_int.h>

#define	LIST_DEVICES	1

// === STRUCTURES ===
typedef struct sPCIDevice
{
	Uint16	bus, slot, fcn;
	Uint16	vendor, device;
	Uint32	class;	// Class:Subclass:ProgIf
	Uint8	revision;
	Uint32	ConfigCache[256/4];
	char	Name[8];
	tVFS_Node	Node;
} tPCIDevice;

// === CONSTANTS ===
#define SPACE_STEP	5
#define MAX_RESERVED_PORT	0xD00

// === PROTOTYPES ===
 int	PCI_Install(char **Arguments);
 int	PCI_ScanBus(int ID, int bFill);
 
char	*PCI_int_ReadDirRoot(tVFS_Node *node, int pos);
tVFS_Node	*PCI_int_FindDirRoot(tVFS_Node *node, const char *filename);
Uint32	PCI_int_GetBusAddr(Uint16 Bus, Uint16 Slot, Uint16 Fcn, Uint8 Offset);
Uint64	PCI_int_ReadDevice(tVFS_Node *node, Uint64 pos, Uint64 length, void *buffer);
 int	PCI_int_EnumDevice(Uint16 bus, Uint16 dev, Uint16 fcn, tPCIDevice *info);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, PCI, PCI_Install, NULL, NULL);
 int	giPCI_BusCount = 1;
 int	giPCI_InodeHandle = -1;
 int	giPCI_DeviceCount = 0;
tPCIDevice	*gPCI_Devices = NULL;
tDevFS_Driver	gPCI_DriverStruct = {
	NULL, "pci",
	{
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.ReadDir = PCI_int_ReadDirRoot,
	.FindDir = PCI_int_FindDirRoot
	}
};
Uint32	*gaPCI_PortBitmap = NULL;
Uint32	gaPCI_BusBitmap[256/32];
 
// === CODE ===
/**
 * \brief Scan the PCI Bus for devices
 * \param Arguments	Boot-time parameters
 */
int PCI_Install(char **Arguments)
{
	 int	i;
	void	*tmpPtr;
	
	// Build Portmap
	gaPCI_PortBitmap = malloc( 1 << 13 );
	if( !gaPCI_PortBitmap ) {
		Log_Error("PCI", "Unable to allocate %i bytes for bitmap", 1 << 13);
		return MODULE_ERR_MALLOC;
	}
	memset( gaPCI_PortBitmap, 0, 1 << 13 );
	for( i = 0; i < MAX_RESERVED_PORT / 32; i ++ )
		gaPCI_PortBitmap[i] = -1;
	for( i = 0; i < MAX_RESERVED_PORT % 32; i ++ )
		gaPCI_PortBitmap[MAX_RESERVED_PORT / 32] = 1 << i;
	
	// Scan Bus (Bus 0, Don't fill gPCI_Devices)
	i = PCI_ScanBus(0, 0);
	if(i != MODULE_ERR_OK)	return i;
		
	if(giPCI_DeviceCount == 0) {
		Log_Notice("PCI", "No devices were found");
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Allocate device buffer
	tmpPtr = malloc(giPCI_DeviceCount * sizeof(tPCIDevice));
	if(tmpPtr == NULL) {
		Log_Warning("PCI", "Malloc ERROR");
		return MODULE_ERR_MALLOC;
	}
	gPCI_Devices = tmpPtr;
	
	Log_Log("PCI", "%i devices, filling structure", giPCI_DeviceCount);
	
	// Reset counts
	giPCI_DeviceCount = 0;
	giPCI_BusCount = 0;
	memset(gaPCI_BusBitmap, 0, sizeof(gaPCI_BusBitmap));
	// Rescan, filling the PCI device array
	PCI_ScanBus(0, 1);
	
	// Complete Driver Structure
	gPCI_DriverStruct.RootNode.Size = giPCI_DeviceCount;
	
	// And add to DevFS
	DevFS_AddDevice(&gPCI_DriverStruct);
	
	return MODULE_ERR_OK;
}

/**
 * \brief Scans a specific PCI Bus
 * \param BusID	PCI Bus ID to scan
 * \param bFill	Fill the \a gPCI_Devices array?
 */
int PCI_ScanBus(int BusID, int bFill)
{
	 int	dev, fcn;
	tPCIDevice	devInfo;
	
	if( gaPCI_BusBitmap[BusID/32] & (1 << (BusID%32)) )
		return MODULE_ERR_OK;
	
	gaPCI_BusBitmap[BusID/32] |= (1 << (BusID%32));
	
	for( dev = 0; dev < 32; dev++ )	// 32 Devices per bus
	{
		for( fcn = 0; fcn < 8; fcn++ )	// Max 8 functions per device
		{
			// Check if the device/function exists
			if(!PCI_int_EnumDevice(BusID, dev, fcn, &devInfo))
				continue;
			
			if(devInfo.class == PCI_OC_PCIBRIDGE)
			{
				#if LIST_DEVICES
				if( !bFill )
					Log_Log("PCI", "Bridge @ %i,%i:%i (0x%x:0x%x)",
						BusID, dev, fcn, devInfo.vendor, devInfo.device);
				#endif
				//TODO: Handle PCI-PCI Bridges
				//PCI_ScanBus(devInfo.???, bFill);
				giPCI_BusCount ++;
			}
			else
			{
				#if LIST_DEVICES
				if( !bFill )
					Log_Log("PCI", "Device %i,%i:%i %06x => 0x%04x:0x%04x",
						BusID, dev, fcn, devInfo.class, devInfo.vendor, devInfo.device);
				#endif
			}
			
			if( bFill ) {
				devInfo.Node.Inode = giPCI_DeviceCount;
				memcpy(&gPCI_Devices[giPCI_DeviceCount], &devInfo, sizeof(tPCIDevice));
			}
			giPCI_DeviceCount ++;
			
			// If bit 23 of (soemthing) is set, there are sub-functions
			if(fcn == 0 && !(devInfo.ConfigCache[3] & 0x00800000) )
				break;
		}
	}
	
	return MODULE_ERR_OK;
}

/**
 * \brief Read from Root of PCI Driver
*/
char *PCI_int_ReadDirRoot(tVFS_Node *Node, int Pos)
{
	ENTER("pNode iPos", Node, Pos);
	if(Pos < 0 || Pos >= giPCI_DeviceCount) {
		LEAVE('n');
		return NULL;
	}
	
	LEAVE('s', gPCI_Devices[Pos].Name);
	return strdup( gPCI_Devices[Pos].Name );
}
/**
 */
tVFS_Node *PCI_int_FindDirRoot(tVFS_Node *node, const char *filename)
{
	 int	bus,slot,fcn;
	 int	i;
	// Validate Filename (Pointer and length)
	if(!filename || strlen(filename) != 7)
		return NULL;
	// Check for spacers
	if(filename[2] != '.' || filename[5] != ':')
		return NULL;
	
	// Get Information
	if(filename[0] < '0' || filename[0] > '9')	return NULL;
	bus = (filename[0] - '0')*10;
	if(filename[1] < '0' || filename[1] > '9')	return NULL;
	bus += filename[1] - '0';
	if(filename[3] < '0' || filename[3] > '9')	return NULL;
	slot = (filename[3] - '0')*10;
	if(filename[4] < '0' || filename[4] > '9')	return NULL;
	slot += filename[4] - '0';
	if(filename[6] < '0' || filename[6] > '9')	return NULL;
	fcn = filename[6] - '0';
	
	// Find Match
	for(i=0;i<giPCI_DeviceCount;i++)
	{
		if(gPCI_Devices[i].bus != bus)		continue;
		if(gPCI_Devices[i].slot != slot)	continue;
		if(gPCI_Devices[i].fcn != fcn)	continue;
		
		return &gPCI_Devices[i].Node;
	}
	
	// Error Return
	return NULL;
}

/**
 */
Uint64 PCI_int_ReadDevice(tVFS_Node *node, Uint64 pos, Uint64 length, void *buffer)
{	
	if( pos + length > 256 )	return 0;
	
	memcpy(
		buffer,
		(char*)gPCI_Devices[node->Inode].ConfigCache + pos,
		length);
	
	return length;
}

// --- Kernel Code Interface ---
/**
 * \brief Counts the devices with the specified codes
 * \param vendor	Vendor ID
 * \param device	Device ID
 */
int PCI_CountDevices(Uint16 vendor, Uint16 device)
{
	int i, ret=0;
	for(i=0;i<giPCI_DeviceCount;i++)
	{
		if(gPCI_Devices[i].vendor != vendor)	continue;
		if(gPCI_Devices[i].device != device)	continue;
		ret ++;
	}
	return ret;
}

/**
 * \brief Gets the ID of the specified PCI device
 * \param vendor	Vendor ID
 * \param device	Device ID
 * \param idx	Number of matching entry wanted
 */
tPCIDev PCI_GetDevice(Uint16 vendor, Uint16 device, int idx)
{
	 int	i, j=0;
	for( i = 0; i < giPCI_DeviceCount; i ++ )
	{
		if(gPCI_Devices[i].vendor != vendor)	continue;
		if(gPCI_Devices[i].device != device)	continue;
		if(j == idx)	return i;
		j ++;
	}
	return -1;
}

/**
 * \brief Gets the ID of a device by it's class code
 * \param class	Class Code
 * \param mask	Mask for class comparison
 * \param prev	ID of previous device (-1 for no previous)
 */
tPCIDev PCI_GetDeviceByClass(Uint32 class, Uint32 mask, tPCIDev prev)
{
	 int	i;
	// Check if prev is negative (meaning get first)
	if(prev < 0)	i = 0;
	else	i = prev+1;
	
	for( ; i < giPCI_DeviceCount; i++ )
	{
		if((gPCI_Devices[i].class & mask) == class)
			return i;
	}
	return -1;
}

int PCI_GetDeviceInfo(tPCIDev ID, Uint16 *Vendor, Uint16 *Device, Uint32 *Class)
{
	tPCIDevice	*dev = &gPCI_Devices[ID];
	if(ID < 0 || ID >= giPCI_DeviceCount)	return 1;
	
	if(Vendor)	*Vendor = dev->vendor;
	if(Device)	*Device = dev->device;
	if(Class)	*Class = dev->class;
	return 0;
}

int PCI_GetDeviceVersion(tPCIDev ID, Uint8 *Revision)
{
	tPCIDevice	*dev = &gPCI_Devices[ID];
	if(ID < 0 || ID >= giPCI_DeviceCount)	return 1;
	
	if(Revision)	*Revision = dev->revision;
	return 0;
}

int PCI_GetDeviceSubsys(tPCIDev ID, Uint16 *SubsystemVendor, Uint16 *SubsystemID)
{
	tPCIDevice	*dev = &gPCI_Devices[ID];
	if(ID < 0 || ID >= giPCI_DeviceCount)	return 1;
	
	if(SubsystemVendor)	*SubsystemVendor = dev->ConfigCache[0x2c/4] & 0xFFFF;
	if(SubsystemID)	*SubsystemID = dev->ConfigCache[0x2c/4] >> 16;

	return 0;
}

Uint32 PCI_int_GetBusAddr(Uint16 Bus, Uint16 Slot, Uint16 Fcn, Uint8 Offset)
{
	Bus &= 0xFF;
	Slot &= 0x1F;
	Fcn &= 7;
	Offset &= 0xFC;
	return ((Uint32)Bus << 16) | (Slot << 11) | (Fcn << 8) | (Offset & 0xFC);
}

Uint32 PCI_ConfigRead(tPCIDev ID, int Offset, int Size)
{
	tPCIDevice	*dev;
	Uint32	dword, addr;
	
	if( ID < 0 || ID >= giPCI_DeviceCount )	return 0;
	if( Offset < 0 || Offset > 256 )	return 0;

	// TODO: Should I support non-aligned reads?
	if( Offset & (Size - 1) )	return 0;

	dev = &gPCI_Devices[ID];
	addr = PCI_int_GetBusAddr(dev->bus, dev->slot, dev->fcn, Offset);

	dword = PCI_CfgReadDWord(addr);
	gPCI_Devices[ID].ConfigCache[Offset/4] = dword;
	switch( Size )
	{
	case 1:	return (dword >> (8 * (Offset&3))) & 0xFF;
	case 2:	return (dword >> (8 * (Offset&2))) & 0xFFFF;
	case 4:	return dword;
	default:
		return 0;
	}
}

void PCI_ConfigWrite(tPCIDev ID, int Offset, int Size, Uint32 Value)
{
	tPCIDevice	*dev;
	Uint32	dword, addr;
	 int	shift;
	if( ID < 0 || ID >= giPCI_DeviceCount )	return ;
	if( Offset < 0 || Offset > 256 )	return ;
	
	dev = &gPCI_Devices[ID];
	addr = PCI_int_GetBusAddr(dev->bus, dev->slot, dev->fcn, Offset);

	if(Size != 4)
		dword = PCI_CfgReadDWord(addr);
	switch(Size)
	{
	case 1:
		shift = (Offset&3)*8;
		dword &= ~(0xFF << shift);
		dword |= Value << shift;
	 	break;
	case 2:
		shift = (Offset&2)*8;
		dword &= ~(0xFFFF << shift);
		dword |= Value << shift;
		break;
	case 4:
		dword = Value;
		break;
	default:
		return;
	}
	PCI_CfgWriteDWord(addr, dword);
}

/**
 * \brief Get the IRQ assigned to a device
 */
Uint8 PCI_GetIRQ(tPCIDev id)
{
	if(id < 0 || id >= giPCI_DeviceCount)
		return 0;
	return gPCI_Devices[id].ConfigCache[15] & 0xFF;
	//return PCI_CfgReadByte( gPCI_Devices[id].bus, gPCI_Devices[id].slot, gPCI_Devices[id].fcn, 0x3C);
}

/**
 * \brief Read the a BAR (base address register) from the PCI config space
 */
Uint32 PCI_GetBAR(tPCIDev id, int BARNum)
{
	if(id < 0 || id >= giPCI_DeviceCount)
		return 0;
	if(BARNum < 0 || BARNum >= 6)
		return 0;
	return gPCI_Devices[id].ConfigCache[4+BARNum];
}

/**
 * \brief Get device information for a slot/function
 */
int PCI_int_EnumDevice(Uint16 bus, Uint16 slot, Uint16 fcn, tPCIDevice *info)
{
	Uint32	vendor_dev, tmp;
	 int	i;
	Uint32	addr;
	addr = PCI_int_GetBusAddr(bus, slot, fcn, 0);	

	vendor_dev = PCI_CfgReadDWord( addr );
	if((vendor_dev & 0xFFFF) == 0xFFFF)	// Invalid Device
		return 0;

	info->ConfigCache[0] = vendor_dev;
	for( i = 1, addr += 4; i < 256/4; i ++, addr += 4 )
	{
		info->ConfigCache[i] = PCI_CfgReadDWord(addr);
	}	

	info->bus = bus;
	info->slot = slot;
	info->fcn = fcn;
	info->vendor = vendor_dev & 0xFFFF;
	info->device = vendor_dev >> 16;
	tmp = info->ConfigCache[2];
	info->revision = tmp & 0xFF;
	info->class = tmp >> 8;
	
//	#if LIST_DEVICES
//	Log("BAR0 0x%08x BAR1 0x%08x BAR2 0x%08x", info->ConfigCache[4], info->ConfigCache[5], info->ConfigCache[6]);
//	Log("BAR3 0x%08x BAR4 0x%08x BAR5 0x%08x", info->ConfigCache[7], info->ConfigCache[8], info->ConfigCache[9]);
//	Log("Class: 0x%06x", info->class);
//	#endif
	
	// Make node name
	info->Name[0] = '0' + bus/10;
	info->Name[1] = '0' + bus%10;
	info->Name[2] = '.';
	info->Name[3] = '0' + slot/10;
	info->Name[4] = '0' + slot%10;
	info->Name[5] = ':';
	info->Name[6] = '0' + fcn;
	info->Name[7] = '\0';
	
	// Create VFS Node
	memset( &info->Node, 0, sizeof(tVFS_Node) );
	info->Node.Size = 256;
	
	info->Node.NumACLs = 1;
	info->Node.ACLs = &gVFS_ACL_EveryoneRO;
	
	info->Node.Read = PCI_int_ReadDevice;
	
	return 1;
}

// === EXPORTS ===
//*
EXPORT(PCI_CountDevices);
EXPORT(PCI_GetDevice);
EXPORT(PCI_GetDeviceByClass);
EXPORT(PCI_GetDeviceInfo);
EXPORT(PCI_GetDeviceVersion);
EXPORT(PCI_GetDeviceSubsys);
//EXPORT(PCI_AssignPort);
EXPORT(PCI_GetBAR);
EXPORT(PCI_GetIRQ);
//*/

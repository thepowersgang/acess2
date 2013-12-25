/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * drv/pci.c
 * - PCI Enumeration and Arbitration
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <drv_pci_int.h>
#include <virtual_pci.h>

#define USE_PORT_BITMAP	0
#define	LIST_DEVICES	1
#define PCI_MAX_BUSSES	8

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
 
 int	PCI_int_ReadDirRoot(tVFS_Node *node, int pos, char Dest[FILENAME_MAX]);
tVFS_Node	*PCI_int_FindDirRoot(tVFS_Node *node, const char *filename, Uint Flags);
Uint32	PCI_int_GetBusAddr(Uint16 Bus, Uint16 Slot, Uint16 Fcn, Uint8 Offset);
size_t	PCI_int_ReadDevice(tVFS_Node *node, off_t Offset, size_t Length, void *buffer, Uint Flags);
 int	PCI_int_EnumDevice(Uint16 bus, Uint16 dev, Uint16 fcn, tPCIDevice *info);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, PCI, PCI_Install, NULL, NULL);
 int	giPCI_BusCount = 1;
Uint8	gaPCI_BusNumbers[PCI_MAX_BUSSES];
 int	giPCI_InodeHandle = -1;
 int	giPCI_DeviceCount = 0;
tPCIDevice	*gPCI_Devices = NULL;
tVFS_NodeType	gPCI_RootNodeType = {
	.TypeName = "PCI Root Node",
	.ReadDir = PCI_int_ReadDirRoot,
	.FindDir = PCI_int_FindDirRoot
};
tVFS_NodeType	gPCI_DevNodeType = {
	.TypeName = "PCI Dev Node",
	.Read = PCI_int_ReadDevice
};
tDevFS_Driver	gPCI_DriverStruct = {
	NULL, "pci",
	{
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Type = &gPCI_RootNodeType
	}
};
#if USE_PORT_BITMAP
Uint32	*gaPCI_PortBitmap = NULL;
#endif
Uint32	gaPCI_BusBitmap[256/32];
 
// === CODE ===
/**
 * \brief Scan the PCI Bus for devices
 * \param Arguments	Boot-time parameters
 */
int PCI_Install(char **Arguments)
{
	 int	ret, bus;
	void	*tmpPtr;
	
	#if USE_PORT_BITMAP
	// Build Portmap
	gaPCI_PortBitmap = malloc( 1 << 13 );
	if( !gaPCI_PortBitmap ) {
		Log_Error("PCI", "Unable to allocate %i bytes for bitmap", 1 << 13);
		return MODULE_ERR_MALLOC;
	}
	memset( gaPCI_PortBitmap, 0, 1 << 13 );
	 int	i;
	for( i = 0; i < MAX_RESERVED_PORT / 32; i ++ )
		gaPCI_PortBitmap[i] = -1;
	for( i = 0; i < MAX_RESERVED_PORT % 32; i ++ )
		gaPCI_PortBitmap[MAX_RESERVED_PORT / 32] = 1 << i;
	#endif	

	// Scan Bus (Bus 0, Don't fill gPCI_Devices)
	giPCI_DeviceCount = 0;
	giPCI_BusCount = 1;
	gaPCI_BusNumbers[0] = 0;
	for( bus = 0; bus < giPCI_BusCount; bus ++ )
	{
		ret = PCI_ScanBus(gaPCI_BusNumbers[bus], 0);
		if(ret != MODULE_ERR_OK)	return ret;
	}
	// TODO: PCIe
	// - Add VPCI Devices
	giPCI_DeviceCount += giVPCI_DeviceCount;
	
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
	giPCI_BusCount = 1;
	memset(gaPCI_BusBitmap, 0, sizeof(gaPCI_BusBitmap));
	// Rescan, filling the PCI device array
	for( bus = 0; bus < giPCI_BusCount; bus ++ )
	{
		PCI_ScanBus(gaPCI_BusNumbers[bus], 1);
	}
	// Insert VPCI Devices
	for( int i = 0; i < giVPCI_DeviceCount; i ++ )
	{
		tPCIDevice	*devinfo = &gPCI_Devices[giPCI_DeviceCount];
		
		devinfo->bus = -1;
		devinfo->slot = i;
		devinfo->fcn = 0;
		devinfo->vendor = gaVPCI_Devices[i].Vendor;
		devinfo->device = gaVPCI_Devices[i].Device;
		devinfo->revision = gaVPCI_Devices[i].Class & 0xFF;
		devinfo->class = gaVPCI_Devices[i].Class >> 8;
		snprintf(devinfo->Name, sizeof(devinfo->Name), "%02x.%02x:%x", 0xFF, i, 0);
		
		#if LIST_DEVICES
		Log_Log("PCI", "Device %i,%i:%i %06x => 0x%04x:0x%04x Rev %i",
			0xFF, i, 0, devinfo->class,
			devinfo->vendor, devinfo->device, devinfo->revision);
		#endif

		for(int j = 0; j < 256/4; j ++ )
			devinfo->ConfigCache[j] = VPCI_Read(&gaVPCI_Devices[i], j*4, 4);

		memset(&devinfo->Node, 0, sizeof(devinfo->Node));
		devinfo->Node.Inode = giPCI_DeviceCount;
		devinfo->Node.Size = 256;
		devinfo->Node.NumACLs = 1;
		devinfo->Node.ACLs = &gVFS_ACL_EveryoneRO;
		devinfo->Node.Type = &gPCI_DevNodeType;

		giPCI_DeviceCount ++;
	}
	
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
			
			#if LIST_DEVICES
			if( !bFill )
				Log_Log("PCI", "Device %i,%i:%i %06x => 0x%04x:0x%04x Rev %i",
					BusID, dev, fcn, devInfo.class,
					devInfo.vendor, devInfo.device, devInfo.revision);
			#endif
			
			if( bFill ) {
				devInfo.Node.Inode = giPCI_DeviceCount;
				memcpy(&gPCI_Devices[giPCI_DeviceCount], &devInfo, sizeof(tPCIDevice));
			}
			giPCI_DeviceCount ++;
			
			switch( (devInfo.ConfigCache[3] >> 16) & 0x7F )
			{
			case 0x00:	// Normal device
				break;
			case 0x01:	// PCI-PCI Bridge
				{
				// TODO: Add to list of busses?
				Uint8	sec = (devInfo.ConfigCache[6] & 0x0000FF00) >> 8;
				#if LIST_DEVICES
				if( !bFill ) {
					Uint8	pri = (devInfo.ConfigCache[6] & 0x000000FF) >> 0;
					Log_Log("PCI", "- PCI-PCI Bridge %02x=>%02x", pri, sec);
				}
				#endif
				gaPCI_BusNumbers[giPCI_BusCount++] = sec;
				}
				break;
			case 0x02:	// PCI-CardBus Bridge
				break;
			}

			// If bit 8 of the Header Type register is set, there are sub-functions
			if(fcn == 0 && !(devInfo.ConfigCache[3] & 0x00800000) )
				break;
		}
	}
	
	return MODULE_ERR_OK;
}

/**
 * \brief Read from Root of PCI Driver
*/
int PCI_int_ReadDirRoot(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	ENTER("pNode iPos", Node, Pos);
	if(Pos < 0 || Pos >= giPCI_DeviceCount) {
		LEAVE_RET('i', -EINVAL);
	}
	
	LOG("Name = %s", gPCI_Devices[Pos].Name);
	strncpy(Dest, gPCI_Devices[Pos].Name, FILENAME_MAX);
	LEAVE_RET('i', 0);
}
/**
 */
tVFS_Node *PCI_int_FindDirRoot(tVFS_Node *node, const char *filename, Uint Flags)
{
	// Find Match
	for( int i = 0; i < giPCI_DeviceCount; i ++ )
	{
		int cmp = strcmp(gPCI_Devices[i].Name, filename);
		if( cmp > 0 )	// Sorted list
			break;
		if( cmp == 0 )
			return &gPCI_Devices[i].Node;
	}
	
	// Error Return
	return NULL;
}

/**
 * \brief Read the PCI configuration space of a device
 */
size_t PCI_int_ReadDevice(tVFS_Node *node, off_t pos, size_t length, void *buffer, Uint Flags)
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
	// Detect VPCI devices
	if( dev->bus == -1 ) {
		return VPCI_Read(&gaVPCI_Devices[dev->slot], Offset, Size);
	}

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

	// Detect VPCI devices
	if( dev->bus == -1 ) {
		VPCI_Write(&gaVPCI_Devices[dev->slot], Offset, Size, Value);
		return ;
	}

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

Uint16 PCI_SetCommand(tPCIDev id, Uint16 SBits, Uint16 CBits)
{
	tPCIDevice	*dev = &gPCI_Devices[id];
	dev->ConfigCache[1] &= ~CBits;
	dev->ConfigCache[1] |= SBits;
	Uint32 addr = PCI_int_GetBusAddr(dev->bus, dev->slot, dev->fcn, 1*4);
	PCI_CfgWriteDWord(addr, dev->ConfigCache[1]);
	dev->ConfigCache[1] = PCI_CfgReadDWord(addr);
	return dev->ConfigCache[1];
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
 */
Uint64 PCI_GetValidBAR(tPCIDev ID, int BARNum, tPCI_BARType Type)
{
	if( ID < 0 || ID >= giPCI_DeviceCount )
		return 0;
	if( BARNum < 0 || BARNum >= 6 )
		return 0;
	tPCIDevice	*dev = &gPCI_Devices[ID];
	Uint32	bar_val = dev->ConfigCache[4+BARNum];
	Uint64	ret = -1;
	switch( Type )
	{
	case PCI_BARTYPE_IO:
		if( !(bar_val & 1) )
			return 0;
		ret = bar_val & ~3;
		break;

	case PCI_BARTYPE_MEMNP:
		if( bar_val & 8 )	return 0;
		if(0)	
	case PCI_BARTYPE_MEMP:
		if( !(bar_val & 8) )	return 0;
	case PCI_BARTYPE_MEM:
		if( bar_val & 1 )	return 0;
		if( (bar_val & 6) == 4 ) {
			ASSERTCR(BARNum, <, 5, 0);
			ret = (bar_val & ~0xF) | ((Uint64)dev->ConfigCache[4+BARNum+1] << 32);
		}
		else {
			ret = bar_val & ~0xF;
		}
		break;
	case PCI_BARTYPE_MEM32:
		if( bar_val & 1 )	return 0;
		if( (bar_val & 6) != 0 )	return 0;
		ret = bar_val & ~0xF;
		break;
	case PCI_BARTYPE_MEM64:
		if( bar_val & 1 )	return 0;
		if( (bar_val & 6) != 4 )	return 0;
		ASSERTCR(BARNum, <, 5, 0);
		ret = (bar_val & ~0xF) | ((Uint64)dev->ConfigCache[4+BARNum+1] << 32);
		break;
	}
	ASSERT(ret != -1);
	if( ret == 0 ) {
		Log_Error("PCI", "PCI%i BAR%i correct type, but unallocated (0x%x)",
			ID, BARNum, bar_val);
		return 0;
	}
	return ret;
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
	
	info->bus = bus;
	info->slot = slot;
	info->fcn = fcn;

	// Read configuration
	info->ConfigCache[0] = vendor_dev;
	for( i = 1, addr += 4; i < 256/4; i ++, addr += 4 )
	{
		info->ConfigCache[i] = PCI_CfgReadDWord(addr);
	}	

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
	snprintf(info->Name, 8, "%02x.%02x:%x", bus, slot, fcn);
	
	// Create VFS Node
	memset( &info->Node, 0, sizeof(tVFS_Node) );
	info->Node.Size = 256;
	
	info->Node.NumACLs = 1;
	info->Node.ACLs = &gVFS_ACL_EveryoneRO;
	
	info->Node.Type = &gPCI_DevNodeType;
	
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

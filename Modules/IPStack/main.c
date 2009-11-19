/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 */
#define DEBUG	0
#define VERSION	((0<<8)|10)
#include "ipstack.h"
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <tpl_drv_network.h>

// === IMPORTS ===
 int	ARP_Initialise();

// === PROTOTYPES ===
 int	IPStack_Install(char **Arguments);
char	*IPStack_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_FindDir(tVFS_Node *Node, char *Name);
 int	IPStack_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	IPStack_AddInterface(char *Device);
tAdapter	*IPStack_GetAdapter(char *Path);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, IPStack, IPStack_Install, NULL, NULL);
tDevFS_Driver	gIP_DriverInfo = {
	NULL, "ip",
	{
	.Size = 0,	// Number of interfaces
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = IPStack_ReadDir,
	.FindDir = IPStack_FindDir
	}
};
 int	glIP_Interfaces = 0;
tInterface	*gIP_Interfaces = NULL;
tInterface	*gIP_Interfaces_Last = NULL;
 int	glIP_Adapters = 0;
tAdapter	*gIP_Adapters = NULL;

// === CODE ===
/**
 * \fn int IPStack_Install(char **Arguments)
 * \brief Intialise the relevant parts of the stack and register with DevFS
 */
int IPStack_Install(char **Arguments)
{
	 int	i = 0;
	
	// Install Handlers
	ARP_Initialise();
	
	if(Arguments)
	{
		// Parse module arguments
		for( i = 0; Arguments[i]; i++ )
		{
			//if(strcmp(Arguments[i], "Device") == '=') {
			//	
			//}
		}
	}
	
	return 1;
}

/**
 * \brief Read from the IP Stack's Device Directory
 */
char *IPStack_ReadDir(tVFS_Node *Node, int Pos)
{
	tInterface	*iface;
	char	name[5] = "ip0\0\0";
	
	// Create the name
	if(Pos < 10)
		name[2] = '0' + Pos;
	else {
		name[2] = '0' + Pos/10;
		name[3] = '0' + Pos%10;
	}
	
	// Traverse the list
	for( iface = gIP_Interfaces; iface && Pos--; iface = iface->Next ) ;
	
	// Did we run off the end?
	if(!iface)	return NULL;
	
	// Return the pre-generated name
	return strdup(name);
}

/**
 * \brief Get the node of an interface
 */
tVFS_Node *IPStack_FindDir(tVFS_Node *Node, char *Name)
{
	 int	i;
	 tInterface	*iface;
	
	if(Name[0] != 'i' || Name[1] != 'p')	return NULL;
	if(Name[2] < '0' || Name[2] > '9')	return NULL;
	
	if(Name[3] == '\0') {
		i = Name[2] - '0';
		for( iface = gIP_Interfaces; iface && i--; iface = iface->Next ) ;
		if(!iface)	return NULL;
		return &iface->Node;
	}
	
	if(Name[3] < '0' || Name[3] > '9')	return NULL;
	
	i = (Name[2] - '0')*10;
	i += Name[3] - '0';
	
	for( iface = gIP_Interfaces; iface && i--; iface = iface->Next ) ;
	if(!iface)	return NULL;
	return &iface->Node;
}

static const char *casIOCtls_Root[] = { DRV_IOCTLNAMES, "add_interface", NULL };
static const char *casIOCtls_Iface[] = {
	DRV_IOCTLNAMES,
	"getset_type",
	"get_address", "set_address",
	"getset_subnet",
	"get_gateway", "set_gateway",
	NULL
	};
/**
 * \brief Handles IOCtls for the IPStack root/interfaces
 */
int IPStack_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	case DRV_IOCTL_TYPE:
		return DRV_TYPE_MISC;
	
	case DRV_IOCTL_IDENT:
		if( !CheckMem( Data, 4 ) )	return -1;
		memcpy(Data, "IP\0\0", 4);
		return 1;
	
	case DRV_IOCTL_VERSION:
		return VERSION;
	
	case DRV_IOCTL_LOOKUP:
		if( !CheckString( Data ) )	return -1;
		
		if( Node == &gIP_DriverInfo.RootNode )
			return LookupString( (char**)casIOCtls_Root, (char*)Data );
		else
			return LookupString( (char**)casIOCtls_Iface, (char*)Data );
	}
	
	if(Node == &gIP_DriverInfo.RootNode)
	{
		switch(ID)
		{
		/*
		 * add_interface
		 * - Adds a new IP interface and binds it to a device
		 */
		case 4:
			if( Threads_GetUID() != 0 )	return -1;
			if( !CheckString( Data ) )	return -1;
			return IPStack_AddInterface(Data);
		}
		return 0;	
	}
	else
	{
		tInterface	*iface = (tInterface*)Node->ImplPtr;
		switch(ID)
		{
		/*
		 * getset_type
		 * - Get/Set the interface type
		 */
		case 4:
			// Get Type?
			if( !Data )	return iface->Type;
			// Ok, it's set type
			if( Threads_GetUID() != 0 )	return -1;
			if( !CheckMem( Data, sizeof(int) ) )	return -1;
			switch( *(int*)Data )
			{
			case 0:	// Disable
				iface->Type = 0;
				memset(&iface->IP6, 0, sizeof(tIPv6));	// Clear address
				break;
			case 4:	// IPv4
				iface->Type = 4;
				memset(&iface->IP4, 0, sizeof(tIPv4));
				break;
			case 6:	// IPv6
				iface->Type = 6;
				memset(&iface->IP6, 0, sizeof(tIPv6));
				break;
			default:
				return -1;
			}
			return 0;
		
		/*
		 * get_address
		 * - Get the interface's address
		 */
		case 5:
			switch(iface->Type)
			{
			case 0:	return 1;
			case 4:
				if( !CheckMem( Data, sizeof(tIPv4) ) )	return -1;
				memcpy( Data, &iface->IP4, sizeof(tIPv4) );
				return 1;
			case 6:
				if( !CheckMem( Data, sizeof(tIPv6) ) )	return -1;
				memcpy( Data, &iface->IP6, sizeof(tIPv6) );
				return 1;
			}
			return 0;
		
		/*
		 * set_address
		 * - Get the interface's address
		 */
		case 6:
			if( Threads_GetUID() != 0 )	return -1;
			switch(iface->Type)
			{
			case 0:	return 1;
			case 4:
				if( !CheckMem( Data, sizeof(tIPv4) ) )	return -1;
				iface->Type = 0;	// One very hacky mutex/trash protector
				memcpy( &iface->IP4, Data, sizeof(tIPv4) );
				iface->Type = 4;
				return 1;
			case 6:
				if( !CheckMem( Data, sizeof(tIPv6) ) )	return -1;
				iface->Type = 0;
				memcpy( &iface->IP6, Data, sizeof(tIPv6) );
				iface->Type = 6;
				return 1;
			}
			return 0;
		
		/*
		 * getset_subnet
		 * - Get/Set the bits in the address subnet
		 */
		case 7:
			// Get?
			if( Data == NULL )
			{
				switch( iface->Type )
				{
				case 4:		return iface->IP4.SubnetBits;
				case 6:		return iface->IP6.SubnetBits;
				default:	return 0;
				}
			}
			
			// Ok, set.
			if( Threads_GetUID() != 0 )	return -1;
			if( !CheckMem(Data, sizeof(int)) )	return -1;
			
			// Check and set the subnet bits
			switch( iface->Type )
			{
			case 4:
				if( *(int*)Data < 0 || *(int*)Data > 31 )	return -1;
				iface->IP4.SubnetBits = *(int*)Data;
				return iface->IP4.SubnetBits;
			case 6:
				if( *(int*)Data < 0 || *(int*)Data > 127 )	return -1;
				iface->IP6.SubnetBits = *(int*)Data;
				return iface->IP6.SubnetBits;
			default:
				break;
			}
			
			return 0;
		}
	}
	
	return 0;
}

// --- Internal ---
/**
 * \fn int IPStack_AddInterface(char *Device)
 * \brief Adds an interface to the list
 */
int IPStack_AddInterface(char *Device)
{
	tInterface	*iface;
	
	iface = malloc(sizeof(tInterface));
	if(!iface)	return -2;	// Return ERR_MYBAD
	
	iface->Next = NULL;
	iface->Type = 0;	// Unset type
	
	// Create Node
	iface->Node.Flags = VFS_FFLAG_DIRECTORY;
	iface->Node.Size = 0;
	iface->Node.NumACLs = 1;
	iface->Node.ACLs = &gVFS_ACL_EveryoneRX;
	iface->Node.ReadDir = NULL;
	iface->Node.FindDir = NULL;
	
	// Get adapter handle
	iface->Adapter = IPStack_GetAdapter(Device);
	if( !iface->Adapter ) {
		free( iface );
		return -1;	// Return ERR_YOUFAIL
	}
	
	// Append to list
	LOCK( &glIP_Interfaces );
	if( gIP_Interfaces ) {
		gIP_Interfaces_Last->Next = iface;
		gIP_Interfaces_Last = iface;
	}
	else {
		gIP_Interfaces = iface;
		gIP_Interfaces_Last = iface;
	}
	RELEASE( &glIP_Interfaces );
	
	// Success!
	return 1;
}

/**
 * \fn tAdapter *IPStack_GetAdapter(char *Path)
 * \brief Gets/opens an adapter given the path
 */
tAdapter *IPStack_GetAdapter(char *Path)
{
	tAdapter	*dev;
	
	LOCK( &glIP_Adapters );
	
	// Check if this adapter is already open
	for( dev = gIP_Adapters; dev; dev = dev->Next )
	{
		if( strcmp(dev->Device, Path) == 0 ) {
			dev->NRef ++;
			RELEASE( &glIP_Adapters );
			return dev;
		}
	}
	
	// Ok, so let's open it
	dev = malloc( sizeof(tAdapter) + strlen(Path) + 1 );
	if(!dev) {
		RELEASE( &glIP_Adapters );
		return NULL;
	}
	
	// Fill Structure
	strcpy( dev->Device, Path );
	dev->NRef = 1;
	
	// Open Device
	dev->DeviceFD = VFS_Open( dev->Device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE );
	if( dev->DeviceFD == -1 ) {
		free( dev );
		RELEASE( &glIP_Adapters );
		return NULL;
	}
	
	// Check that it is a network interface
	if( VFS_IOCtl(dev->DeviceFD, 0, NULL) != DRV_TYPE_NETWORK ) {
		Warning("IPStack_GetAdapter: '%s' is not a network interface", dev->Device);
		VFS_Close( dev->DeviceFD );
		free( dev );
		RELEASE( &glIP_Adapters );
		return NULL;
	}
	
	// Get MAC Address
	VFS_IOCtl(dev->DeviceFD, NET_IOCTL_GETMAC, &dev->MacAddr);
	
	// Add to list
	dev->Next = gIP_Adapters;
	gIP_Adapters = dev;
	
	RELEASE( &glIP_Adapters );
	return dev;
}

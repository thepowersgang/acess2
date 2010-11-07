/*
 * Acess2 IP Stack
 * - Interface Control
 */
#define DEBUG	0
#define VERSION	VER2(0,10)
#include "ipstack.h"
#include "link.h"
#include <tpl_drv_common.h>
#include <tpl_drv_network.h>

// === CONSTANTS ===
//! Default timeout value, 30 seconds
#define DEFAULT_TIMEOUT	(30*1000)

// === IMPORTS ===
extern int	IPv4_Ping(tInterface *Iface, tIPv4 Addr);
//extern int	IPv6_Ping(tInterface *Iface, tIPv6 Addr);
extern tVFS_Node	gIP_RouteNode;

// === PROTOTYPES ===
char	*IPStack_Root_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_Root_IOCtl(tVFS_Node *Node, int ID, void *Data);

 int	IPStack_AddInterface(const char *Device, const char *Name);
 int	IPStack_AddFile(tSocketFile *File);
tAdapter	*IPStack_GetAdapter(const char *Path);

char	*IPStack_Iface_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_Iface_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_Iface_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
//! Loopback (127.0.0.0/8, ::1) Pseudo-Interface
tInterface	gIP_LoopInterface = {
	Node: {
		ImplPtr: &gIP_LoopInterface,
		Flags: VFS_FFLAG_DIRECTORY,
		Size: -1,
		NumACLs: 1,
		ACLs: &gVFS_ACL_EveryoneRX,
		ReadDir: IPStack_Iface_ReadDir,
		FindDir: IPStack_Iface_FindDir,
		IOCtl: IPStack_Iface_IOCtl
	},
	Adapter: NULL,
	Type: 0
};
tShortSpinlock	glIP_Interfaces;
tInterface	*gIP_Interfaces = NULL;
tInterface	*gIP_Interfaces_Last = NULL;

tSocketFile	*gIP_FileTemplates;
tMutex	glIP_Adapters;
tAdapter	*gIP_Adapters = NULL;
 int	giIP_NextIfaceId = 1;

// === CODE ===

/**
 * \brief Read from the IP Stack's Device Directory
 */
char *IPStack_Root_ReadDir(tVFS_Node *Node, int Pos)
{
	tInterface	*iface;
	char	*name;
	ENTER("pNode iPos", Node, Pos);
	

	// Routing Subdir
	if( Pos == 0 ) {
		return strdup("routes");
	}
	// Pseudo Interfaces
	if( Pos == 1 ) {
		return strdup("lo");
	}
	Pos -= 2;
	
	// Traverse the list
	for( iface = gIP_Interfaces; iface && Pos--; iface = iface->Next ) ;
	
	// Did we run off the end?
	if(!iface) {
		LEAVE('n');
		return NULL;
	}
	
	name = malloc(4);
	if(!name) {
		Log_Warning("IPStack", "IPStack_Root_ReadDir - malloc error");
		LEAVE('n');
		return NULL;
	}
	
	// Create the name
	Pos = iface->Node.ImplInt;
	if(Pos < 10) {
		name[0] = '0' + Pos;
		name[1] = '\0';
	}
	else if(Pos < 100) {
		name[0] = '0' + Pos/10;
		name[1] = '0' + Pos%10;
		name[2] = '\0';
	}
	else {
		name[0] = '0' + Pos/100;
		name[1] = '0' + (Pos/10)%10;
		name[2] = '0' + Pos%10;
		name[3] = '\0';
	}
	
	LEAVE('s', name);
	// Return the pre-generated name
	return name;
}

/**
 * \brief Get the node of an interface
 */
tVFS_Node *IPStack_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	i, num;
	tInterface	*iface;
	
	ENTER("pNode sName", Node, Name);
	
	// Routing subdir
	if( strcmp(Name, "routes") == 0 ) {
		return &gIP_RouteNode;
	}
	
	// Loopback
	if( strcmp(Name, "lo") == 0 ) {
		return &gIP_LoopInterface.Node;
	}
	
	i = 0;	num = 0;
	while('0' <= Name[i] && Name[i] <= '9')
	{
		num *= 10;
		num += Name[i] - '0';
		i ++;
	}
	if(Name[i] != '\0') {
		LEAVE('n');
		return NULL;
	}
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next )
	{
		if( (int)iface->Node.ImplInt == num )
		{
			LEAVE('p', &iface->Node);
			return &iface->Node;
		}
	}
	LEAVE('p', NULL);
	return NULL;
}

static const char *casIOCtls_Root[] = { DRV_IOCTLNAMES, "add_interface", NULL };
/**
 * \brief Handles IOCtls for the IPStack root
 */
int IPStack_Root_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	case DRV_IOCTL_TYPE:
		LEAVE('i', DRV_TYPE_MISC);
		return DRV_TYPE_MISC;
	
	case DRV_IOCTL_IDENT:
		tmp = ModUtil_SetIdent(Data, "IPStack");
		LEAVE('i', 1);
		return 1;
	
	case DRV_IOCTL_VERSION:
		LEAVE('x', VERSION);
		return VERSION;
	
	case DRV_IOCTL_LOOKUP:
		tmp = ModUtil_LookupString( (char**)casIOCtls_Root, (char*)Data );
		LEAVE('i', tmp);
		return tmp;
		
		/*
		 * add_interface
		 * - Adds a new IP interface and binds it to a device
		 */
	case 4:
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		if( !CheckString( Data ) )	LEAVE_RET('i', -1);
		{
			char	name[4] = "";
			tmp = IPStack_AddInterface(Data, name);
		}
		LEAVE_RET('i', tmp);
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn int IPStack_AddInterface(char *Device)
 * \brief Adds an interface to the list
 */
int IPStack_AddInterface(const char *Device, const char *Name)
{
	tInterface	*iface;
	tAdapter	*card;
	
	ENTER("sDevice", Device);
	
	card = IPStack_GetAdapter(Device);
	
	iface = malloc(sizeof(tInterface) + strlen(Name));
	if(!iface) {
		LEAVE('i', -2);
		return -2;	// Return ERR_MYBAD
	}
	
	iface->Next = NULL;
	iface->Type = 0;	// Unset type
	
	// Create Node
	iface->Node.ImplPtr = iface;
	iface->Node.Flags = VFS_FFLAG_DIRECTORY;
	iface->Node.Size = -1;
	iface->Node.NumACLs = 1;
	iface->Node.ACLs = &gVFS_ACL_EveryoneRX;
	iface->Node.ReadDir = IPStack_Iface_ReadDir;
	iface->Node.FindDir = IPStack_Iface_FindDir;
	iface->Node.IOCtl = IPStack_Iface_IOCtl;
	iface->Node.MkNod = NULL;
	iface->Node.Link = NULL;
	iface->Node.Relink = NULL;
	iface->Node.Close = NULL;
	
	// Set Defaults
	iface->TimeoutDelay = DEFAULT_TIMEOUT;
	
	// Get adapter handle
	iface->Adapter = IPStack_GetAdapter(Device);
	if( !iface->Adapter ) {
		free( iface );
		LEAVE('i', -1);
		return -1;	// Return ERR_YOUFAIL
	}
	
	// Delay setting ImplInt until after the adapter is opened
	// Keeps things simple
	iface->Node.ImplInt = giIP_NextIfaceId++;
	
	// Append to list
	SHORTLOCK( &glIP_Interfaces );
	if( gIP_Interfaces ) {
		gIP_Interfaces_Last->Next = iface;
		gIP_Interfaces_Last = iface;
	}
	else {
		gIP_Interfaces = iface;
		gIP_Interfaces_Last = iface;
	}
	SHORTREL( &glIP_Interfaces );

//	gIP_DriverInfo.RootNode.Size ++;
	
	// Success!
	LEAVE('i', iface->Node.ImplInt);
	return iface->Node.ImplInt;
}

/**
 * \brief Adds a file to the socket list
 */
int IPStack_AddFile(tSocketFile *File)
{
	Log_Log("IPStack", "Added file '%s'", File->Name);
	File->Next = gIP_FileTemplates;
	gIP_FileTemplates = File;
	return 0;
}

// ---
// VFS Functions
// ---
/**
 * \brief Read from an interface's directory
 */
char *IPStack_Iface_ReadDir(tVFS_Node *Node, int Pos)
{
	tSocketFile	*file = gIP_FileTemplates;
	while(Pos-- && file) {
		file = file->Next;
	}
	
	if(!file)	return NULL;
	
	return strdup(file->Name);
}

/**
 * \brief Gets a named node from an interface directory
 */
tVFS_Node *IPStack_Iface_FindDir(tVFS_Node *Node, const char *Name)
{
	tSocketFile	*file = gIP_FileTemplates;
	
	// Get file definition
	for(;file;file = file->Next)
	{
		if( strcmp(file->Name, Name) == 0 )	break;
	}
	if(!file)	return NULL;
	
	// Pass the buck!
	return file->Init(Node->ImplPtr);
}

/**
 * \brief Names for interface IOCtl Calls
 */
static const char *casIOCtls_Iface[] = {
	DRV_IOCTLNAMES,
	"getset_type",
	"get_address", "set_address",
	"getset_subnet",
	"get_gateway", "set_gateway",
	"get_device",
	"ping",
	NULL
	};
/**
 * \brief Handles IOCtls for the IPStack interfaces
 */
int IPStack_Iface_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp;
	tInterface	*iface = (tInterface*)Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	case DRV_IOCTL_TYPE:
		LEAVE('i', DRV_TYPE_MISC);
		return DRV_TYPE_MISC;
	
	case DRV_IOCTL_IDENT:
		tmp = ModUtil_SetIdent(Data, STR(IDENT));
		LEAVE('i', 1);
		return 1;
	
	case DRV_IOCTL_VERSION:
		LEAVE('x', VERSION);
		return VERSION;
	
	case DRV_IOCTL_LOOKUP:
		tmp = ModUtil_LookupString( (char**)casIOCtls_Iface, (char*)Data );
		LEAVE('i', tmp);
		return tmp;
	
	/*
	 * getset_type
	 * - Get/Set the interface type
	 */
	case 4:
		// Set Type?
		if( Data )
		{
			// Ok, it's set type
			if( Threads_GetUID() != 0 ) {
				LOG("Attempt by non-root to alter an interface (%i)", Threads_GetUID());
				LEAVE('i', -1);
				return -1;
			}
			if( !CheckMem( Data, sizeof(int) ) ) {
				LOG("Invalid pointer %p", Data);
				LEAVE('i', -1);
				return -1;
			}
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
				LEAVE('i', -1);
				return -1;
			}
		}
		LEAVE('i', iface->Type);
		return iface->Type;
	
	/*
	 * get_address
	 * - Get the interface's address
	 */
	case 5:
		switch(iface->Type)
		{
		case 0:	LEAVE_RET('i', 1);
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			memcpy( Data, &iface->IP4.Address, sizeof(tIPv4) );
			LEAVE_RET('i', 1);
		case 6:
			if( !CheckMem( Data, sizeof(tIPv6) ) )	LEAVE_RET('i', -1);
			memcpy( Data, &iface->IP6.Address, sizeof(tIPv6) );
			LEAVE_RET('i', 1);
		}
		LEAVE_RET('i', 0);
	
	/*
	 * set_address
	 * - Get the interface's address
	 */
	case 6:
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		switch(iface->Type)
		{
		case 0:	LEAVE_RET('i', 1);
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			iface->Type = 0;	// One very hacky mutex/trash protector
			memcpy( &iface->IP4.Address, Data, sizeof(tIPv4) );
			iface->Type = 4;
			LEAVE_RET('i', 1);
		case 6:
			if( !CheckMem( Data, sizeof(tIPv6) ) )	LEAVE_RET('i', -1);
			iface->Type = 0;
			memcpy( &iface->IP6.Address, Data, sizeof(tIPv6) );
			iface->Type = 6;
			LEAVE_RET('i', 1);
		}
		LEAVE_RET('i', 0);
	
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
			case 4:		LEAVE_RET('i', iface->IP4.SubnetBits);
			case 6:		LEAVE_RET('i', iface->IP6.SubnetBits);
			default:	LEAVE_RET('i', 0);
			}
		}
		
		// Ok, set.
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		if( !CheckMem(Data, sizeof(int)) )	LEAVE_RET('i', -1);
		
		// Check and set the subnet bits
		switch( iface->Type )
		{
		case 4:
			if( *(int*)Data < 0 || *(int*)Data > 31 )	LEAVE_RET('i', -1);
			iface->IP4.SubnetBits = *(int*)Data;
			LEAVE_RET('i', iface->IP4.SubnetBits);
		case 6:
			if( *(int*)Data < 0 || *(int*)Data > 127 )	LEAVE_RET('i', -1);
			iface->IP6.SubnetBits = *(int*)Data;
			LEAVE_RET('i', iface->IP6.SubnetBits);
		default:
			break;
		}
		
		LEAVE('i', 0);
		return 0;
		
	/*
	 * get_gateway
	 * - Get the interface's IPv4 gateway
	 */
	case 8:
		switch(iface->Type)
		{
		case 0:
			LEAVE_RET('i', 1);
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			memcpy( Data, &iface->IP4.Gateway, sizeof(tIPv4) );
			LEAVE_RET('i', 1);
		case 6:
			LEAVE_RET('i', 1);
		}
		LEAVE('i', 0);
		return 0;
	
	/*
	 * set_gateway
	 * - Get/Set the interface's IPv4 gateway
	 */
	case 9:
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		switch(iface->Type)
		{
		case 0:
			LEAVE_RET('i', 1);
		
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			iface->Type = 0;	// One very hacky mutex/trash protector
			memcpy( &iface->IP4.Gateway, Data, sizeof(tIPv4) );
			iface->Type = 4;
			LEAVE_RET('i', 1);
			
		case 6:
			LEAVE_RET('i', 1);
		}
		break;
	
	/*
	 * get_device
	 * - Gets the name of the attached device
	 */
	case 10:
		if( Data == NULL )
			LEAVE_RET('i', iface->Adapter->DeviceLen);
		if( !CheckMem( Data, iface->Adapter->DeviceLen+1 ) )
			LEAVE_RET('i', -1);
		strcpy( Data, iface->Adapter->Device );
		return iface->Adapter->DeviceLen;
	
	/*
	 * ping
	 * - Send an ICMP Echo
	 */
	case 11:
		switch(iface->Type)
		{
		case 0:
			LEAVE_RET('i', 1);
		
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			tmp = IPv4_Ping(iface, *(tIPv4*)Data);
			LEAVE('i', tmp);
			return tmp;
			
		case 6:
			LEAVE_RET('i', 1);
		}
		break;
	
	}
	
	LEAVE('i', 0);
	return 0;
}

// --- Internal ---
/**
 * \fn tAdapter *IPStack_GetAdapter(const char *Path)
 * \brief Gets/opens an adapter given the path
 */
tAdapter *IPStack_GetAdapter(const char *Path)
{
	tAdapter	*dev;
	 int	tmp;
	
	ENTER("sPath", Path);
	
	Mutex_Acquire( &glIP_Adapters );
	
	// Check if this adapter is already open
	for( dev = gIP_Adapters; dev; dev = dev->Next )
	{
		if( strcmp(dev->Device, Path) == 0 ) {
			dev->NRef ++;
			Mutex_Release( &glIP_Adapters );
			LEAVE('p', dev);
			return dev;
		}
	}
	
	// Ok, so let's open it
	dev = malloc( sizeof(tAdapter) + strlen(Path) + 1 );
	if(!dev) {
		Mutex_Release( &glIP_Adapters );
		LEAVE('n');
		return NULL;
	}
	
	// Fill Structure
	strcpy( dev->Device, Path );
	dev->NRef = 1;
	dev->DeviceLen = strlen(Path);
	
	// Open Device
	dev->DeviceFD = VFS_Open( dev->Device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE );
	if( dev->DeviceFD == -1 ) {
		free( dev );
		Mutex_Release( &glIP_Adapters );
		LEAVE('n');
		return NULL;
	}
	
	// Check that it is a network interface
	tmp = VFS_IOCtl(dev->DeviceFD, 0, NULL);
	LOG("Device type = %i", tmp);
	if( tmp != DRV_TYPE_NETWORK ) {
		Warning("IPStack_GetAdapter: '%s' is not a network interface", dev->Device);
		VFS_Close( dev->DeviceFD );
		free( dev );
		Mutex_Release( &glIP_Adapters );
		LEAVE('n');
		return NULL;
	}
	
	// Get MAC Address
	VFS_IOCtl(dev->DeviceFD, NET_IOCTL_GETMAC, &dev->MacAddr);
	
	// Add to list
	dev->Next = gIP_Adapters;
	gIP_Adapters = dev;
	
	Mutex_Release( &glIP_Adapters );
	
	// Start watcher
	Link_WatchDevice( dev );
	
	LEAVE('p', dev);
	return dev;
}

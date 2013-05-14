/*
 * Acess2 IP Stack
 * - Interface Control
 */
#define DEBUG	0
#define VERSION	VER2(0,10)
#include "ipstack.h"
#include "link.h"
#include <api_drv_common.h>
#include "include/adapters.h"
#include "interface.h"

// === CONSTANTS ===
//! Default timeout value, 5 seconds
#define DEFAULT_TIMEOUT	(5*1000)

// === IMPORTS ===
extern int	IPv4_Ping(tInterface *Iface, tIPv4 Addr);
//extern int	IPv6_Ping(tInterface *Iface, tIPv6 Addr);
extern tVFS_Node	gIP_RouteNode;
extern tVFS_Node	gIP_AdaptersNode;

// === PROTOTYPES ===
 int	IPStack_Root_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_Root_IOCtl(tVFS_Node *Node, int ID, void *Data);

 int	IPStack_AddFile(tSocketFile *File);
tInterface	*IPStack_AddInterface(const char *Device, int Type, const char *Name);

 int	IPStack_Iface_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*IPStack_Iface_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_Iface_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
tVFS_NodeType	gIP_RootNodeType = {
	.ReadDir = IPStack_Root_ReadDir,
	.FindDir = IPStack_Root_FindDir,
	.IOCtl = IPStack_Root_IOCtl
};
tVFS_NodeType	gIP_InterfaceNodeType = {
		.ReadDir = IPStack_Iface_ReadDir,
		.FindDir = IPStack_Iface_FindDir,
		.IOCtl = IPStack_Iface_IOCtl
};
//! Loopback (127.0.0.0/8, ::1) Pseudo-Interface
tInterface	gIP_LoopInterface = {
	.Node = {
		.ImplPtr = &gIP_LoopInterface,
		.Flags = VFS_FFLAG_DIRECTORY,
		.Size = -1,
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.Type = &gIP_InterfaceNodeType
	},
	.Adapter = NULL,
	.Type = 0
};
tShortSpinlock	glIP_Interfaces;
tInterface	*gIP_Interfaces = NULL;
tInterface	*gIP_Interfaces_Last = NULL;
 int	giIP_NextIfaceId = 1;

tSocketFile	*gIP_FileTemplates;

// === CODE ===

/**
 * \brief Read from the IP Stack's Device Directory
 */
int IPStack_Root_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	tInterface	*iface;
	ENTER("pNode iPos", Node, Pos);
	

	// Routing Subdir
	if( Pos == 0 ) {
		strcpy(Dest, "routes");
		return 0;
	}
	// Adapters
	if( Pos == 1 ) {
		strcpy(Dest, "adapters");
		return 0;
	}
	// Pseudo Interfaces
	if( Pos == 2 ) {
		strcpy(Dest, "lo");
		return 0;
	}
	Pos -= 3;
	
	// Traverse the list
	for( iface = gIP_Interfaces; iface && Pos--; iface = iface->Next ) ;
	
	// Did we run off the end?
	if(!iface) {
		LEAVE('i', -EINTERNAL);
		return -EINVAL;
	}
	
	// Create the name
	strncpy(Dest, iface->Name, FILENAME_MAX);
	
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Get the node of an interface
 */
tVFS_Node *IPStack_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	#if 0
	 int	i, num;
	#endif
	tInterface	*iface;
	
	ENTER("pNode sName", Node, Name);
	
	// Routing subdir
	if( strcmp(Name, "routes") == 0 ) {
		LEAVE('p', &gIP_RouteNode);
		return &gIP_RouteNode;
	}
	
	// Adapters subdir
	if( strcmp(Name, "adapters") == 0 ) {
		LEAVE('p', &gIP_AdaptersNode);
		return &gIP_AdaptersNode;
	}
	
	// Loopback
	if( strcmp(Name, "lo") == 0 ) {
		LEAVE('p', &gIP_LoopInterface.Node);
		return &gIP_LoopInterface.Node;
	}
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next )
	{
		if( strcmp(iface->Name, Name) == 0 )
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
	BASE_IOCTLS(DRV_TYPE_MISC, "IPStack", VERSION, casIOCtls_Root)
		
	/*
	 * add_interface
	 * - Adds a new interface and binds it to a device
	 */
	case 4: {
		const struct {
			const char *Device;
			const char *Name;
			 int	Type;
		} *ifinfo = Data;
		
		if( Threads_GetUID() != 0 )
			LEAVE_RET('i', -1);
		if( !CheckMem(ifinfo, sizeof(*ifinfo)) )
			LEAVE_RET('i', -1);
		if( !MM_IsUser(ifinfo->Device) || !CheckString( ifinfo->Device ) )
			LEAVE_RET('i', -1);
		if( !MM_IsUser(ifinfo->Name) || !CheckString( ifinfo->Name ) )
			LEAVE_RET('i', -1);

		LOG("New interface of type %i for '%s' named '%s'",
			ifinfo->Type, ifinfo->Device, ifinfo->Name);
		tInterface *iface = IPStack_AddInterface(ifinfo->Device, ifinfo->Type, ifinfo->Name);
		if(iface == NULL)	LEAVE_RET('i', -1);

		tmp = iface->Node.ImplInt;
		LEAVE_RET('i', tmp);
		}
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn tInterface *IPStack_AddInterface(char *Device)
 * \brief Adds an interface to the list
 */
tInterface *IPStack_AddInterface(const char *Device, int Type, const char *Name)
{
	tInterface	*iface;
	tAdapter	*card;
	 int	nameLen, addrsize;
	
	ENTER("sDevice", Device);
	
	// Check address type
	addrsize = IPStack_GetAddressSize(Type);
	if( addrsize == -1 ) {
		Log_Debug("IPStack", "Bad address type %i", Type);
		return NULL;
	}

	// Open card
	card = Adapter_GetByName(Device);
	if( !card ) {
		Log_Debug("IPStack", "Unable to open card '%s'", Device);
		LEAVE('n');
		return NULL;	// ERR_YOURBAD
	}

	// Get name length	
	if( Name[0] )
	{
		nameLen = strlen(Name);
	}
	else
	{
		nameLen = snprintf(NULL, 0, "%i", giIP_NextIfaceId);
	}

	iface = malloc(
		sizeof(tInterface)
		+ nameLen + 1
		+ addrsize*3	// Address, Route->Network, Route->NextHop
		);
	if(!iface) {
		Log_Warning("IPStack", "AddInterface - malloc() failed");
		// TODO: Close card
		LEAVE('n');
		return NULL;	// Return ERR_MYBAD
	}
	
	iface->Next = NULL;
	iface->Type = Type;	// Unset type
	iface->Address = iface->Name + nameLen + 1;	// Address
	memset(iface->Address, 0, addrsize);
	memset(&iface->Route, 0, sizeof(iface->Route));
	iface->Route.Network = iface->Address + addrsize;
	memset(iface->Route.Network, 0, addrsize);
	iface->Route.NextHop = iface->Route.Network + addrsize;
	memset(iface->Route.NextHop, 0, addrsize);
	
	// Create Node
	iface->Node.ImplPtr = iface;
	iface->Node.Flags = VFS_FFLAG_DIRECTORY;
	iface->Node.Size = -1;
	iface->Node.NumACLs = 1;
	iface->Node.ACLs = &gVFS_ACL_EveryoneRX;
	iface->Node.Type = &gIP_InterfaceNodeType;
	
	// Set Defaults
	iface->TimeoutDelay = DEFAULT_TIMEOUT;
	
	// Get adapter handle
	iface->Adapter = card;
	
	// Set name
	if( Name[0] )
	{
		iface->Node.ImplInt = 0;
		strcpy(iface->Name, Name);
	}
	else
	{
		iface->Node.ImplInt = giIP_NextIfaceId++;
		sprintf(iface->Name, "%i", (int)iface->Node.ImplInt);
	}
	
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

	// Success!
	LEAVE('p', iface);
	return iface;
}

/**
 * \brief Adds a file to the socket list
 */
int IPStack_AddFile(tSocketFile *File)
{
//	Log_Log("IPStack", "Added file '%s'", File->Name);
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
int IPStack_Iface_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	tSocketFile	*file = gIP_FileTemplates;
	while(Pos-- && file) {
		file = file->Next;
	}
	
	if(!file)
		return -EINVAL;
	
	strncpy(Dest, file->Name, FILENAME_MAX);
	return 0;
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
	"get_device",
	"ping",
	NULL
	};
/**
 * \brief Handles IOCtls for the IPStack interfaces
 */
int IPStack_Iface_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp, size;
	tInterface	*iface = (tInterface*)Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	BASE_IOCTLS(DRV_TYPE_MISC, "IPStack", VERSION, casIOCtls_Iface)
	
	/*
	 * getset_type
	 * - Get/Set the interface type
	 */
	case 4:
		// Set Type?
		if( Data )
		{
#if 0
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
			
			// Set type
			iface->Type = *(int*)Data;
			LOG("Interface type set to %i", iface->Type);
			size = IPStack_GetAddressSize(iface->Type);
			// Check it's actually valid
			if( iface->Type != 0 && size == 0 ) {
				iface->Type = 0;
				LEAVE('i', -1);
				return -1;
			}
			
			// Clear address
			memset(iface->Address, 0, size);
#endif
			Log_Notice("IPStack", "Interface ioctl(settype) no longer usable");
		}
		LEAVE('i', iface->Type);
		return iface->Type;
	
	/*
	 * get_address
	 * - Get the interface's address
	 */
	case 5:
		size = IPStack_GetAddressSize(iface->Type);
		if( !CheckMem( Data, size ) )	LEAVE_RET('i', -1);
		memcpy( Data, iface->Address, size );
		LEAVE('i', 1);
		return 1;
	
	/*
	 * set_address
	 * - Set the interface's address
	 */
	case 6:
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		
		size = IPStack_GetAddressSize(iface->Type);
		if( !CheckMem( Data, size ) )	LEAVE_RET('i', -1);
		// TODO: Protect against trashing
		LOG("Interface address set to '%s'", IPStack_PrintAddress(iface->Type, Data));
		memcpy( iface->Address, Data, size );
		LEAVE_RET('i', 1);
	
	/*
	 * getset_subnet
	 * - Get/Set the bits in the address subnet
	 */
	case 7:
		// Do we want to set the value?
		if( Data )
		{
			// Are we root? (TODO: Check Owner/Group)
			if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
			// Is the memory valid
			if( !CheckMem(Data, sizeof(int)) )	LEAVE_RET('i', -1);
			
			// Is the mask sane?
			if( *(int*)Data < 0 || *(int*)Data > IPStack_GetAddressSize(iface->Type)*8-1 )
				LEAVE_RET('i', -1);
			LOG("Set subnet bits to %i", *(int*)Data);
			// Ok, set it
			iface->SubnetBits = *(int*)Data;
		}
		LEAVE_RET('i', iface->SubnetBits);
	
	/*
	 * get_device
	 * - Gets the name of the attached device
	 */
	case 8:
		if( iface->Adapter == NULL )
			LEAVE_RET('i', 0);
		char *name = Adapter_GetName(iface->Adapter);
		 int len = strlen(name);
		if( Data ) {
			if( !CheckMem( Data, len+1 ) ) {
				free(name);
				LEAVE_RET('i', -1);
			}
			strcpy( Data, name );
		}
		free(name);
		LEAVE_RET('i', len);
	
	/*
	 * ping
	 * - Send an ICMP Echo
	 */
	case 9:
		switch(iface->Type)
		{
		case 0:
			LEAVE_RET('i', 1);
		
		case 4:
			if( !CheckMem( Data, sizeof(tIPv4) ) )	LEAVE_RET('i', -1);
			tmp = IPv4_Ping(iface, *(tIPv4*)Data);
			LEAVE_RET('i', tmp);
			
		case 6:
			LEAVE_RET('i', 1);
		}
		break;
	
	}
	
	LEAVE('i', 0);
	return 0;
}


/*
 * Acess2 IP Stack
 * - Routing Tables
 */
#define DEBUG	0
#define VERSION	VER2(0,10)
#include <acess.h>
#include <tpl_drv_common.h>
#include "ipstack.h"
#include "link.h"

// === IMPORTS ===
tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Filename);

// === PROTOTYPES ===
// - Routes directory
char	*IPStack_RouteDir_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_RouteDir_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	IPStack_Route_Create(const char *InterfaceName);
tRoute	*IPStack_FindRoute(int AddressType, void *Address);
// - Individual Routes
 int	IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
 int	giIP_NextRouteId = 1;
tRoute	*gIP_Routes;
tRoute	*gIP_RoutesEnd;
tVFS_Node	gIP_RouteNode = {
	Flags: VFS_FFLAG_DIRECTORY,
	Size: -1,
	NumACLs: 1,
	ACLs: &gVFS_ACL_EveryoneRX,
	ReadDir: IPStack_RouteDir_ReadDir,
	FindDir: IPStack_RouteDir_FindDir,
	IOCtl: IPStack_RouteDir_IOCtl
};

// === CODE ===
/**
 * \brief ReadDir for the /Devices/ip/routes/ directory
 */
char *IPStack_RouteDir_ReadDir(tVFS_Node *Node, int Pos)
{
	tRoute	*rt;
	
	for(rt = gIP_Routes; rt && Pos --; rt = rt->Next);
	
	if( !rt ) {
		return NULL;
	}
	
	{
		 int	len = sprintf(NULL, "%i", rt->Node.Inode);
		char	buf[len+1];
		sprintf(buf, "%i", rt->Node.Inode);
		return strdup(buf);
	}
}

/**
 * \brief FindDir for the /Devices/ip/routes/ directory
 */
tVFS_Node *IPStack_RouteDir_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	num = atoi(Name);
	tRoute	*rt;
	
	// Zero is invalid, sorry :)
	if( !num )	return NULL;
	
	// The list is inherently sorted, so we can do a quick search
	for(rt = gIP_Routes; rt && rt->Node.Inode < num; rt = rt->Next);
	// Fast fail on larger number
	if( rt->Node.Inode > num )
		return NULL;
	
	return &rt->Node;
}

/**
 * \brief Names for the route list IOCtl Calls
 */
static const char *casIOCtls_RouteDir[] = {
	DRV_IOCTLNAMES,
	"add_route",	// Add a route - char *InterfaceName
	"locate_route",	// Find the best route for an address - struct {int Type, char Address[]} *
	NULL
	};

/**
 * \brief IOCtl for /Devices/ip/routes/
 */
int IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp;
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
		tmp = ModUtil_LookupString( (char**)casIOCtls_RouteDir, (char*)Data );
		LEAVE('i', tmp);
		return tmp;
	
	case 4:	// Add Route
		if( !CheckString(Data) )	return -1;
		return IPStack_Route_Create(Data);
	
	case 5:	// Locate Route
		{
			struct {
				 int	Type;
				Uint8	Addr[];
			}	*data = Data;
			tRoute	*rt;
			
			if( !CheckMem(Data, sizeof(int)) )
				return -1;
			if( !CheckMem(Data, sizeof(int) + IPStack_GetAddressSize(data->Type)) )
				return -1;
			
			rt = IPStack_FindRoute(data->Type, data->Addr);
			
			if( !rt )
				return 0;
			
			return rt->Node.Inode;
		}
		break;
	}
	return 0;
}

/**
 * \brief Create a new route entry
 * \param InterfaceName	Name of the interface using this route
 */
int IPStack_Route_Create(const char *InterfaceName)
{
	tRoute	*rt;
	tInterface	*iface;
	 int	size;
	
	// Get interface
	// Note: Oh man! This is such a hack
	{
		tVFS_Node	*node = IPStack_Root_FindDir(NULL, InterfaceName);
		if( !node )	return 0;
		iface = node->ImplPtr;
	}
	
	// Get the size of the specified address type
	size = IPStack_GetAddressSize(iface->Type);
	if( size == 0 ) {
		return 0;
	}
	
	// Allocate space
	rt = calloc(1, sizeof(tRoute) + size*2 );
	
	// Set up node
	rt->Node.ImplPtr = rt;
	rt->Node.Inode = giIP_NextRouteId ++;
	rt->Node.Size = 0;
	rt->Node.NumACLs = 1,
	rt->Node.ACLs = &gVFS_ACL_EveryoneRO;
	rt->Node.IOCtl = IPStack_Route_IOCtl;
	
	// Set up state
	rt->AddressType = iface->Type;
	rt->Network = (void *)( (tVAddr)rt + sizeof(tRoute) );
	rt->SubnetBits = 0;
	rt->NextHop = (void *)( (tVAddr)rt + sizeof(tRoute) + size );
	rt->Interface = iface;
	
	// Add to list
	if( gIP_RoutesEnd ) {
		gIP_RoutesEnd->Next = rt;
		gIP_RoutesEnd = rt;
	}
	else {
		gIP_Routes = gIP_RoutesEnd = rt;
	}
	
	return rt->Node.Inode;
}

/**
 */
tRoute	*IPStack_FindRoute(int AddressType, void *Address)
{
	tRoute	*rt;
	tRoute	*best = NULL;
	
	for( rt = gIP_Routes; rt; rt = rt->Next )
	{
		if( rt->AddressType != AddressType )	continue;
		
		if( !IPStack_CompareAddress(AddressType, rt->Network, Address, rt->SubnetBits) )
			continue;
		
		if( best ) {
			// More direct routes are preferred
			if( best->SubnetBits > rt->SubnetBits )	continue;
			// If equally direct, choose the best metric
			if( best->SubnetBits == rt->SubnetBits && best->Metric < rt->Metric )	continue;
		}
		
		best = rt;
	}
	
	return best;
}

/**
 * \brief Names for route IOCtl Calls
 */
static const char *casIOCtls_Route[] = {
	DRV_IOCTLNAMES,
	"get_network",	// Get network - (void *Data), returns boolean success
	"set_network",	// Set network - (void *Data), returns boolean success
	"get_nexthop",	// Get next hop - (void *Data), returns boolean success
	"set_nexthop",	// Set next hop - (void *Data), returns boolean success
	"getset_subnetbits",	// Get/Set subnet bits - (int *Bits), returns current value
	"getset_metric",	// Get/Set metric - (int *Metric), returns current value
	"get_interface",	// Get interface name - (char *Name), returns name length, NULL OK
	NULL
	};

/**
 * \brief IOCtl for /Devices/ip/routes/#
 */
int IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp;
	 int	*iData = Data;
	tRoute	*rt = Node->ImplPtr;
	 int	addrSize = IPStack_GetAddressSize(rt->AddressType);
	
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
		tmp = ModUtil_LookupString( (char**)casIOCtls_Route, (char*)Data );
		LEAVE('i', tmp);
		return tmp;
	
	// Get Network
	case 4:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(Data, rt->Network, addrSize);
		return 1;
	// Set Network
	case 5:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(rt->Network, Data, addrSize);
		return 1;
	
	// Get Next Hop
	case 6:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(Data, rt->NextHop, addrSize);
		return 1;
	// Set Next Hop
	case 7:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(rt->NextHop, Data, addrSize);
		return 1;
	
	// Get/Set Subnet Bits
	case 8:
		if( Data ) {
			if( !CheckMem(Data, sizeof(int)) )	return -1;
			if( *iData < 0 || *iData > addrSize*8 )
				return -1;
			rt->SubnetBits = *iData;
		}
		return rt->SubnetBits;
	
	// Get/Set Metric
	case 9:
		if( Data ) {
			if( !CheckMem(Data, sizeof(int)) )	return -1;
			if( *iData < 0 )	return -1;
			rt->Metric = *iData;
		}
		return rt->Metric;
	
	// Get interface name
	case 10:
		if( Data ) {
			if( !CheckMem(Data, strlen(rt->Interface->Name) + 1) )
				return -1;
			strcpy(Data, rt->Interface->Name);
		}
		return strlen(rt->Interface->Name);
	
	default:
		return 0;
	}
}

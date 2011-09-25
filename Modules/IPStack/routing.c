/*
 * Acess2 IP Stack
 * - Routing Tables
 */
#define DEBUG	0
#define VERSION	VER2(0,10)
#include <acess.h>
#include <api_drv_common.h>
#include "ipstack.h"
#include "link.h"

#define	DEFAUTL_METRIC	30

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;
extern tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Filename);

// === PROTOTYPES ===
// - Routes directory
char	*IPStack_RouteDir_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_RouteDir_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data);
// - Route Management
tRoute	*IPStack_Route_Create(const char *InterfaceName);
tRoute	*IPStack_AddRoute(const char *Interface, void *Network, int SubnetBits, void *NextHop, int Metric);
tRoute	*IPStack_FindRoute(int AddressType, tInterface *Interface, void *Address);
// - Individual Routes
 int	IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
 int	giIP_NextRouteId = 1;
tRoute	*gIP_Routes;
tRoute	*gIP_RoutesEnd;
tVFS_Node	gIP_RouteNode = {
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.ReadDir = IPStack_RouteDir_ReadDir,
	.FindDir = IPStack_RouteDir_FindDir,
	.IOCtl = IPStack_RouteDir_IOCtl
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
	
	// Interpret the name as <type>:<addr>, returning the interface for
	// needed to access that address.
	//   E.g. '4:0A02000A'	- 10.2.0.10
	// Hm... It could do with a way to have a better address type representation
	if( Name[1] == ':' )	// TODO: Allow variable length type codes
	{
		 int	addrSize = IPStack_GetAddressSize(num);
		Uint8	addrData[addrSize];
		
		// Errof if the size is invalid
		if( strlen(Name) != 2 + addrSize*2 )
			return NULL;
		
		// Parse the address
		// - Error if the address data is not fully hex
		if( UnHex(addrData, addrSize, Name + 2) != addrSize )
			return NULL;
		
		// Find the route
		rt = IPStack_FindRoute(num, NULL, addrData);
		if(!rt)	return NULL;
		
		// Return the interface node
		// - Sure it's hijacking it from inteface.c's area, but it's
		//   simpler this way
		return &rt->Interface->Node;
	}
	
	// The list is inherently sorted, so we can do a quick search
	for(rt = gIP_Routes; rt && rt->Node.Inode < num; rt = rt->Next);
	
	// Fast fail end of list / larger number
	if( !rt || rt->Node.Inode > num )
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
	tRoute	*rt;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	BASE_IOCTLS(DRV_TYPE_MISC, STR(IDENT), VERSION, casIOCtls_RouteDir)
	
	case 4:	// Add Route
		if( !CheckString(Data) )	LEAVE_RET('i', -1);
		rt = IPStack_Route_Create(Data);
		if( !rt )
			tmp = -1;
		else
			tmp = rt->Node.Inode;
		LEAVE('i', tmp);
		return tmp;
	
	case 5:	// Locate Route
		{
			struct {
				 int	Type;
				Uint8	Addr[];
			}	*data = Data;
			
			if( !CheckMem(Data, sizeof(int)) )
				LEAVE_RET('i', -1);
			if( !CheckMem(Data, sizeof(int) + IPStack_GetAddressSize(data->Type)) )
				LEAVE_RET('i', -1);
			
			Log_Debug("IPStack", "Route_RouteDir_IOCtl - FindRoute %i, %s",
				data->Type, IPStack_PrintAddress(data->Type, data->Addr) );
			rt = IPStack_FindRoute(data->Type, NULL, data->Addr);
			
			if( !rt )
				LEAVE_RET('i', 0);
			
			LEAVE('i', rt->Node.Inode);
			return rt->Node.Inode;
		}
		break;
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Create a new route entry
 * \param InterfaceName	Name of the interface using this route
 */
tRoute *IPStack_Route_Create(const char *InterfaceName)
{
	tRoute	*rt;
	tInterface	*iface;
	 int	size;
	
	// Get interface
	// Note: Oh man! This is such a hack
	{
		tVFS_Node	*node = IPStack_Root_FindDir(NULL, InterfaceName);
		if( !node ) {
			Log_Debug("IPStack", "IPStack_Route_Create - Unknown interface '%s'\n", InterfaceName);
			return NULL;
		}
		iface = node->ImplPtr;
		if(node->Close)	node->Close(node);
	}
	
	// Get the size of the specified address type
	size = IPStack_GetAddressSize(iface->Type);
	if( size == 0 ) {
		return NULL;
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
	rt->Metric = DEFAUTL_METRIC;
	memset(rt->Network, 0, size);
	memset(rt->NextHop, 0, size);
	
	// Add to list
	if( gIP_RoutesEnd ) {
		gIP_RoutesEnd->Next = rt;
		gIP_RoutesEnd = rt;
	}
	else {
		gIP_Routes = gIP_RoutesEnd = rt;
	}
	
	Log_Log("IPStack", "Route entry for '%s' created", InterfaceName);
	
	return rt;
}

/**
 * \brief Add and fill a route
 */
tRoute *IPStack_AddRoute(const char *Interface, void *Network, int SubnetBits, void *NextHop, int Metric)
{
	tRoute	*rt = IPStack_Route_Create(Interface);
	 int	addrSize;
	
	if( !rt )	return NULL;
	
	addrSize = IPStack_GetAddressSize(rt->Interface->Type);
	
	memcpy(rt->Network, Network, addrSize);
	if( NextHop )
		memcpy(rt->NextHop, NextHop, addrSize);
	rt->SubnetBits = SubnetBits;
	if( Metric )
		rt->Metric = Metric;
	
	return rt;
}

/**
 */
tRoute *IPStack_FindRoute(int AddressType, tInterface *Interface, void *Address)
{
	tRoute	*rt;
	tRoute	*best = NULL;
	tInterface	*iface;
	 int	addrSize;
	
	ENTER("iAddressType pInterface sAddress",
		AddressType, Interface, IPStack_PrintAddress(AddressType, Address));
	
	if( Interface && AddressType != Interface->Type ) {
		LOG("Interface->Type (%i) != AddressType", Interface->Type);
		LEAVE('n');
		return NULL;
	}
	
	// Get address size
	addrSize = IPStack_GetAddressSize(AddressType);
	
	// Check against explicit routes
	for( rt = gIP_Routes; rt; rt = rt->Next )
	{
		// Check interface
		if( Interface && rt->Interface != Interface )	continue;
		// Check address type
		if( rt->AddressType != AddressType )	continue;
		
		LOG("Checking network %s/%i", IPStack_PrintAddress(AddressType, rt->Network), rt->SubnetBits);
		
		// Check if the address matches
		if( !IPStack_CompareAddress(AddressType, rt->Network, Address, rt->SubnetBits) )
			continue;
		
		if( best ) {
			// More direct routes are preferred
			if( best->SubnetBits > rt->SubnetBits ) {
				LOG("Skipped - less direct (%i < %i)", rt->SubnetBits, best->SubnetBits);
				continue;
			}
			// If equally direct, choose the best metric
			if( best->SubnetBits == rt->SubnetBits && best->Metric < rt->Metric ) {
				LOG("Skipped - higher metric (%i > %i)", rt->Metric, best->Metric);
				continue;
			}
		}
		
		best = rt;
	}
	
	// Check against implicit routes
	if( !best && !Interface )
	{
		for( iface = gIP_Interfaces; iface; iface = iface->Next )
		{
			if( Interface && iface != Interface )	continue;
			if( iface->Type != AddressType )	continue;
			
			
			// Check if the address matches
			if( !IPStack_CompareAddress(AddressType, iface->Address, Address, iface->SubnetBits) )
				continue;
			
			if( best ) {
				// More direct routes are preferred
				if( best->SubnetBits > rt->SubnetBits ) {
					LOG("Skipped - less direct (%i < %i)", rt->SubnetBits, best->SubnetBits);
					continue;
				}
				// If equally direct, choose the best metric
				if( best->SubnetBits == rt->SubnetBits && best->Metric < rt->Metric ) {
					LOG("Skipped - higher metric (%i > %i)", rt->Metric, best->Metric);
					continue;
				}
			}
			
			rt = &iface->Route;
			memcpy(rt->Network, iface->Address, addrSize);
			memset(rt->NextHop, 0, addrSize);
			rt->Metric = DEFAUTL_METRIC;
			rt->SubnetBits = iface->SubnetBits;
			
			best = rt;
		}
	}
	if( !best && Interface )
	{
		rt = &Interface->Route;
		// Make sure route is up to date
		memcpy(rt->Network, Interface->Address, addrSize);
		memset(rt->NextHop, 0, addrSize);
		rt->Metric = DEFAUTL_METRIC;
		rt->SubnetBits = Interface->SubnetBits;
		
		if( IPStack_CompareAddress(AddressType, rt->Network, Address, rt->SubnetBits) )
		{
			best = rt;
		}
	}
	
	LEAVE('p', best);
	return best;
}

/**
 * \brief Names for route IOCtl Calls
 */
static const char *casIOCtls_Route[] = {
	DRV_IOCTLNAMES,
	"get_type",	// Get address type - (void), returns integer type
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
	 int	*iData = Data;
	tRoute	*rt = Node->ImplPtr;
	 int	addrSize = IPStack_GetAddressSize(rt->AddressType);
	
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	BASE_IOCTLS(DRV_TYPE_MISC, STR(IDENT), VERSION, casIOCtls_Route)
	
	// Get address type
	case 4:
		return rt->AddressType;
	
	// Get Network
	case 5:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(Data, rt->Network, addrSize);
		return 1;
	// Set Network
	case 6:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(rt->Network, Data, addrSize);
		return 1;
	
	// Get Next Hop
	case 7:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(Data, rt->NextHop, addrSize);
		return 1;
	// Set Next Hop
	case 8:
		if( !CheckMem(Data, addrSize) )	return -1;
		memcpy(rt->NextHop, Data, addrSize);
		return 1;
	
	// Get/Set Subnet Bits
	case 9:
		if( Data ) {
			if( !CheckMem(Data, sizeof(int)) )	return -1;
			if( *iData < 0 || *iData > addrSize*8 )
				return -1;
			rt->SubnetBits = *iData;
		}
		return rt->SubnetBits;
	
	// Get/Set Metric
	case 10:
		if( Data ) {
			if( !CheckMem(Data, sizeof(int)) )	return -1;
			if( *iData < 0 )	return -1;
			rt->Metric = *iData;
		}
		return rt->Metric;
	
	// Get interface name
	case 11:
		if( Data ) {
			if( !CheckMem(Data, strlen(rt->Interface->Name) + 1) )
				return -1;
			strcpy(Data, rt->Interface->Name);
		}
		return strlen(rt->Interface->Name);
	
	default:
		return -1;
	}
}

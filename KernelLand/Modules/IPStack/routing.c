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
 int	IPStack_RouteDir_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	IPStack_RouteDir_Unlink(tVFS_Node *Node, const char *OldName);
tRoute	*_Route_FindExactRoute(int Type, void *Network, int Subnet, int Metric);
 int	_Route_ParseRouteName(const char *Name, void *Addr, int *SubnetBits, int *Metric);
 int	IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data);
// - Route Management
tRoute	*IPStack_Route_Create(int AddrType, void *Network, int SubnetBits, int Metric);
tRoute	*IPStack_AddRoute(const char *Interface, void *Network, int SubnetBits, void *NextHop, int Metric);
tRoute	*_Route_FindInterfaceRoute(int AddressType, void *Address);
tRoute	*IPStack_FindRoute(int AddressType, tInterface *Interface, void *Address);
// - Individual Routes
 int	IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
 int	giIP_NextRouteId = 1;
tRoute	*gIP_Routes;
tRoute	*gIP_RoutesEnd;
tVFS_NodeType	gIP_RouteNodeType = {
	.IOCtl = IPStack_Route_IOCtl
};
tVFS_NodeType	gIP_RouteDirNodeType = {
	.ReadDir = IPStack_RouteDir_ReadDir,
	.FindDir = IPStack_RouteDir_FindDir,
	.MkNod = IPStack_RouteDir_MkNod,
	.Unlink = IPStack_RouteDir_Unlink,
	.IOCtl = IPStack_RouteDir_IOCtl
};
tVFS_Node	gIP_RouteNode = {
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Type = &gIP_RouteDirNodeType
};

// === CODE ===
/**
 * \brief ReadDir for the /Devices/ip/routes/ directory
 */
char *IPStack_RouteDir_ReadDir(tVFS_Node *Node, int Pos)
{
	tRoute	*rt;
	
	for(rt = gIP_Routes; rt && Pos --; rt = rt->Next);
	if( !rt )	return NULL;
	
	{
		 int	addrlen = IPStack_GetAddressSize(rt->AddressType);
		 int	len = sprintf(NULL, "%i::%i:%i", rt->AddressType, rt->SubnetBits, rt->Metric) + addrlen*2;
		char	buf[len+1];
		 int	ofs;
		ofs = sprintf(buf, "%i:", rt->AddressType);
		ofs += Hex(buf+ofs, addrlen, rt->Network);
		sprintf(buf+ofs, ":%i:%i", rt->SubnetBits, rt->Metric);
		return strdup(buf);
	}
}

/**
 * \brief FindDir for the /Devices/ip/routes/ directory
 */
tVFS_Node *IPStack_RouteDir_FindDir(tVFS_Node *Node, const char *Name)
{
	// Interpret the name as <type>:<addr>, returning the interface for
	// needed to access that address.
	//   E.g. '@4:0A02000A'	- 10.2.0.10
	// Hm... It could do with a way to have a better address type representation
	if( Name[0] == '@' )
	{
		tRoute	*rt;
		 int	ofs = 1;
	 	 int	type;
		
		ofs += ParseInt(Name+ofs, &type);
		 int	addrSize = IPStack_GetAddressSize(type);
		Uint8	addrData[addrSize];

		// Separator
		if( Name[ofs] != ':' )	return NULL;
		ofs ++;

		// Error if the size is invalid
		if( strlen(Name+ofs) != addrSize*2 )
			return NULL;
		
		// Parse the address
		// - Error if the address data is not fully hex
		if( UnHex(addrData, addrSize, Name + ofs) != addrSize )
			return NULL;
		
		// Find the route
		rt = IPStack_FindRoute(type, NULL, addrData);
		if(!rt)	return NULL;
	
		if( !rt->Interface )
		{	
			LOG("Why does this route not have a node? trying to find an iface for the next hop");

			rt = _Route_FindInterfaceRoute(type, rt->NextHop);
			if(!rt) {
				Log_Notice("Cannot find route to next hop '%s'",
					IPStack_PrintAddress(type, rt->NextHop));
				return NULL;
			}
		}
		if( !rt->Interface ) {
			Log_Notice("Routes", "No interface for route %p, what the?", rt);
			return NULL;
		}
		// Return the interface node
		// - Sure it's hijacking it from inteface.c's area, but it's
		//   simpler this way
		return &rt->Interface->Node;
	}
	else if( Name[0] == '#' )
	{
		int num, ofs = 1;
		
		ofs = ParseInt(Name+ofs, &num);
		if( ofs == 1 )	return NULL;
		if( Name[ofs] != '\0' )	return NULL;
		if( num < 0)	return NULL;		

		for( tRoute *rt = gIP_Routes; rt; rt = rt->Next )
		{
			if( rt->Node.Inode > num )	return NULL;
			if( rt->Node.Inode == num )	return &rt->Node;
		}
		return NULL;
	}
	else
	{
		 int	type = _Route_ParseRouteName(Name, NULL, NULL, NULL);
		if( type <= 0 )	return NULL;

		 int	addrSize = IPStack_GetAddressSize(type);
		Uint8	addrData[addrSize];
		 int	subnet_bits, metric;
		
		_Route_ParseRouteName(Name, addrData, &subnet_bits, &metric);

		tRoute *rt = _Route_FindExactRoute(type, addrData, subnet_bits, metric);
		if(rt)	return &rt->Node;
		return NULL;
	}
}

/**
 * \brief Create a new route node
 */
int IPStack_RouteDir_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	if( Flags )	return -EINVAL;	
	if( Threads_GetUID() != 0 )	return -EACCES;

	 int	type = _Route_ParseRouteName(Name, NULL, NULL, NULL);
	if( type <= 0 )	return -EINVAL;

	 int	size = IPStack_GetAddressSize(type);
	Uint8	addrdata[size];	
	 int	subnet, metric;

	_Route_ParseRouteName(Name, addrdata, &subnet, &metric);

	// Check for duplicates
	if( _Route_FindExactRoute(type, addrdata, subnet, metric) )
		return -EEXIST;

	IPStack_Route_Create(type, addrdata, subnet, metric);

	return 0;
}

/**
 * \brief Rename / Delete a route
 */
int IPStack_RouteDir_Unlink(tVFS_Node *Node, const char *OldName)
{
	tRoute	*rt;
	
	if( Threads_GetUID() != 0 )	return -EACCES;

	// Get the original route entry
	{
		 int	type = _Route_ParseRouteName(OldName, NULL, NULL, NULL);
		if(type <= 0)	return -EINVAL;
		Uint8	addr[IPStack_GetAddressSize(type)];
		 int	subnet, metric;
		_Route_ParseRouteName(OldName, addr, &subnet, &metric);
		
		rt = _Route_FindExactRoute(type, addr, subnet, metric);
	}	

	// Delete the route
	tRoute	*prev = NULL;
	for(tRoute *r = gIP_Routes; r && r != rt; prev = r, r = r->Next);
	
	if(prev)
		prev->Next = rt->Next;
	else
		gIP_Routes = rt->Next;
	free(rt);
	return 0;
}

tRoute *_Route_FindExactRoute(int Type, void *Network, int Subnet, int Metric)
{
	for(tRoute *rt = gIP_Routes; rt; rt = rt->Next)
	{
		if( rt->AddressType != Type )	continue;
		if( rt->Metric != Metric )	continue;
		if( rt->SubnetBits != Subnet )	continue;
		if( IPStack_CompareAddress(Type, rt->Network, Network, -1) == 0 )
			continue ;
		return rt;
	}
	return NULL;
}

int _Route_ParseRouteName(const char *Name, void *Addr, int *SubnetBits, int *Metric)
{
	 int	type, addrlen;
	 int	ofs = 0, ilen;
	
	ENTER("sName pAddr pSubnetBits pMetric", Name, Addr, SubnetBits, Metric);

	ilen = ParseInt(Name, &type);
	if(ilen == 0) {
		LOG("Type failed to parse");
		LEAVE_RET('i', -1);
	}
	ofs += ilen;
	
	if(Name[ofs] != ':')	LEAVE_RET('i', -1);
	ofs ++;

	addrlen = IPStack_GetAddressSize(type);
	if( Addr )
	{
		if( UnHex(Addr, addrlen, Name + ofs) != addrlen )
			return -1;
	}
	ofs += addrlen*2;
	
	if(Name[ofs] != ':')	LEAVE_RET('i', -1);
	ofs ++;

	ilen = ParseInt(Name+ofs, SubnetBits);
	if(ilen == 0) {
		LOG("Subnet failed to parse");
		LEAVE_RET('i', -1);
	}
	ofs += ilen;
	
	if(Name[ofs] != ':')	LEAVE_RET('i', -1);
	ofs ++;

	ilen = ParseInt(Name+ofs, Metric);
	if(ilen == 0) {
		LOG("Metric failed to parse");
		LEAVE_RET('i', -1);
	}
	ofs += ilen;
	
	if(Name[ofs] != '\0')	LEAVE_RET('i', -1);

	LEAVE('i', type);
	return type;
}

/**
 * \brief Names for the route list IOCtl Calls
 */
static const char *casIOCtls_RouteDir[] = {
	DRV_IOCTLNAMES,
	"locate_route",	// Find the best route for an address - struct {int Type, char Address[]} *
	NULL
	};

/**
 * \brief IOCtl for /Devices/ip/routes/
 */
int IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tRoute	*rt;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	BASE_IOCTLS(DRV_TYPE_MISC, STR(IDENT), VERSION, casIOCtls_RouteDir)
	
	case 4:	// Locate Route
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
tRoute *IPStack_Route_Create(int AddrType, void *Network, int SubnetBits, int Metric)
{
	tRoute	*rt;
	 int	size;
	
	// Get the size of the specified address type
	size = IPStack_GetAddressSize(AddrType);
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
	rt->Node.Type = &gIP_RouteNodeType;
	
	// Set up state
	rt->AddressType = AddrType;
	rt->Network = (void *)( (tVAddr)rt + sizeof(tRoute) );
	rt->SubnetBits = SubnetBits;
	rt->NextHop = (void *)( (tVAddr)rt + sizeof(tRoute) + size );
	rt->Interface = NULL;
	rt->Metric = Metric;
	memcpy(rt->Network, Network, size);
	memset(rt->NextHop, 0, size);
	
	// Clear non-fixed bits
	{
		Uint8	*data = rt->Network;
		 int	i;
		i = SubnetBits / 8;
		if( SubnetBits % 8 ) {
			data[i] &= ~((1 << (8 - SubnetBits % 8)) - 1);
			i ++;
		}
		memset(data + i, 0, size - i);
	}	


	// Add to list
	if( gIP_RoutesEnd ) {
		gIP_RoutesEnd->Next = rt;
		gIP_RoutesEnd = rt;
	}
	else {
		gIP_Routes = gIP_RoutesEnd = rt;
	}
	
//	Log_Log("IPStack", "Route entry for '%s' created", InterfaceName);
	
	return rt;
}

/**
 * \brief Add and fill a route
 */
tRoute *IPStack_AddRoute(const char *Interface, void *Network, int SubnetBits, void *NextHop, int Metric)
{
	tInterface	*iface;
	tRoute	*rt;
	 int	addrSize;
	
	{
		tVFS_Node	*tmp;
		tmp = IPStack_Root_FindDir(NULL, Interface);
		if(!tmp)	return NULL;
		iface = tmp->ImplPtr;
		if(tmp->Type->Close)	tmp->Type->Close(tmp);
	}

	rt = IPStack_Route_Create(iface->Type, Network, SubnetBits, Metric);
	if( !rt )	return NULL;
	
	addrSize = IPStack_GetAddressSize(iface->Type);
	rt->Interface = iface;
	
	if( NextHop )
		memcpy(rt->NextHop, NextHop, addrSize);
	
	return rt;
}

/**
 * \brief Locates what interface should be used to get directly to an address
 */
tRoute *_Route_FindInterfaceRoute(int AddressType, void *Address)
{
	tRoute	*best = NULL, *rt;
	 int addrSize = IPStack_GetAddressSize(AddressType);
	for( tInterface *iface = gIP_Interfaces; iface; iface = iface->Next )
	{
		if( iface->Type != AddressType )	continue;
		
		// Check if the address matches
		if( !IPStack_CompareAddress(AddressType, iface->Address, Address, iface->SubnetBits) )
			continue;
		
		rt = &iface->Route;
		if( !rt->Interface )
		{
			memcpy(rt->Network, iface->Address, addrSize);
			memset(rt->NextHop, 0, addrSize);
			rt->Metric = DEFAUTL_METRIC;
			rt->SubnetBits = iface->SubnetBits;
			rt->Interface = iface;
		}
		
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

	return best;
}

/**
 */
tRoute *IPStack_FindRoute(int AddressType, tInterface *Interface, void *Address)
{
	tRoute	*rt;
	tRoute	*best = NULL;
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
		if( Interface && rt->Interface && rt->Interface != Interface )	continue;
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
		best = _Route_FindInterfaceRoute(AddressType, Address);
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
	"get_nexthop",	// Get next hop - (void *Data), returns boolean success
	"set_nexthop",	// Set next hop - (void *Data), returns boolean success
	"get_interface",	// Get interface name - (char *Name), returns name length, NULL OK
	"set_interface",	// Set interface - (const char *Name)
	NULL
	};

/**
 * \brief IOCtl for /Devices/ip/routes/#
 */
int IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tRoute	*rt = Node->ImplPtr;
	 int	addrSize = IPStack_GetAddressSize(rt->AddressType);

	ENTER("pNode iID pData", Node, ID, Data);
	
	switch(ID)
	{
	// --- Standard IOCtls (0-3) ---
	BASE_IOCTLS(DRV_TYPE_MISC, STR(IDENT), VERSION, casIOCtls_Route)
	
	// Get Next Hop
	case 4:
		if( !CheckMem(Data, addrSize) )	LEAVE_RET('i', -1);
		memcpy(Data, rt->NextHop, addrSize);
		LEAVE_RET('i', 1);
	// Set Next Hop
	case 5:
		if( Threads_GetUID() != 0 )	LEAVE_RET('i', -1);
		if( !CheckMem(Data, addrSize) )	LEAVE_RET('i', -1);
		memcpy(rt->NextHop, Data, addrSize);
		LEAVE_RET('i', 1);

	// Get interface name
	case 6:
		if( !rt->Interface ) {
			if(Data && !CheckMem(Data, 1) )
				LEAVE_RET('i', -1);
			if(Data)
				*(char*)Data = 0;
			LEAVE_RET('i', 0);
		}
		if( Data ) {
			if( !CheckMem(Data, strlen(rt->Interface->Name) + 1) )
				LEAVE_RET('i', -1);
			strcpy(Data, rt->Interface->Name);
		}
		LEAVE_RET('i', strlen(rt->Interface->Name));
	// Set interface name
	case 7:
		if( Threads_GetUID() != 0 )
			LEAVE_RET('i', -1);
		if( !CheckString(Data) )
			LEAVE_RET('i', -1);
		else
		{
			tInterface	*iface;
			tVFS_Node	*tmp;
			tmp = IPStack_Root_FindDir(NULL, Data);
			if(!tmp)
				LEAVE_RET('i', -1);
			iface = tmp->ImplPtr;
			if(tmp->Type->Close)	tmp->Type->Close(tmp);
			
			if( iface->Type != rt->AddressType )
				LEAVE_RET('i', -1);

			// TODO: Other checks?		
	
			rt->Interface = iface;
		}
		LEAVE_RET('i', 0);

	default:
		LEAVE_RET('i', -1);
	}
}

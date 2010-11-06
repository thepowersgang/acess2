/*
 * Acess2 IP Stack
 * - Routing Tables
 */
#include "ipstack.h"
#include "link.h"


// === TYPES ===
typedef struct sRoute {
	struct sRoute	*Next;
	tVFS_Node	Node;
	
	 int	AddressType;	// 0: Invalid, 4: IPv4, 6: IPv4
	void	*Network;	// Pointer to tIPv4/tIPv6/... at end of structure
	 int	SubnetBits;
	void	*NextHop;	// Pointer to tIPv4/tIPv6/... at end of structure
	tInterface	*Interface;
}	tRoute;

// === PROTOTYPES ===
// - Routes directory
char	*IPStack_RouteDir_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_RouteDir_FindDir(tVFS_Node *Node, const char *Name);
 int	IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data);
 int	IPStack_Route_Create(const char *InterfaceName);
// - Individual Routes
Uint64	IPStack_Route_IOCtl(tVFS_Node *Node, int ID, void *Data);


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
		 int	len = snprintf(NULL, 12, "%i", rt->Node.Inode);
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
 * \brief Names for interface IOCtl Calls
 */
static const char *casIOCtls_RouteDir[] = {
	DRV_IOCTLNAMES,
	"add_route"	// Add a route - char *InterfaceName
	NULL
	};

int IPStack_RouteDir_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
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
	}
	return 0;
}

/**
 * \brief Create a new route entry
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
	size = IPStack_GetAddressSize(iface->AddressType);
	if( size == 0 ) {
		return 0;
	}
	
	// Allocate space
	rt = calloc(1, sizeof(tRoute) + size*2 );
	
	rt->Next = NULL;
	//rt->Node
	
	rt->AddressType = iface->AddressType;
	rt->Network = (void *)( (tVAddr)rt + sizeof(tRoute) );
	rt->SubnetBits = 0;
	rt->NextHop = (void *)( (tVAddr)rt + sizeof(tRoute) + size );
	rt->Interface = iface;
}

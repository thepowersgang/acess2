/*
 * Acess2 IP Stack
 * - Stack Initialisation
 */
#define DEBUG	0
#define VERSION	VER2(0,10)
#include "ipstack.h"
#include "link.h"
#include <modules.h>
#include <fs_devfs.h>

// === IMPORTS ===
extern int	ARP_Initialise();
extern void	UDP_Initialise();
extern void	TCP_Initialise();
extern int	IPv4_Initialise();
extern int	IPv6_Initialise();

extern tAdapter	*IPStack_GetAdapter(const char *Path);
extern char	*IPStack_Root_ReadDir(tVFS_Node *Node, int Pos);
extern tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Name);
extern int	IPStack_Root_IOCtl(tVFS_Node *Node, int ID, void *Data);
extern tInterface	gIP_LoopInterface;
extern tInterface	*IPStack_AddInterface(const char *Device, const char *Name);
extern tRoute	*IPStack_AddRoute(const char *Interface, void *Network, int SubnetBits, void *NextHop, int Metric);

// === PROTOTYPES ===
 int	IPStack_Install(char **Arguments);
 int	IPStack_CompareAddress(int AddressType, const void *Address1, const void *Address2, int CheckBits);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, IPStack, IPStack_Install, NULL, NULL);
tDevFS_Driver	gIP_DriverInfo = {
	NULL, "ip",
	{
	.Size = -1,	// Number of interfaces
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = IPStack_Root_ReadDir,
	.FindDir = IPStack_Root_FindDir,
	.IOCtl = IPStack_Root_IOCtl
	}
};

// === CODE ===
/**
 * \fn int IPStack_Install(char **Arguments)
 * \brief Intialise the relevant parts of the stack and register with DevFS
 */
int IPStack_Install(char **Arguments)
{
	 int	i = 0;
	
	// Layer 3 - Network Layer Protocols
	ARP_Initialise();
	IPv4_Initialise();
	IPv6_Initialise();
	// Layer 4 - Transport Layer Protocols
	TCP_Initialise();
	UDP_Initialise();
	
	if(Arguments)
	{
		// Parse module arguments
		for( i = 0; Arguments[i]; i++ )
		{
			// TODO:
			// Define interfaces by <Device>:<Type>:<HexStreamAddress>:<Bits>
			// Where:
			// - <Device> is the device path (E.g. /Devices/ne2k/0)
			// - <Type> is a number (e.g. 4) or symbol (e.g. AF_INET4)
			// - <HexStreamAddress> is a condensed hexadecimal stream (in big endian)
			//      (E.g. 0A000201 for 10.0.2.1 IPv4)
			// - <Bits> is the number of subnet bits (E.g. 24 for an IPv4 Class C)
			// Example: /Devices/ne2k/0:4:0A00020A:24
			//  would define an interface with the address 10.0.2.10/24
			if( Arguments[i][0] == '/' ) {
				// Define Interface
				char	*dev, *type, *addr, *bits;
				
				// Read definition
				dev = Arguments[i];
				type = strchr(dev, ':');
				if( !type ) {
					Log_Warning("IPStack", "<Device>:<Type>:<HexStreamAddress>:<Bits>");
					continue;
				}
				*type = '\0';	type ++;
				
				addr = strchr(type, ':');
				if( !addr ) {
					Log_Warning("IPStack", "<Device>:<Type>:<HexStreamAddress>:<Bits>");
					continue;
				}
				*addr = '\0';	addr ++;
				
				bits = strchr(addr, ':');
				if( !bits ) {
					Log_Warning("IPStack", "<Device>:<Type>:<HexStreamAddress>:<Bits>");
					continue;
				}
				*bits = '\0';	bits ++;
				
				// Define interface
				{
					 int	iType = atoi(type);
					 int	size = IPStack_GetAddressSize(iType);
					Uint8	addrData[size];
					 int	iBits = atoi(bits);
					
					UnHex(addrData, size, addr);
					
					tInterface	*iface = IPStack_AddInterface(dev, "");
					if( !iface ) {
						Log_Warning("IPStack", "Unable to add interface on '%s'", dev);
						continue ;
					}
					iface->Type = iType;
					memcpy(iface->Address, addrData, size);
					iface->SubnetBits = iBits;
					
					// Route for addrData/iBits, no next hop, default metric
					IPStack_AddRoute(iface->Name, iface->Address, iBits, NULL, 0);

					Log_Notice("IPStack", "Boot interface %s/%i on %s",
						IPStack_PrintAddress(iType, addrData), iBits,
						dev);
				}
				
				continue;
			}
			
			// I could also define routes using <Interface>:<HexStreamNetwork>:<Bits>[:<HexStreamGateway>]
			// Example: 1:00000000:0:0A000201
			if( '0' <= Arguments[i][0] && Arguments[i][0] <= '9' )
			{
				// Define Interface
				char	*ifaceName, *network, *bits, *gateway;
				
				// Read definition
				ifaceName = Arguments[i];
				
				network = strchr(ifaceName, ':');
				if( !network ) {
					Log_Warning("IPStack", "<iface>:<HexStreamNetwork>:<Bits>:<HexStreamGateway>");
					continue;
				}
				*network = '\0';	network ++;
				
				bits = strchr(network, ':');
				if( !bits ) {
					Log_Warning("IPStack", "<Device>:<Type>:<HexStreamAddress>:<Bits>");
					continue;
				}
				*bits = '\0';	bits ++;
				
				gateway = strchr(bits, ':');
				if( gateway ) {
					*gateway = '\0';	gateway ++;
				}
				
				// Define route
				{
					tVFS_Node	*node = IPStack_Root_FindDir(NULL, ifaceName);
					if( !node ) {
						Log_Warning("IPStack", "Unknown interface '%s' in arg %i", ifaceName, i);
						continue ;
					}
					tInterface	*iface = node->ImplPtr;
					
					 int	size = IPStack_GetAddressSize(iface->Type);
					Uint8	netData[size];
					Uint8	gwData[size];
					 int	iBits = atoi(bits);
					
					UnHex(netData, size, network);
					if( gateway )
						UnHex(gwData, size, gateway);
					else
						memset(gwData, 0, size);
					
					IPStack_AddRoute(ifaceName, netData, iBits, gwData, 30);
				}
				
				continue;
			}
		}
	}
	
	// Initialise loopback interface
	gIP_LoopInterface.Adapter = IPStack_GetAdapter("LOOPBACK");
	
	DevFS_AddDevice( &gIP_DriverInfo );
	
	return MODULE_ERR_OK;
}

/**
 * \brief Gets the size (in bytes) of a specified form of address
 */
int IPStack_GetAddressSize(int AddressType)
{
	switch(AddressType)
	{
	case -1:	// -1 = maximum
		return sizeof(tIPv6);
	
	case AF_NULL:
		return 0;
	
	case AF_INET4:
		return sizeof(tIPv4);
	case AF_INET6:
		return sizeof(tIPv6);
		
	default:
		return 0;
	}
}

/**
 * \brief Compare two IP Addresses masked by CheckBits
 */
int IPStack_CompareAddress(int AddressType, const void *Address1, const void *Address2, int CheckBits)
{
	 int	size = IPStack_GetAddressSize(AddressType);
	Uint8	mask;
	const Uint8	*addr1 = Address1, *addr2 = Address2;
	
	// Sanity check size
	if( CheckBits < 0 )	CheckBits = size*8;
	if( CheckBits > size*8 )	CheckBits = size*8;
	
	if( CheckBits == 0 )	return 1;	// /0 matches anything
	
	// Check first bits/8 bytes
	if( memcmp(Address1, Address2, CheckBits/8) != 0 )	return 0;
	
	// Check if the mask is a multiple of 8
	if( CheckBits % 8 == 0 )	return 1;
	
	// Check last bits
	mask = 0xFF << (8 - (CheckBits % 8));
	if( (addr1[CheckBits/8] & mask) == (addr2[CheckBits/8] & mask) )
		return 1;
	
	return 0;
}

const char *IPStack_PrintAddress(int AddressType, const void *Address)
{
	switch( AddressType )
	{
	case 4: {
		static char	ret[4*3+3+1];
		const Uint8	*addr = Address;
		sprintf(ret, "%i.%i.%i.%i", addr[0], addr[1], addr[2], addr[3]);
		return ret;
		}
	
	case 6: {	// TODO: address compression
		static char	ret[8*4+7+1];
		const Uint16	*addr = Address;
		sprintf(ret, "%x:%x:%x:%x:%x:%x:%x:%x",
			addr[0], addr[1], addr[2], addr[3],
			addr[4], addr[5], addr[6], addr[7]
			);
		return ret;
		}
	
	default:
		return "";
	}
}

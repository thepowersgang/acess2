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
#include "include/adapters.h"
#include "interface.h"
#include "init.h"

// === IMPORTS ===
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
	.Type = &gIP_RootNodeType
	}
};

// === CODE ===
/**
 * \fn int IPStack_Install(char **Arguments)
 * \brief Intialise the relevant parts of the stack and register with DevFS
 */
int IPStack_Install(char **Arguments)
{
	// TODO: different Layer 2 protocols
	// Layer 3 - Network Layer Protocols
	ARP_Initialise();
	IPv4_Initialise();
	IPv6_Initialise();
	// Layer 4 - Transport Layer Protocols
	TCP_Initialise();
	UDP_Initialise();
	
	// Initialise loopback interface
	gIP_LoopInterface.Adapter = Adapter_GetByName("lo");
	
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

bool IPStack_AddressIsBroadcast(int AddrType, const void *Addr, int SubnetBits)
{
	const size_t	addrsize = IPStack_GetAddressSize(AddrType);
	const Uint8	*addr = Addr;
	
	ASSERTC( SubnetBits, >=, 0 );
	ASSERTC( SubnetBits, <=, addrsize * 8 );
	const size_t	host_bits = addrsize*8 - SubnetBits;
	
	for( int i = 0; i < host_bits/8; i ++ )
	{
		if( addr[addrsize-i-1] != 0xFF )
			return false;
	}
	Uint8	mask = 0xFF >> (8-(host_bits%8));
	if( (addr[addrsize-host_bits/8-1] & mask) != mask )
		return false;
	return true;
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
			ntohs(addr[0]), ntohs(addr[1]), ntohs(addr[2]), ntohs(addr[3]),
			ntohs(addr[4]), ntohs(addr[5]), ntohs(addr[6]), ntohs(addr[7])
			);
		return ret;
		}
	
	default:
		return "";
	}
}

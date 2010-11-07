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

// === PROTOTYPES ===
 int	IPStack_Install(char **Arguments);
 int	IPStack_CompareAddress(int AddressType, void *Address1, void *Address2, int CheckBits);

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
			// Define interfaces by <Device>,<Type>,<HexStreamAddress>,<Bits>
			// Where:
			// - <Device> is the device path (E.g. /Devices/ne2k/0)
			// - <Type> is a number (e.g. 4) or symbol (e.g. AF_INET4)
			// - <HexStreamAddress> is a condensed hexadecimal stream (in big endian)
			//      (E.g. 0A000201 for 10.0.2.1 IPv4)
			// - <Bits> is the number of subnet bits (E.g. 24 for an IPv4 Class C)
		}
	}
	
	gIP_LoopInterface.Adapter = IPStack_GetAdapter("/Devices/fifo/anon");
	
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
	default:
	case AF_NULL:
		return 0;
	case AF_INET4:
		return sizeof(tIPv4);
	case AF_INET6:
		return sizeof(tIPv6);
	}
}

/**
 * \brief Compare two IP Addresses masked by CheckBits
 */
int IPStack_CompareAddress(int AddressType, void *Address1, void *Address2, int CheckBits)
{
	 int	size = IPStack_GetAddressSize(AddressType);
	Uint8	mask;
	Uint8	*addr1 = Address1, *addr2 = Address2;
	
	// Sanity check size
	if( CheckBits < 0 )	CheckBits = 0;
	if( CheckBits > size*8 )	CheckBits = size*8;
	
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

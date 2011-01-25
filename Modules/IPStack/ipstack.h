/*
 * Acess2 IP Stack
 * - Common Header
 */
#ifndef _IPSTACK_H_
#define _IPSTACK_H_

#include <acess.h>
#include <vfs.h>

typedef union uIPv4	tIPv4;
typedef union uIPv6	tIPv6;
typedef struct sMacAddr	tMacAddr;
typedef struct sAdapter	tAdapter;
typedef struct sInterface	tInterface;
typedef struct sSocketFile	tSocketFile;

typedef void	(*tIPCallback)(tInterface *Interface, void *Address, int Length, void *Buffer);

enum eInterfaceTypes {
	AF_NULL,
	AF_INET4 = 4,	// tIPv4
	AF_INET6 = 6	// tIPv6
};

union uIPv4 {
	Uint32	L;
	Uint8	B[4];
} __attribute__((packed));

union uIPv6 {
	Uint16	W[8];
	Uint32	L[4];
	Uint8	B[16];
} __attribute__((packed));

struct sMacAddr {
	Uint8	B[6];
} __attribute__((packed));

/**
 * \brief Route definition structure
 */
typedef struct sRoute {
	struct sRoute	*Next;
	
	tVFS_Node	Node;	//!< Node for route manipulation
	
	tInterface	*Interface;	//!< Interface for this route
	 int	AddressType;	//!< 0: Invalid, 4: IPv4, 6: IPv4
	void	*Network;	//!< Network - Pointer to tIPv4/tIPv6/... at end of structure
	 int	SubnetBits;	//!< Number of bits in \a Network that are valid
	void	*NextHop;	//!< Next Hop address - Pointer to tIPv4/tIPv6/... at end of structure
	 int	Metric;	//!< Route priority
}	tRoute;

struct sInterface {
	struct sInterface	*Next;	//!< Next interface in list
	
	tVFS_Node	Node;	//!< Node to use the interface
	
	tAdapter	*Adapter;	//!< Adapter the interface is associated with
	 int	TimeoutDelay;	//!< Time in miliseconds before a packet times out
	 int	Type;	//!< Interface type, see ::eInterfaceTypes
	
	void	*Address;	//!< IP address (stored after the Name)
	 int	SubnetBits;	//!< Number of bits that denote the address network
	
	tRoute	Route;	//!< Interface route
	
	char	Name[];
};

/**
 * \brief Represents a network adapter
 */
struct sAdapter {
	struct sAdapter	*Next;
	
	 int	DeviceFD;	//!< File descriptor of the device
	 int	NRef;	//!< Number of times it's been referenced
	
	tMacAddr	MacAddr;	//!< Physical address of the adapter
	 int	DeviceLen;	//!< Device name length
	char	Device[];	//!< Device name
};

/**
 * \brief Describes a socket file definition
 */
struct sSocketFile
{
	struct sSocketFile	*Next;
	const char	*Name;
	
	tVFS_Node	*(*Init)(tInterface *Interface);
};

static const tMacAddr cMAC_BROADCAST = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};

#define MAC_SET(t,v)	memcpy(&(t),&(v),sizeof(tMacAddr))
#define IP4_SET(t,v)	(t).L = (v).L;
#define IP6_SET(t,v)	memcpy(&(t),&(v),sizeof(tIPv6))

#define MAC_EQU(a,b)	(memcmp(&(a),&(b),sizeof(tMacAddr))==0)
#define IP4_EQU(a,b)	((a).L==(b).L)
#define IP6_EQU(a,b)	(memcmp(&(a),&(b),sizeof(tIPv6))==0)

// === FUNCTIONS ===
#define htonb(v)	(v)
#define htons(v)	BigEndian16(v)
#define htonl(v)	BigEndian32(v)
#define ntonb(v)	(v)
#define ntohs(v)	BigEndian16(v)
#define ntohl(v)	BigEndian32(v)

extern int	IPStack_AddFile(tSocketFile *File);
extern int	IPStack_GetAddressSize(int AddressType);
extern int	IPStack_CompareAddress(int AddressType, void *Address1, void *Address2, int CheckBits);
extern const char	*IPStack_PrintAddress(int AddressType, void *Address);

extern tRoute	*IPStack_FindRoute(int AddressType, tInterface *Interface, void *Address);

#endif

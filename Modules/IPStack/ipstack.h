/*
 * Acess2 IP Stack
 * - Common Header
 */
#ifndef _IPSTACK_H_
#define _IPSTACK_H_

#include <common.h>
#include <vfs.h>

typedef union uIPv4	tIPv4;
typedef union uIPv6	tIPv6;
typedef struct sMacAddr	tMacAddr;
typedef struct sAdapter	tAdapter;
typedef struct sInterface	tInterface;

typedef void	(*tIPCallback)(tInterface *Interface, void *Address, int Length, void *Buffer);

union uIPv4 {
	Uint32	L;
	Uint8	B[4];
};

union uIPv6 {
	Uint32	L[4];
	Uint8	B[16];
};

struct sMacAddr {
	Uint8	B[6];
};

struct sInterface {
	struct sInterface	*Next;
	tVFS_Node	Node;
	tAdapter	*Adapter;
	 int	Type;	// 4 for IPv4 and 6 for IPv6
	union {
		struct	{
			tIPv6	Address;
			 int	SubnetBits;
		}	IP6;
		
		struct {
			tIPv4	Address;
			tIPv4	Gateway;
			 int	SubnetBits;
		}	IP4;
	};
};

/**
 * \brief Represents a network adapter
 */
struct sAdapter {
	struct sAdapter	*Next;
	char	*Device;
	 int	DeviceFD;
	
	tMacAddr	MacAddr;
	
	tInterface	*Interfaces;
};

static const tMacAddr cMAC_BROADCAST = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};

#define MAC_SET(t,v)	memcpy(&(t),&(v),sizeof(tMacAddr))
#define IP4_SET(t,v)	(t).L = (v).L;
#define IP6_SET(t,v)	memcpy(&(t),&(v),sizeof(tIPv6))

#define MAC_EQU(a,b)	memcmp(&(a),&(b),sizeof(tMacAddr))
#define IP4_EQU(a,b)	((a).L==(b).L)
#define IP6_EQU(a,b)	memcmp(&(a),&(b),sizeof(tIPv6))

// === FUNCTIONS ===
#define htonb(v)	(v)
#define htons(in)	BigEndian16(in)
#define htonl(in)	BigEndian32(in)
#define ntonb(v)	(v)
#define ntohs(in)	BigEndian16(in)
#define ntohl(in)	BigEndian16(in)

#endif

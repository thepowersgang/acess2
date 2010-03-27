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

union uIPv4 {
	Uint32	L;
	Uint8	B[4];
} __attribute__((packed));

union uIPv6 {
	Uint32	L[4];
	Uint8	B[16];
} __attribute__((packed));

struct sMacAddr {
	Uint8	B[6];
} __attribute__((packed));

struct sInterface {
	struct sInterface	*Next;
	tVFS_Node	Node;
	tAdapter	*Adapter;
	 int	TimeoutDelay;	// Time in miliseconds before a connection times out
	 int	Type;	// 0 for disabled, 4 for IPv4 and 6 for IPv6
	union {
		struct	{
			tIPv6	Address;
			 int	SubnetBits;	//Should this be outside the union?
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
	
	 int	DeviceFD;
	 int	NRef;
	
	tMacAddr	MacAddr;
	 int	DeviceLen;
	char	Device[];
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

#define MAC_EQU(a,b)	memcmp(&(a),&(b),sizeof(tMacAddr))
#define IP4_EQU(a,b)	((a).L==(b).L)
#define IP6_EQU(a,b)	memcmp(&(a),&(b),sizeof(tIPv6))

// === FUNCTIONS ===
#define htonb(v)	(v)
#define htons(in)	BigEndian16(in)
#define htonl(in)	BigEndian32(in)
#define ntonb(v)	(v)
#define ntohs(in)	BigEndian16(in)
#define ntohl(in)	BigEndian32(in)

extern int	IPStack_AddFile(tSocketFile *File);

#endif

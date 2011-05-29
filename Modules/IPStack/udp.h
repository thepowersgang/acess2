/*
 * Acess2 IP Stack
 * - UDP Definitions
 */
#ifndef _UDP_H_
#define _UDP_H_

#include "ipstack.h"
#include "ipv4.h"

typedef struct sUDPHeader	tUDPHeader;
typedef struct sUDPEndpoint	tUDPEndpoint;
typedef struct sUDPPacket	tUDPPacket;
typedef struct sUDPChannel	tUDPChannel;

struct sUDPHeader
{
	Uint16	SourcePort;
	Uint16	DestPort;
	Uint16	Length;
	Uint16	Checksum;
	Uint8	Data[];
};

struct sUDPEndpoint
{
	Uint16	Port;
	Uint16	AddrType;
	union {
		tIPv4	v4;
		tIPv6	v6;
	}	Addr;
};

struct sUDPPacket
{
	struct sUDPPacket	*Next;
	tUDPEndpoint	Remote;
	size_t	Length;
	Uint8	Data[];
};

struct sUDPChannel
{
	struct sUDPChannel	*Next;
	tInterface	*Interface;
	Uint16	LocalPort;

	tUDPEndpoint	Remote;	// Only accept packets form this address/port pair
	 int	RemoteMask;	// Mask on the address
	
	tVFS_Node	Node;
	tShortSpinlock	lQueue;
	tUDPPacket	* volatile Queue;
	tUDPPacket	*QueueEnd;
};

#endif


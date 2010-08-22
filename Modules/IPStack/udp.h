/*
 * Acess2 IP Stack
 * - UDP Definitions
 */
#ifndef _UDP_H_
#define _UDP_H_

#include "ipstack.h"
#include "ipv4.h"

typedef struct sUDPHeader	tUDPHeader;
typedef struct sUDPServer	tUDPServer;
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

struct sUDPPacket
{
	struct sUDPPacket	*Next;
	size_t	Length;
	Uint8	Data[];
};

struct sUDPChannel
{
	struct sUDPChannel	*Next;
	tInterface	*Interface;
	Uint16	LocalPort;
	union {
		tIPv4	v4;
		tIPv6	v6;
	}	RemoteAddr;
	Uint16	RemotePort;
	tVFS_Node	Node;
	tShortSpinlock	lQueue;
	tUDPPacket	* volatile Queue;
	tUDPPacket	*QueueEnd;
};

struct sUDPServer
{
	struct sUDPServer	*Next;
	
	tVFS_Node	Node;
	
	tInterface	*Interface;
	Uint16	ListenPort;
	 int	NextID;
	 int	NumChannels;
	tUDPChannel	*Channels;
	tMutex	Lock;
	tUDPChannel	* volatile NewChannels;
};

#endif

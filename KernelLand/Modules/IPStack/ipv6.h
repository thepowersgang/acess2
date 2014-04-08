/*
 * Acess2 IP Stack
 * - IPv6 Definitions
 */
#ifndef _IPV6_H_
#define _IPV6_H_

#include "ipstack.h"
#include "include/buffer.h"

typedef struct sIPv6Header	tIPv6Header;

struct sIPv6Header
{
	#if 0
	// High 4: Version
	// Next 8: Traffic Class
	// Low 20: Flow Label
	Uint32	Head;
	#else
	union {
		Uint32	Head;	// Allow a ntohl to happen
		struct {
			unsigned Version:	4;
			unsigned TrafficClass:	8;
			unsigned FlowLabel:	20;
		} PACKED;
	} PACKED;
	#endif
	Uint16	PayloadLength;
	Uint8	NextHeader;	// Type of payload data
	Uint8	HopLimit;
	tIPv6	Source;
	tIPv6	Destination;
	char	Data[];
};

#define IPV6_ETHERNET_ID	0x86DD

extern int	IPv6_RegisterCallback(int ID, tIPCallback Callback);
extern int	IPv6_SendPacket(tInterface *Iface, tIPv6 Destination, int Protocol, tIPStackBuffer *Buffer);

#endif

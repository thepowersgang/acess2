/*
 * Acess2 IP Stack
 * - IPv6 Definitions
 */
#ifndef _IPV6_H_
#define _IPV6_H_

#include "ipstack.h"

typedef struct sIPv6Header	tIPv6Header;

struct sIPv6Header
{
	#if 1
	// High 4: Version
	// Next: Traffic Class
	// Low 20: Flow Label
	Uint32	Head;
	#else
	struct {
		unsigned Version:	4;
		unsigned TrafficClass:	8;
		unsigned FlowLabel:	20;
	}	__attribute__((packed));
	#endif
	Uint16	PayloadLength;
	Uint8	NextHeader;	// Type of payload data
	Uint8	HopLimit;
	tIPv6	Source;
	tIPv6	Destination;
};

#define IPV6_ETHERNET_ID	0x86DD

#endif

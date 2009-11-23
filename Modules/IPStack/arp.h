/*
 * Acess2 IP Stack
 * - Common Header
 */
#ifndef _ARP_H_
#define _ARP_H_

#include "ipstack.h"

typedef struct sArpRequest4	tArpRequest4;
typedef struct sArpRequest6	tArpRequest6;

struct sArpRequest4 {
	Uint16	HWType;
	Uint16	Type;
	Uint8	HWSize, SWSize;
	Uint16	Request;
	tMacAddr	SourceMac;
	tIPv4	SourceIP;
	tMacAddr	DestMac;
	tIPv4	DestIP;
} __attribute__((packed));

struct sArpRequest6 {
	Uint16	HWType;
	Uint16	Type;
	Uint8	HWSize, SWSize;
	Uint16	Request;
	tMacAddr	SourceMac;
	tIPv6	SourceIP;
	tMacAddr	DestMac;
	tIPv6	DestIP;
} __attribute__((packed));

#endif

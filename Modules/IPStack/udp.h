/*
 * Acess2 IP Stack
 * - UDP Definitions
 */
#ifndef _UDP_H_
#define _UDP_H_

#include "ipstack.h"
#include "ipv4.h"

typedef struct sUDPHeader	tUDPHeader;

struct sUDPHeader
{
	Uint16	SourcePort;
	Uint16	DestPort;
	Uint16	Length;
	Uint16	Checksum;
	Uint8	Data[];
};

#endif

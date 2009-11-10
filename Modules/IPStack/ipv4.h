/*
 * Acess2 IP Stack
 * - IPv4 Definitions
 */
#ifndef _IPV4_H_
#define _IPV4_H_

#include "ipstack.h"

typedef struct sIPv4Header	tIPv4Header;

struct sIPv4Header
{
	struct {
		unsigned Version:	4;	// = 4
		unsigned HeaderLength:	4;	// in 4-byte chunks
	};
	Uint8	DiffServices;	// Differentiated Services
	Uint16	TotalLength;
	Uint16	Identifcation;
	
	struct {
		unsigned Reserved:	1;
		unsigned DontFragment:	1;
		unsigned MoreFragments:	1;
		unsigned FragOffLow:	5;
	};
	Uint8	FragOffHi;	// Number of 8-byte blocks from the original start
	
	Uint8	TTL;	// Max number of hops effectively
	Uint8	Protocol;
	Uint16	HeaderChecksum;	// One's Complement Sum of the entire header must equal zero
	
	tIPv4	Source;
	tIPv4	Destination;
	
	Uint8	Options[];
};

#define IP4PROT_ICMP	1
#define IP4PROT_TCP	6
#define IP4PROT_UDP	17

#define IPV4_ETHERNET_ID	0x0800

#endif

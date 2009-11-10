/*
 * Acess2 IP Stack
 * - ICMP Handling
 */
#ifndef _ICMP_H_
#define _ICMP_H_

// === TYPEDEFS ===
typedef struct sICMPHeader	tICMPHeader;

// === STRUCTURES ===
struct sICMPHeader
{
	Uint8	Type;
	Uint8	Code;
	Uint16	Checksum;
	Uint16	ID;
	Uint16	Sequence;
};

#endif

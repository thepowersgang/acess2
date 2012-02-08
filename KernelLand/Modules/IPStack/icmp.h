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
	Uint8	Data[];
};

// === CONSTANTS ===
enum eICMPTypes
{
	ICMP_ECHOREPLY = 0,
	ICMP_UNREACHABLE = 3,
	ICMP_QUENCH = 4,
	ICMP_REDIRECT = 5,
	ICMP_ALTADDR = 6,
	ICMP_ECHOREQ = 8,
	ICMP_TRACE = 30	// Information Request
};

#endif

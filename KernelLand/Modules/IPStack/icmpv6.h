/*
 * Acess2 IP Stack
 * - By John Hodge (thePowersGang)
 *
 * icmpv6.c
 * - Internet Control Message Protocol v6
 */
#ifndef _ICMPV6_H_
#define _ICMPV6_H_

#define IPV6PROT_ICMPV6	58

typedef struct {
	Uint8	Type;
	Uint8	Code;
	Uint16	Checksum;
} PACKED tICMPv6_Header;

typedef struct {
	Uint32	Reserved;
} PACKED tICMPv6_RS;

typedef struct {
	Uint8	CurHopLimit;
	Uint8	Flags;	// M:O:6
	Uint16	RouterLifetime;	// Seconds, life time as a default router (wtf does that mean?)
	Uint32	ReachableTime;
	Uint32	RetransTimer;	// Miliseconds, time between transmissions of RAs from this router
	Uint8	Options[0];
} PACKED tICMPv6_RA;

typedef struct {
	Uint32	Reserved;
	tIPv6	TargetAddress;
	Uint8	Options[0];
} PACKED tICMPv6_NS;

typedef struct {
	Uint32	Flags;
	tIPv6	TargetAddress;
	Uint8	Options[0];
} PACKED tICMPv6_NA;

typedef struct {
	Uint32	Reserved;
	tIPv6	TargetAddress;
	tIPv6	DestinationAddress;
	Uint8	Options[0];
} PACKED tICMPv6_Redirect;

typedef struct {
	Uint8	Type;	// 1,2
	Uint8	Length;	// Length of field in units of 8 bytes (incl header), typically 1
	Uint8	Address[0];
} PACKED tICMPv6_Opt_LinkAddr;

typedef struct {
	Uint8	Type;	// 3
	Uint8	Length;	// Length of field in units of 8 bytes (incl header), typically
	Uint8	PrefixLength;
	Uint8	Flags;	// L:A:6 - 
	Uint32	ValidLifetime;
	Uint32	PreferredLifetime;
	Uint32	_reserved2;
	tIPv6	Prefix[0];
} PACKED tICMPv6_Opt_Prefix;

typedef struct {
	Uint8	Type;	// 4
	Uint8	Length;
	Uint16	_rsvd1;
	Uint32	_rsvd2;
	Uint8	Data[0];	// All or part of the redirected message (not exceeding MTU)
} PACKED tICMPv6_Opt_Redirect;

typedef struct {
	Uint8	Type;	// 4
	Uint8	Length;
	Uint16	_resvd1;
	Uint32	MTU;
} PACKED tICMPv6_Opt_MTU;

enum {
	ICMPV6_ERR_UNREACHABLE,
	ICMPV6_ERR_PACKET_TOO_BIG,
	ICMPV6_ERR_TIME_EXCEEDED,
	ICMPV6_ERR_PARAMETER_PROBLEM,
};
enum {
	ICMPV6_INFO_ECHO_REQUEST = 128,
	ICMPV6_INFO_ECHO_REPLY,

	ICMPV6_INFO_ROUTER_SOLICITATION = 133,
	ICMPV6_INFO_ROUTER_ADVERTISEMENT,
	ICMPV6_INFO_NEIGHBOUR_SOLICITATION,
	ICMPV6_INFO_NEIGHBOUR_ADVERTISMENT,
	ICMPV6_INFO_REDIRECT,
};

enum {
	ICMPV6_OPTION_SRCLINK = 1,
	ICMPV6_OPTION_TGTLINK,
	ICMPV6_OPTION_PREFIX,
	ICMPV6_OPTION_REDIRECTED_HDR,
	ICMPV6_OPTION_MTU,
};

#endif


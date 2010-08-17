/*
 * Acess2 IP Stack
 * - Firewall Rules
 */
#include "ipstack.h"

// === IMPORTS ===

// === PROTOTYPES ===
 int	IPTablesV4_TestChain(
	const char *RuleName,
	const tIPv4 *Src, const tIPv4 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);
 int	IPTablesV6_TestChain(
	const char *RuleName,
	const tIPv6 *Src, const tIPv6 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);

// === GLOBALS ===

// === CODE ===
/**
 * \brief Tests an IPv4 chain on a packet
 * \return Boolean Disallow (0: Packet Allowed, 1: Drop, 2: Reject)
 */
int IPTablesV4_TestChain(
	const char *RuleName,
	const tIPv4 *Src, const tIPv4 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	)
{
	return 0;	// Accept all for now
}

/**
 * \brief Tests an IPv6 chain on a packet
 * \return Boolean Disallow (0: Packet Allowed, 1: Drop, 2: Reject)
 */
int IPTablesV6_TestChain(
	const char *RuleName,
	const tIPv6 *Src, const tIPv6 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	)
{
	return 0;	// Accept all for now
}


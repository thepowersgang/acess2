/*
 */
#ifndef _FIREWALL_H_
#define _FIREWALL_H_

/**
 * \brief Tests a packet on a chain
 */
extern int	IPTablesV4_TestChain(
	const char *RuleName,
	const tIPv4 *Src, const tIPv4 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);

extern int	IPTablesV6_TestChain(
	const char *RuleName,
	const tIPv6 *Src, const tIPv6 *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);

#endif

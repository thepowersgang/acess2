/*
 */
#ifndef _FIREWALL_H_
#define _FIREWALL_H_

enum eFirewallActions
{
	FIREWALL_ACCEPT,
	FIREWALL_DROP
};

/**
 * \brief Tests a packet on a chain
 */
extern int	IPTables_TestChain(
	const char *RuleName,
	const int AddressType,
	const void *Src, const void *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);

#endif

/*
 * Acess2 IP Stack
 * - Firewall Rules
 */
#include "ipstack.h"
#include "firewall.h"

// === IMPORTS ===

// === TYPES ===
typedef struct sFirewallMod	tFirewallMod;
typedef struct sModuleRule	tModuleRule;
typedef struct sRule	tRule;
typedef struct sChain	tChain;

struct sModuleRule
{
	tModuleRule	*Next;
	
	tFirewallMod	*Mod;
	
	char	Data[];
};

struct sRule
{
	tRule	*Next;
	
	 int	PacketCount;	// Number of packets seen
	 int	ByteCount;		// Number of bytes seen (IP Payload bytes)
	
	 int	bInvertSource;
	void	*Source;
	 int	SourceMask;
	 
	 int	bInvertDest;
	void	*Dest;
	 int	DestMask;
	
	tModuleRule	*Modules;
	
	char	Action[];	// Target rule name
};

struct sChain
{
	tChain	*Next;
	
	tRule	*FirstRule;
	tRule	*LastRule;
	
	char	Name[];
};

// === PROTOTYPES ===
 int	IPTables_TestChain(
	const char *RuleName,
	const int AddressType,
	const void *Src, const void *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	);

// === GLOBALS ===
tChain	*gapFirewall_Chains[10];
tChain	gFirewall_DROP = {.Name="DROP"};
tChain	gFirewall_ACCEPT = {.Name="ACCEPT"};
tChain	gFirewall_RETURN = {.Name="RETURN"};

// === CODE ===
int IPTables_DoRule(
	tRule *Rule, int AddrType,
	const void *Src, const void *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data)
{
	return 0;
}

/**
 * \brief Tests an IPv4 chain on a packet
 * \return Boolean Disallow (0: Packet Allowed, 1: Drop, 2: Reject, 3: Continue)
 */
int IPTables_TestChain(
	const char *RuleName,
	const int AddressType,
	const void *Src, const void *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data
	)
{
	 int	rv;
	tChain	*chain;
	tRule	*rule;
	
	for( chain = gapFirewall_Chains[AddressType]; chain; chain = chain->Next )
	{
		if( strcmp(chain->Name, RuleName) == 0 )
			break;
	}
	if( !chain )	return -1;	// Bad rule name
	
	// Check the rules
	for( rule = chain->FirstRule; rule; rule = rule->Next )
	{
		rv = IPTables_DoRule(rule, AddressType, Src, Dest, Type, Flags, Length, Data);
		if( rv == -1 )
			continue ;
		
		return rv;
	}
	
	
	return 0;	// Accept all for now
}

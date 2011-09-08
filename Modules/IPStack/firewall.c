/*
 * Acess2 IP Stack
 * - Firewall Rules
 */
#include "ipstack.h"
#include "firewall.h"

#define MAX_ADDRTYPE	9

// === IMPORTS ===

// === TYPES ===
typedef struct sKeyValue	tKeyValue;
typedef struct sFirewallMod	tFirewallMod;
typedef struct sModuleRule	tModuleRule;
typedef struct sRule	tRule;
typedef struct sChain	tChain;

// === STRUCTURES ===
struct sKeyValue
{
	const char	*Key;
	const char	*Value;
};

struct sFirewallMod
{
	const char	*Name;
	
	 int	(*Match)(tModuleRule *Rule, int AddrType,
			const void *Src, const void *Dest,
			Uint8 Type, Uint32 Flags,
			size_t Length, const void *Data);
	
	tModuleRule	*(*Create)(tKeyValue *Params);
};

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
	 int	ByteCount;  	// Number of bytes seen (IP Payload bytes)
	
	 int	bInvertSource;	// Boolean NOT flag on source
	void	*Source;	// Source address bytes
	 int	SourceMask;	// Source address mask bits
	 
	 int	bInvertDest;	// Boolean NOT flag on destination
	void	*Dest;  	// Destination address bytes
	 int	DestMask;	// Destination address mask bits
	
	tModuleRule	*Modules;	// Modules loaded for this rule
	
	char	Target[];	// Target rule name
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
tChain	*gapFirewall_Chains[MAX_ADDRTYPE+1];
tChain	gFirewall_DROP = {.Name="DROP"};
tChain	gFirewall_ACCEPT = {.Name="ACCEPT"};
tChain	gFirewall_RETURN = {.Name="RETURN"};

// === CODE ===
/**
 * \brief Apply a rule to a packet
 * \return -1 for no match, -2 for RETURN, eFirewallAction otherwise
 */
int IPTables_DoRule(
	tRule *Rule, int AddrType,
	const void *Src, const void *Dest,
	Uint8 Type, Uint32 Flags,
	size_t Length, const void *Data)
{
	 int	rv;
	// Check if source doesn't match
	if( !IPStack_CompareAddress(AddrType, Src, Rule->Source, Rule->SourceMask) == !Rule->bInvertSource )
		return -1;
	// Check if destination doesn't match
	if( !IPStack_CompareAddress(AddrType, Dest, Rule->Dest, Rule->DestMask) == !Rule->bInvertDest )
		return -1;
	
	// TODO: Handle modules (UDP/TCP/etc)
	tModuleRule *modrule;
	for( modrule = Rule->Modules; modrule; modrule = modrule->Next )
	{
		if( !modrule->Mod->Match )	continue;
		rv = modrule->Mod->Match(modrule, AddrType, Src, Dest, Type, Flags, Length, Data);
		if(rv != 0)	return rv;	// No match / action
	}
	
	// Update statistics
	Rule->PacketCount ++;
	Rule->ByteCount += Length;
	
	return IPTables_TestChain(Rule->Target, AddrType, Src, Dest, Type, Flags, Length, Data);
}

/**
 * \brief Tests an IPv4 chain on a packet
 * \return Boolean Disallow (0: Packet Allowed, 1: Drop, 2: Reject, 3: Continue, -1 no match)
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
	
	if( AddressType >= MAX_ADDRTYPE )	return -1;	// Bad address type
	
	// Catch builtin targets
	if(strcmp(RuleName, "") == 0)	return -1;	// No action
	if(strcmp(RuleName, "ACCEPT") == 0)	return 0;	// Accept packet
	if(strcmp(RuleName, "DROP") == 0)	return 1;	// Drop packet
	if(strcmp(RuleName, "RETURN") == 0)	return -2;	// Return from rule
	
	// Find the rule
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
		if( rv == -2 )	// -2 = Return from a chain/table, pretend no match
			return -1;
		
		return rv;
	}
	
	
	return 0;	// Accept all for now
}

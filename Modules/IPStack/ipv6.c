/*
 * Acess2 IP Stack
 * - IPv6 Protcol Handling
 */
#include "ipstack.h"
#include "link.h"
#include "ipv6.h"

// === IMPORTS ===
extern Uint32	IPv4_Netmask(int FixedBits);

// === PROTOTYPES ===
 int	IPv6_Initialise();
void	IPv6_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer);
tInterface	*IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast);

// === CODE ===
/**
 * \fn int IPv6_Initialise()
 */
int IPv6_Initialise()
{
	Link_RegisterType(IPV6_ETHERNET_ID, IPv6_int_GetPacket);
	return 1;
}

/**
 * \fn void IPv6_int_GetPacket(tInterface *Interface, tMacAddr From, int Length, void *Buffer)
 * \brief Process an IPv6 Packet
 */
void IPv6_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer)
{
	tIPv6Header	*hdr = Buffer;
	if(Length < sizeof(tIPv6Header))	return;
	
	if(hdr->Version != 6)	return;
}

/**
 * \fn tInterface *IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address)
 * \brief Searches an adapter for a matching address
 */
tInterface *IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast)
{
	 int	i, j;
	tInterface	*iface = NULL;
	Uint32	netmask;
	
	for( iface = Adapter->Interfaces; iface; iface = iface->Next)
	{
		// Skip non-IPv6 Interfaces
		if( iface->Type != 6 )	continue;
		
		// If the address is a perfect match, return this interface
		if( IP6_EQU(Address, iface->IP6.Address) )	return iface;
		
		// Check if we want to match broadcast addresses
		if( !Broadcast )	continue;
		
		// Check for broadcast
		if( iface->IP6.SubnetBits > 32 && Address.L[0] != iface->IP6.Address.L[0] )
			continue;
		if( iface->IP6.SubnetBits > 64 && Address.L[1] != iface->IP6.Address.L[1] )
			continue;
		if( iface->IP6.SubnetBits > 96 && Address.L[2] != iface->IP6.Address.L[2] )
			continue;
		
		j = iface->IP6.SubnetBits / 32;
		i = iface->IP6.SubnetBits % 32;
		netmask = IPv4_Netmask( iface->IP6.SubnetBits % 32 );
		
		// Check the last bit of the netmask
		if( (Address.L[j] >> i) != (iface->IP6.Address.L[j] >> i) )	continue;
		
		// Check that the host portion is one
		if( (Address.L[j] & ~netmask) != (0xFFFFFFFF & ~netmask) )	continue;
		if( j >= 2 && Address.L[3] != 0xFFFFFFFF)	continue;
		if( j >= 1 && Address.L[2] != 0xFFFFFFFF)	continue;
		if( j >= 0 && Address.L[1] != 0xFFFFFFFF)	continue;
		
		return iface;
	}
	return NULL;
}

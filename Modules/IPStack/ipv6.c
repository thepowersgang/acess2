/*
 * Acess2 IP Stack
 * - IPv6 Protcol Handling
 */
#include "ipstack.h"
#include "link.h"
#include "ipv6.h"

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;
extern Uint32	IPv4_Netmask(int FixedBits);

// === PROTOTYPES ===
 int	IPv6_Initialise();
void	IPv6_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer);
tInterface	*IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast);

// === CODE ===
/**
 * \brief Initialise the IPv6 handling code
 */
int IPv6_Initialise()
{
	Link_RegisterType(IPV6_ETHERNET_ID, IPv6_int_GetPacket);
	return 1;
}

/**
 * \fn void IPv6_int_GetPacket(tInterface *Interface, tMacAddr From, int Length, void *Buffer)
 * \brief Process an IPv6 Packet
 * \param Interface	Input interface
 * \param From	Source MAC address
 * \param Length	Packet length
 * \param Buffer	Packet data
 */
void IPv6_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer)
{
	tIPv6Header	*hdr = Buffer;
	if(Length < sizeof(tIPv6Header))	return;
	
	hdr->Head = ntohl(hdr->Head);
	
	//if( ((hdr->Head >> (20+8)) & 0xF) != 6 )
	if( hdr->Version != 6 )
		return;
	
	Log_Debug("IPv6", "hdr = {");
	//Log_Debug("IPv6", " .Version       = %i", (hdr->Head >> (20+8)) & 0xF );
	//Log_Debug("IPv6", " .TrafficClass  = %i", (hdr->Head >> (20)) & 0xFF );
	//Log_Debug("IPv6", " .FlowLabel     = %i", hdr->Head & 0xFFFFF );
	Log_Debug("IPv6", " .Version       = %i", hdr->Version );
	Log_Debug("IPv6", " .TrafficClass  = %i", hdr->TrafficClass );
	Log_Debug("IPv6", " .FlowLabel     = %i", hdr->FlowLabel );
	Log_Debug("IPv6", " .PayloadLength = 0x%04x", ntohs(hdr->PayloadLength) );
	Log_Debug("IPv6", " .NextHeader    = 0x%02x", hdr->NextHeader );
	Log_Debug("IPv6", " .HopLimit      = 0x%02x", hdr->HopLimit );
	Log_Debug("IPv6", " .Source        = %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", hdr->Source );
	Log_Debug("IPv6", " .Destination   = %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", hdr->Destination );
	Log_Debug("IPv6", "}");
	
}

/**
 * \fn tInterface *IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address)
 * \brief Searches an adapter for a matching address
 * \param Adapter	Source adapter
 * \param Address	Destination Address
 * \param Broadcast	Allow broadcast?
 */
tInterface *IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast)
{
	 int	i, j;
	tInterface	*iface = NULL;
	Uint32	netmask;
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next)
	{
		// Check for this adapter
		if( iface->Adapter != Adapter )	continue;
		
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

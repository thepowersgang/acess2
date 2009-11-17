/*
 * Acess2 IP Stack
 * - IPv4 Protcol Handling
 */
#include "ipstack.h"
#include "link.h"
#include "ipv4.h"

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;

// === PROTOTYPES ===
 int	IPv4_Initialise();
void	IPv4_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer);
tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
Uint32	IPv4_Netmask(int FixedBits);

// === GLOBALS ===
tIPCallback	gaIPv4_Callbacks[256];

// === CODE ===
/**
 * \fn int IPv4_Initialise()
 */
int IPv4_Initialise()
{
	Link_RegisterType(IPV4_ETHERNET_ID, IPv4_int_GetPacket);
	return 1;
}

/**
 * \fn void IPv4_int_GetPacket(tInterface *Adapter, tMacAddr From, int Length, void *Buffer)
 * \brief Process an IPv4 Packet
 */
void IPv4_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer)
{
	tIPv4Header	*hdr = Buffer;
	tInterface	*iface;
	Uint8	*data;
	 int	dataLength;
	if(Length < sizeof(tIPv4Header))	return;
	
	// Check that the version IS IPv4
	if(hdr->Version != 4)	return;
	
	// Check Header checksum
	//TODO
	
	// Check Packet length
	if(hdr->TotalLength > Length)	return;
	
	// Get Interface (allowing broadcasts)
	iface = IPv4_GetInterface(Adapter, hdr->Source, 1);
	if(!iface)	return;	// Not for us? Well, let's ignore it
	
	// Defragment
	//TODO
	
	dataLength = hdr->TotalLength - sizeof(tIPv4Header);
	data = &hdr->Options[0];
	
	// Send it on
	gaIPv4_Callbacks[hdr->Protocol] (iface, &hdr->Source, dataLength, data);
}

/**
 * \fn tInterface *IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address)
 * \brief Searches an adapter for a matching address
 */
tInterface *IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast)
{
	tInterface	*iface = NULL;
	Uint32	netmask;
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next)
	{
		if( iface->Adapter != Adapter )	continue;
		if( iface->Type != 4 )	continue;
		if( IP4_EQU(Address, iface->IP4.Address) )
			return iface;
		
		if( !Broadcast )	continue;
		
		// Check for broadcast
		netmask = IPv4_Netmask(iface->IP4.SubnetBits);
		if( (Address.L & netmask) == (iface->IP4.Address.L & netmask)
		 && (Address.L & ~netmask) == (0xFFFFFFFF & ~netmask) )
			return iface;
	}
	return NULL;
}

/**
 * \brief Convert a network prefix to a netmask
 * 
 * For example /24 will become 255.255.255.0
 */
Uint32 IPv4_Netmask(int FixedBits)
{
	Uint32	ret = 0xFFFFFFFF;
	ret >>= (32-FixedBits);
	ret <<= (32-FixedBits);
	return ret;
}

/*
 * Acess2 IP Stack
 * - IPv4 Protcol Handling
 */
#include "ipstack.h"
#include "link.h"
#include "ipv4.h"

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;
extern void	ICMP_Initialise();
extern void	UDP_Initialise();

// === PROTOTYPES ===
 int	IPv4_Initialise();
 int	IPv4_RegisterCallback(int ID, tIPCallback Callback);
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
	ICMP_Initialise();
	UDP_Initialise();
	Link_RegisterType(IPV4_ETHERNET_ID, IPv4_int_GetPacket);
	return 1;
}

/**
 * \fn int IPv4_RegisterCallback( int ID, tIPCallback Callback )
 * \brief Registers a callback
 */
int IPv4_RegisterCallback(int ID, tIPCallback Callback)
{
	if( ID < 0 || ID > 255 )	return 0;
	if( gaIPv4_Callbacks[ID] )	return 0;
	gaIPv4_Callbacks[ID] = Callback;
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
	
	//Log("[IPv4 ] Version = %i", hdr->Version);
	Log("[IPv4 ] HeaderLength = %i", hdr->HeaderLength);
	Log("[IPv4 ] DiffServices = %i", hdr->DiffServices);
	//Log("[IPv4 ] TotalLength = %i", ntohs(hdr->TotalLength) );
	//Log("[IPv4 ] Identifcation = %i", ntohs(hdr->Identifcation) );
	//Log("[IPv4 ] TTL = %i", hdr->TTL );
	Log("[IPv4 ] Protocol = %i", hdr->Protocol );
	//Log("[IPv4 ] HeaderChecksum = 0x%x", ntohs(hdr->HeaderChecksum) );
	Log("[IPv4 ] Source = %i.%i.%i.%i",
		hdr->Source.B[0], hdr->Source.B[1], hdr->Source.B[2], hdr->Source.B[3] );
	Log("[IPv4 ] Destination = %i.%i.%i.%i",
		hdr->Destination.B[0], hdr->Destination.B[1],
		hdr->Destination.B[2], hdr->Destination.B[3] );
	
	// Check that the version IS IPv4
	if(hdr->Version != 4) {
		Log("[IPv4 ] hdr->Version(%i) != 4", hdr->Version);
		return;
	}
	
	// Check Header checksum
	//TODO
	
	// Check Packet length
	if( ntohs(hdr->TotalLength) > Length) {
		Log("[IPv4 ] hdr->TotalLength(%i) > Length(%i)", hdr->TotalLength, Length);
		return;
	}
	
	// Get Interface (allowing broadcasts)
	iface = IPv4_GetInterface(Adapter, hdr->Destination, 1);
	if(!iface) {
		Log("[IPv4 ] Ignoring Packet (Not for us)");
		return;	// Not for us? Well, let's ignore it
	}
	
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
	Uint32	addr, this;
	
	addr = ntohl( Address.L );
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next)
	{
		if( iface->Adapter != Adapter )	continue;
		if( iface->Type != 4 )	continue;
		if( IP4_EQU(Address, iface->IP4.Address) )
			return iface;
		
		if( !Broadcast )	continue;
		
		this = ntohl( iface->IP4.Address.L );
		
		// Check for broadcast
		netmask = IPv4_Netmask(iface->IP4.SubnetBits);
		
		//Log("netmask = 0x%08x", netmask);
		//Log("addr = 0x%08x", addr);
		//Log("this = 0x%08x", this);
		//Log("%08x == %08x && %08x == %08x",
		//	(addr & netmask), (this & netmask),
		//	(addr & ~netmask), (0xFFFFFFFF & ~netmask)
		//	);
		if( (addr & netmask) == (this & netmask)
		 && (addr & ~netmask) == (0xFFFFFFFF & ~netmask) )
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
	// Returs a little endian netmask
	return ret;
}

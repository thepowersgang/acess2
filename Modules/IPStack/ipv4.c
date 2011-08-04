/*
 * Acess2 IP Stack
 * - IPv4 Protcol Handling
 */
#include "ipstack.h"
#include "link.h"
#include "ipv4.h"
#include "firewall.h"

#define DEFAULT_TTL	32

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;
extern void	ICMP_Initialise();
extern  int	ICMP_Ping(tInterface *Interface, tIPv4 Addr);
extern tMacAddr	ARP_Resolve4(tInterface *Interface, tIPv4 Address);

// === PROTOTYPES ===
 int	IPv4_Initialise();
 int	IPv4_RegisterCallback(int ID, tIPCallback Callback);
void	IPv4_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer);
tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
Uint32	IPv4_Netmask(int FixedBits);
Uint16	IPv4_Checksum(const Uint16 *Buf, int WordCount);
 int	IPv4_Ping(tInterface *Iface, tIPv4 Addr);

// === GLOBALS ===
tIPCallback	gaIPv4_Callbacks[256];

// === CODE ===
/**
 * \brief Initialise the IPv4 Code
 */
int IPv4_Initialise()
{
	ICMP_Initialise();
	Link_RegisterType(IPV4_ETHERNET_ID, IPv4_int_GetPacket);
	return 1;
}

/**
 * \brief Registers a callback
 * \param ID	8-bit packet type ID
 * \param Callback	Callback function
 */
int IPv4_RegisterCallback(int ID, tIPCallback Callback)
{
	if( ID < 0 || ID > 255 )	return 0;
	if( gaIPv4_Callbacks[ID] )	return 0;
	gaIPv4_Callbacks[ID] = Callback;
	return 1;
}

/**
 * \brief Creates and sends an IPv4 Packet
 * \param Iface	Interface
 * \param Address	Destination IP
 * \param Protocol	Protocol ID
 * \param ID	Some random ID number
 * \param Length	Data Length
 * \param Data	Packet Data
 * \return Boolean Success
 */
int IPv4_SendPacket(tInterface *Iface, tIPv4 Address, int Protocol, int ID, int Length, const void *Data)
{
	tMacAddr	to = ARP_Resolve4(Iface, Address);
	 int	bufSize = sizeof(tIPv4Header) + Length;
	char	buf[bufSize];
	tIPv4Header	*hdr = (void*)buf;
	 int	ret;
	
	if( MAC_EQU(to, cMAC_ZERO) ) {
		// No route to host
		Log_Notice("IPv4", "No route to host %i.%i.%i.%i",
			Address.B[0], Address.B[1], Address.B[2], Address.B[3]);
		return 0;
	}
	
	// OUTPUT Firewall rule go here
	ret = IPTables_TestChain("OUTPUT",
		4, (tIPv4*)Iface->Address, &Address,
		Protocol, 0,
		Length, Data);
	if(ret > 0) {
		// Just drop it (with an error)
		Log_Notice("IPv4", "Firewall dropped packet");
		return 0;
	}
	
	memcpy(&hdr->Options[0], Data, Length);
	hdr->Version = 4;
	hdr->HeaderLength = sizeof(tIPv4Header)/4;
	hdr->DiffServices = 0;	// TODO: Check
	
	hdr->Reserved = 0;
	hdr->DontFragment = 0;
	hdr->MoreFragments = 0;
	hdr->FragOffLow = 0;
	hdr->FragOffHi = 0;
	
	hdr->TotalLength = htons( bufSize );
	hdr->Identifcation = htons( ID );	// TODO: Check
	hdr->TTL = DEFAULT_TTL;
	hdr->Protocol = Protocol;
	hdr->HeaderChecksum = 0;	// Will be set later
	hdr->Source = *(tIPv4*)Iface->Address;
	hdr->Destination = Address;
	hdr->HeaderChecksum = htons(IPv4_Checksum((Uint16*)hdr, sizeof(tIPv4Header)/2));
	
	Log_Log("IPv4", "Sending packet to %i.%i.%i.%i",
		Address.B[0], Address.B[1], Address.B[2], Address.B[3]);
	Link_SendPacket(Iface->Adapter, IPV4_ETHERNET_ID, to, bufSize, buf);
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
	 int	ret;
	
	if(Length < sizeof(tIPv4Header))	return;
	
	#if 0
	//Log_Log("IPv4", "Version = %i", hdr->Version);
	//Log_Log("IPv4", "HeaderLength = %i", hdr->HeaderLength);
	//Log_Log("IPv4", "DiffServices = %i", hdr->DiffServices);
	Log_Debug("IPv4", "TotalLength = %i", ntohs(hdr->TotalLength) );
	//Log_Log("IPv4", "Identifcation = %i", ntohs(hdr->Identifcation) );
	//Log_Log("IPv4", "TTL = %i", hdr->TTL );
	Log_Debug("IPv4", "Protocol = %i", hdr->Protocol );
	//Log_Log("IPv4", "HeaderChecksum = 0x%x", ntohs(hdr->HeaderChecksum) );
	Log_Debug("IPv4", "Source = %i.%i.%i.%i",
		hdr->Source.B[0], hdr->Source.B[1], hdr->Source.B[2], hdr->Source.B[3] );
	Log_Debug("IPv4", "Destination = %i.%i.%i.%i",
		hdr->Destination.B[0], hdr->Destination.B[1],
		hdr->Destination.B[2], hdr->Destination.B[3] );
	#endif	

	// Check that the version IS IPv4
	if(hdr->Version != 4) {
		Log_Log("IPv4", "hdr->Version(%i) != 4", hdr->Version);
		return;
	}
	
	// Check Header checksum
	{
		Uint16	hdrVal, compVal;
		hdrVal = ntohs(hdr->HeaderChecksum);
		hdr->HeaderChecksum = 0;
		compVal = IPv4_Checksum((Uint16*)hdr, (hdr->HeaderLength * 4) / 2);
		if(hdrVal != compVal) {
			Log_Log("IPv4", "Header checksum fails (%04x != %04x)", hdrVal, compVal);
			return ;
		}
		hdr->HeaderChecksum = hdrVal;
	}
	
	// Check Packet length
	if( ntohs(hdr->TotalLength) > Length) {
		Log_Log("IPv4", "hdr->TotalLength(%i) > Length(%i)", ntohs(hdr->TotalLength), Length);
		return;
	}
	
	// TODO: Handle packet fragmentation
	
	
	// Get Data and Data Length
	dataLength = ntohs(hdr->TotalLength) - sizeof(tIPv4Header);
	data = &hdr->Options[0];
	
	// Get Interface (allowing broadcasts)
	iface = IPv4_GetInterface(Adapter, hdr->Destination, 1);
	
	// Firewall rules
	if( iface ) {
		// Incoming Packets
		ret = IPTables_TestChain("INPUT",
			4, &hdr->Source, &hdr->Destination,
			hdr->Protocol, 0,
			dataLength, data
			);
	}
	else {
		// Routed packets
		ret = IPTables_TestChain("FORWARD",
			4, &hdr->Source, &hdr->Destination,
			hdr->Protocol, 0,
			dataLength, data
			);
	}
	switch(ret)
	{
	// 0 - Allow
	case 0:	break;
	// 1 - Silent Drop
	case 1:
		Log_Debug("IPv4", "Silently dropping packet");
		return ;
	case -1:
		// Bad rule
		break ;
	// Unknown, silent drop
	default:
		Log_Warning("IPv4", "Unknown firewall rule");
		return ;
	}
	
	// Routing
	if(!iface)
	{
		tMacAddr	to;
		tRoute	*rt;
		
		Log_Debug("IPv4", "Route the packet");
		// Drop the packet if the TTL is zero
		if( hdr->TTL == 0 ) {
			Log_Warning("IPv4", "TODO: Sent ICMP-Timeout when TTL exceeded");
			return ;
		}
		
		hdr->TTL --;
		
		rt = IPStack_FindRoute(4, NULL, &hdr->Destination);	// Get the route (gets the interface)
		to = ARP_Resolve4(rt->Interface, hdr->Destination);	// Resolve address
		if( MAC_EQU(to, cMAC_ZERO) )
			return ;
		
		// Send packet
		Log_Log("IPv4", "Forwarding packet to %i.%i.%i.%i (via %i.%i.%i.%i)",
			hdr->Destination.B[0], hdr->Destination.B[1],
			hdr->Destination.B[2], hdr->Destination.B[3],
			((tIPv4*)rt->NextHop)->B[0], ((tIPv4*)rt->NextHop)->B[1],
			((tIPv4*)rt->NextHop)->B[2], ((tIPv4*)rt->NextHop)->B[3]);
		Link_SendPacket(rt->Interface->Adapter, IPV4_ETHERNET_ID, to, Length, Buffer);
		
		
		return ;
	}
	
	// Send it on
	if( !gaIPv4_Callbacks[hdr->Protocol] ) {
		Log_Log("IPv4", "Unknown Protocol %i", hdr->Protocol);
		return ;
	}
	
	gaIPv4_Callbacks[hdr->Protocol]( iface, &hdr->Source, dataLength, data );
}

/**
 * \fn tInterface *IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address)
 * \brief Searches an adapter for a matching address
 * \param Adapter	Incoming Adapter
 * \param Address	Destination Address
 * \param Broadcast	Allow broadcast packets
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
		if( IP4_EQU(Address, *(tIPv4*)iface->Address) )
			return iface;
		
		if( !Broadcast )	continue;
		
		this = ntohl( ((tIPv4*)iface->Address)->L );
		
		// Check for broadcast
		netmask = IPv4_Netmask(iface->SubnetBits);
		
		if( (addr & netmask) == (this & netmask)
		 && (addr & ~netmask) == (0xFFFFFFFF & ~netmask) )
			return iface;
	}
	return NULL;
}

/**
 * \brief Convert a network prefix to a netmask
 * \param FixedBits	Netmask size (/n)
 * 
 * For example /24 will become 255.255.255.0 (0xFFFFFF00)
 */
Uint32 IPv4_Netmask(int FixedBits)
{
	Uint32	ret = 0xFFFFFFFF;
	ret >>= (32-FixedBits);
	ret <<= (32-FixedBits);
	// Returns a native endian netmask
	return ret;
}

/**
 * \brief Calculate the IPv4 Checksum
 * \param Buf	Input buffer
 * \param Size	Size of input
 * 
 * One's complement sum of all 16-bit words (bitwise inverted)
 */
Uint16 IPv4_Checksum(const Uint16 *Buf, int WordCount)
{
	Uint32	sum = 0;
	 int	i;
	
	// Sum all whole words
	for(i = 0; i < WordCount; i++ )
	{
		Uint16	val = ntohs(Buf[i]);
		sum += val;
	}
	
	// Apply one's complement
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	return ~sum;
}

/**
 * \brief Sends an ICMP Echo and waits for a reply
 * \param IFace	Interface
 * \param Addr	Destination address
 */
int IPv4_Ping(tInterface *IFace, tIPv4 Addr)
{
	return ICMP_Ping(IFace, Addr);
}

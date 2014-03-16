/*
 * Acess2 IP Stack
 * - IPv4 Protcol Handling
 */
#define DEBUG	0
#include "ipstack.h"
#include "link.h"
#include "ipv4.h"
#include "hwaddr_cache.h"
#include "firewall.h"

// === CONSTANTS ===
#define DEFAULT_TTL	32
#define IPV4_TRACE	1	// set to 1 to enable packet tracing

// === IMPORTS ===
extern tInterface	*gIP_Interfaces;
extern void	ICMP_Initialise();
extern  int	ICMP_Ping(tInterface *Interface, tIPv4 Addr);

// === PROTOTYPES ===
 int	IPv4_Initialise();
 int	IPv4_RegisterCallback(int ID, tIPCallback Callback);
void	IPv4_int_GetPacket(tAdapter *Interface, tMacAddr From, int Length, void *Buffer);
tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
Uint32	IPv4_Netmask(int FixedBits);
Uint16	IPv4_Checksum(const void *Buf, size_t Length);
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
int IPv4_SendPacket(tInterface *Iface, tIPv4 Address, int Protocol, int ID, tIPStackBuffer *Buffer)
{
	tMacAddr	to;
	tIPv4Header	hdr;
	 int	length;

	length = IPStack_Buffer_GetLength(Buffer);
	
	// --- Resolve destination MAC address
	to = HWCache_Resolve(Iface, &Address);
	if( MAC_EQU(to, cMAC_ZERO) ) {
		// No route to host
		Log_Notice("IPv4", "No route to host %i.%i.%i.%i",
			Address.B[0], Address.B[1], Address.B[2], Address.B[3]);
		return 0;
	}
	
	// --- Handle OUTPUT firewall rules
	// TODO: Update firewall rules for tIPStackBuffer
	#if 0
	int ret = IPTables_TestChain("OUTPUT",
		4, (tIPv4*)Iface->Address, &Address,
		Protocol, 0,
		length, Data);
	if(ret > 0) {
		// Just drop it (with an error)
		Log_Notice("IPv4", "Firewall dropped packet");
		return 0;
	}
	#endif

	// --- Initialise header	
	hdr.Version = 4;
	hdr.HeaderLength = sizeof(tIPv4Header)/4;
	hdr.DiffServices = 0;	// TODO: Check
	
	hdr.Reserved = 0;
	hdr.DontFragment = 0;
	hdr.MoreFragments = 0;
	hdr.FragOffLow = 0;
	hdr.FragOffHi = 0;
	
	hdr.TotalLength = htons( sizeof(tIPv4Header) + length );
	hdr.Identifcation = htons( ID );	// TODO: Check
	hdr.TTL = DEFAULT_TTL;
	hdr.Protocol = Protocol;
	hdr.HeaderChecksum = 0;	// Will be set later
	hdr.Source = *(tIPv4*)Iface->Address;
	hdr.Destination = Address;

	// Actually set checksum (zeroed above)
	hdr.HeaderChecksum = htons( IPv4_Checksum(&hdr, sizeof(tIPv4Header)) );

	IPStack_Buffer_AppendSubBuffer(Buffer, sizeof(tIPv4Header), 0, &hdr, NULL, NULL);

	#if IPV4_TRACE
	Log_Log("IPv4", "Sending packet to %i.%i.%i.%i",
		Address.B[0], Address.B[1], Address.B[2], Address.B[3]);
	#endif
	Link_SendPacket(Iface->Adapter, IPV4_ETHERNET_ID, to, Buffer);
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
		compVal = IPv4_Checksum(hdr, hdr->HeaderLength * 4);
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
	
	#if IPV4_TRACE
	Log_Debug("IPv4", "Proto 0x%x From %i.%i.%i.%i to %i.%i.%i.%i",
		hdr->Protocol,
		hdr->Source.B[0], hdr->Source.B[1], hdr->Source.B[2], hdr->Source.B[3],
		hdr->Destination.B[0], hdr->Destination.B[1], hdr->Destination.B[2], hdr->Destination.B[3]
		);
	#endif
	
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
		// Drop the packet if the TTL is zero
		if( hdr->TTL == 0 ) {
			Log_Warning("IPv4", "TODO: Send ICMP-Timeout when TTL exceeded");
			return ;
		}
		hdr->TTL --;

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
		//IPStack_RoutePacket(4, &hdr->Destination, Length, Buffer);
		return ;
	}

	// Populate ARP cache from recieved packets
	// - Should be safe
	if( IPStack_CompareAddress(4, &hdr->Source, iface->Address, iface->SubnetBits) )
	{
		HWCache_Set(Adapter, 4, &hdr->Source, &From);
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
	tInterface	*iface = NULL, *zero_iface = NULL;
	Uint32	netmask;
	Uint32	addr, this;

	ENTER("pAdapter xAddress bBroadcast", Adapter, Address, Broadcast);	

	addr = ntohl( Address.L );
	LOG("addr = 0x%x", addr);
	
	for( iface = gIP_Interfaces; iface; iface = iface->Next)
	{
		if( iface->Adapter != Adapter )	continue;
		if( iface->Type != 4 )	continue;
		if( IP4_EQU(Address, *(tIPv4*)iface->Address) ) {
			LOG("Exact match");
			LEAVE('p', iface);
			return iface;
		}

		LOG("iface->Address = 0x%x", *(Uint32*)iface->Address);

		if( *(Uint32*)iface->Address == 0 ) {
			if( zero_iface ) {
				Log_Notice("IPv4", "Multiple 0.0.0.0 interfaces on the same adapter, ignoring");
			}
			else {
				zero_iface = iface;
				LOG("Zero IF %p", iface);
			}
			continue ;
		}		

		if( !Broadcast )	continue;
		
		// Check for broadcast
		this = ntohl( ((tIPv4*)iface->Address)->L );
		netmask = IPv4_Netmask(iface->SubnetBits);
		LOG("iface addr = 0x%x, netmask = 0x%x (bits = %i)", this, netmask, iface->SubnetBits);

		if( (addr & netmask) == (this & netmask) && (addr & ~netmask) == (0xFFFFFFFF & ~netmask) )
		{
			LOG("Broadcast match");
			LEAVE('p', iface);
			return iface;
		}
	}

	// Special case for intefaces that are being DHCP configured
	// - If the interface address is 0.0.0.0, then if there is no match for the
	//   destination the packet is treated as if it was addressed to 0.0.0.0
	if( zero_iface && Broadcast )
	{
		LOG("Using 0.0.0.0 interface with magic!");
		LEAVE('p', zero_iface);
		return zero_iface;
	}

	LEAVE('n');
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
	if( FixedBits == 0 )
		return 0;
	if( FixedBits < 32 )
	{
		ret >>= (32-FixedBits);
		ret <<= (32-FixedBits);
	}
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
Uint16 IPv4_Checksum(const void *Buf, size_t Length)
{
	//Debug_HexDump("IPv4_Checksum", Buf, Length);
	const Uint16	*words = Buf;
	Uint32	sum = 0;
	 int	i;
	
	// Sum all whole words
	for(i = 0; i < Length/2; i++ )
	{
		sum += ntohs(words[i]);
	}
	if( Length & 1 )
		sum += ntohs( words[i] & 0xFF );
	
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

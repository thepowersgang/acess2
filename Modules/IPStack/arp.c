/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 */
#include "ipstack.h"
#include "arp.h"
#include "link.h"

#define	ARP_CACHE_SIZE	64
#define	ARP_MAX_AGE		(60*60*1000)	// 1Hr

// === IMPORTS ===
extern tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
extern tInterface	*IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast);

// === PROTOTYPES ===
 int	ARP_Initialise();
 int	ARP_int_Resolve4(tInterface *Interface, tIPv4 Address);
void	ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer);

// === GLOBALS ===
struct sARP_Cache {
	tMacAddr	MAC;
	tIPv4	IP4;
	tIPv6	IP6;
	Sint64	LastUpdate;
	Sint64	LastUsed;
}	*gaARP_Cache;
 int	giARP_CacheSpace;
 int	giARP_LastUpdateID = 0;
tSpinlock	glARP_Cache;

// === CODE ===
/**
 * \fn int ARP_Initialise()
 * \brief Initalise the ARP section
 */
int ARP_Initialise()
{
	gaARP_Cache = malloc( ARP_CACHE_SIZE * sizeof(*gaARP_Cache) );
	memset( gaARP_Cache, 0, ARP_CACHE_SIZE * sizeof(*gaARP_Cache) );
	giARP_CacheSpace = ARP_CACHE_SIZE;
	
	Link_RegisterType(0x0806, ARP_int_GetPacket);
	return 1;
}

/**
 * \brief Resolves a MAC address from an IPv4 address
 */
tMacAddr ARP_Resolve4(tInterface *Interface, tIPv4 Address)
{
	 int	lastID;
	 int	i;
	
	LOCK( &glARP_Cache );
	for( i = 0; i < giARP_CacheSpace; i++ )
	{
		if(gaARP_Cache[i].IP4.L != Address.L)	continue;
		
		// Check if the entry needs to be refreshed
		if( now() - gaARP_Cache[i].LastUpdate > ARP_MAX_AGE )	break;
		
		RELEASE( &glARP_Cache );
		return gaARP_Cache[i].MAC;
	}
	RELEASE( &glARP_Cache );
	
	lastID = giARP_LastUpdateID;
	ARP_int_Resolve4(Interface, Address);
	for(;;)
	{
		while(lastID == giARP_LastUpdateID)	Threads_Yield();
		
		LOCK( &glARP_Cache );
		for( i = 0; i < giARP_CacheSpace; i++ )
		{
			if(gaARP_Cache[i].IP4.L != Address.L)	continue;
			
			RELEASE( &glARP_Cache );
			return gaARP_Cache[i].MAC;
		}
		RELEASE( &glARP_Cache );
	}
}

/**
 * \fn int ARP_int_Resolve4(tInterface *Interface, tIPv4 Address)
 * \brief Request the network to resolve an IPv4 Address
 * \return Boolean Success
 */
int ARP_int_Resolve4(tInterface *Interface, tIPv4 Address)
{
	struct sArpRequest4	req;
	
	req.HWType = htons(0x100);	// Ethernet
	req.Type   = htons(0x0800);
	req.HWSize = 6;
	req.SWSize = 4;
	req.Request = htons(1);
	req.SourceMac = Interface->Adapter->MacAddr;
	req.SourceIP = Interface->IP4.Address;
	req.DestMac = cMAC_BROADCAST;
	req.DestIP = Address;
	
	Link_SendPacket(Interface->Adapter, 0x0806, req.DestMac, sizeof(struct sArpRequest4), &req);
	
	return 0;
}

/**
 * \fn void ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer)
 * \brief Called when an ARP packet is recieved
 */
void ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer)
{
	 int	i, free = -1;
	 int	oldest = 0;
	tArpRequest4	*req4 = Buffer;
	tArpRequest6	*req6 = Buffer;
	tInterface	*iface;
	
	// Sanity Check Packet
	if( Length < sizeof(tArpRequest4) ) {
		Log("[ARP  ] Recieved undersized packet");
		return ;
	}
	if( ntohs(req4->Type) != 0x0800 ) {
		Log("[ARP  ] Recieved a packet with a bad type 0x%x", ntohs(req4->Type));
		return ;
	}
	if( req4->HWSize != 6 ) {
		Log("[ARP  ] Recieved a packet with HWSize != 6 (%i)", req4->HWSize);
		return;
	}
	if( !MAC_EQU(req4->SourceMac, From) ) {
		Log("[ARP  ] ARP spoofing detected", req4->HWSize);
		return;
	}
	
	Log("[ARP  ] Request ID %i", ntohs(req4->Request));
	
	switch( ntohs(req4->Request) )
	{
	case 1:	// You want my IP?
		Log("[ARP  ] ARP Request Address class %i", req4->SWSize);
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			Log("[ARP  ] From MAC %02x:%02x:%02x:%02x:%02x:%02x",
				req4->SourceMac.B[0], req4->SourceMac.B[1],
				req4->SourceMac.B[2], req4->SourceMac.B[3],
				req4->SourceMac.B[4], req4->SourceMac.B[5]);
			Log("[ARP  ] to MAC %02x:%02x:%02x:%02x:%02x:%02x",
				req4->DestMac.B[0], req4->DestMac.B[1],
				req4->DestMac.B[2], req4->DestMac.B[3],
				req4->DestMac.B[4], req4->DestMac.B[5]);
			Log("[ARP  ] ARP Request IPv4 Address %i.%i.%i.%i",
				req4->DestIP.B[0], req4->DestIP.B[1], req4->DestIP.B[2],
				req4->DestIP.B[3]);
			Log("[ARP  ] from %i.%i.%i.%i",
				req4->SourceIP.B[0], req4->SourceIP.B[1],
				req4->SourceIP.B[2], req4->SourceIP.B[3]);
			iface = IPv4_GetInterface(Adapter, req4->DestIP, 0);
			if( iface )
			{
				req4->DestIP = req4->SourceIP;
				req4->DestMac = req4->SourceMac;
				req4->SourceIP = iface->IP4.Address;
				req4->SourceMac = Adapter->MacAddr;
				req4->Request = htons(2);
				Log("[ARP  ] Hey, That's us!");
				Log("[ARP  ] Sending back %02x:%02x:%02x:%02x:%02x:%02x",
					req4->SourceMac.B[0], req4->SourceMac.B[1],
					req4->SourceMac.B[2], req4->SourceMac.B[3],
					req4->SourceMac.B[4], req4->SourceMac.B[5]);
				Link_SendPacket(Adapter, 0x0806, req4->DestMac, sizeof(tArpRequest4), req4);
			}
			break;
		case 6:
			if( Length < sizeof(tArpRequest6) ) {
				Log("[ARP  ] Recieved undersized packet (IPv6)");
				return ;
			}
			iface = IPv6_GetInterface(Adapter, req6->DestIP, 0);
			if( iface )
			{
				req6->DestIP = req6->SourceIP;
				req6->DestMac = req6->SourceMac;
				req6->SourceIP = iface->IP6.Address;
				req6->SourceMac = Adapter->MacAddr;
				req6->Request = htons(2);
				Link_SendPacket(Adapter, 0x0806, req6->DestMac, sizeof(tArpRequest6), req6);
			}
			break;
		default:
			Log("[ARP  ] Unknown Protocol Address size (%i)", req4->SWSize);
			return ;
		}
		
		break;
	
	case 2:	// Ooh! A response!
		// Find an entry for the MAC address in the cache
		LOCK(&glARP_Cache);
		for( i = giARP_CacheSpace; i--; )
		{
			if(gaARP_Cache[oldest].LastUpdate > gaARP_Cache[i].LastUpdate) {
				oldest = i;
			}
			if( MAC_EQU(gaARP_Cache[i].MAC, From) )	break;
			if( gaARP_Cache[i].LastUpdate == 0 && free==-1 )	free = i;
		}
		if(i + 1 == 0) {
			if(free != -1)
				i = free;
			else
				i = oldest;
		}
		
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			gaARP_Cache[i].IP4 = req4->SourceIP;
			break;
		case 6:
			if( Length < sizeof(tArpRequest6) ) {
				Log("[ARP  ] Recieved undersized packet (IPv6)");
				return ;
			}
			gaARP_Cache[i].IP6 = req6->SourceIP;
			break;
		default:
			Log("[ARP  ] Unknown Protocol Address size (%i)", req4->SWSize);
			RELEASE(&glARP_Cache);
			return ;
		}
		
		gaARP_Cache[i].LastUpdate = now();
		RELEASE(&glARP_Cache);
		break;
	}
}

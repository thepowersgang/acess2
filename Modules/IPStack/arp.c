/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 */
#include "ipstack.h"
#include "arp.h"
#include "link.h"

#define	ARP_CACHE_SIZE	64

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
}	*gaARP_Cache;
 int	giARP_CacheSpace;

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
	
	switch( req4->Request )
	{
	case 1:	// You want my IP?
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			iface = IPv4_GetInterface(Adapter, req4->DestIP, 0);
			if( iface )
			{
				IP4_SET(req4->DestIP, req4->SourceIP);
				req4->DestMac = req4->SourceMac;
				req4->SourceIP = iface->IP4.Address;
				req4->SourceMac = Adapter->MacAddr;
				req4->Request = htons(2);
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
			return ;
		}
		
		gaARP_Cache[i].LastUpdate = now();
		break;
	}
}

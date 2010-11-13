/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 */
#define DEBUG	0
#include "ipstack.h"
#include "arp.h"
#include "link.h"

#define ARPv6	1
#define	ARP_CACHE_SIZE	64
#define	ARP_MAX_AGE		(60*60*1000)	// 1Hr

// === IMPORTS ===
extern tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
#if ARPv6
extern tInterface	*IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast);
#endif

// === PROTOTYPES ===
 int	ARP_Initialise();
tMacAddr	ARP_Resolve4(tInterface *Interface, tIPv4 Address);
void	ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer);

// === GLOBALS ===
struct sARP_Cache4 {
	tIPv4	IP;
	tMacAddr	MAC;
	Sint64	LastUpdate;
	Sint64	LastUsed;
}	*gaARP_Cache4;
 int	giARP_Cache4Space;
tMutex	glARP_Cache4;
#if ARPv6
struct sARP_Cache6 {
	tIPv6	IP;
	tMacAddr	MAC;
	Sint64	LastUpdate;
	Sint64	LastUsed;
}	*gaARP_Cache6;
 int	giARP_Cache6Space;
tMutex	glARP_Cache6;
#endif
 int	giARP_LastUpdateID = 0;

// === CODE ===
/**
 * \fn int ARP_Initialise()
 * \brief Initalise the ARP section
 */
int ARP_Initialise()
{
	gaARP_Cache4 = malloc( ARP_CACHE_SIZE * sizeof(struct sARP_Cache4) );
	memset( gaARP_Cache4, 0, ARP_CACHE_SIZE * sizeof(struct sARP_Cache4) );
	giARP_Cache4Space = ARP_CACHE_SIZE;
	
	#if ARPv6
	gaARP_Cache6 = malloc( ARP_CACHE_SIZE * sizeof(struct sARP_Cache6) );
	memset( gaARP_Cache6, 0, ARP_CACHE_SIZE * sizeof(struct sARP_Cache6) );
	giARP_Cache6Space = ARP_CACHE_SIZE;
	#endif
	
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
	struct sArpRequest4	req;
	
	ENTER("pInterface xAddress", Interface, Address);
	
	Mutex_Acquire( &glARP_Cache4 );
	for( i = 0; i < giARP_Cache4Space; i++ )
	{
		if(gaARP_Cache4[i].IP.L != Address.L)	continue;
		
		// Check if the entry needs to be refreshed
		if( now() - gaARP_Cache4[i].LastUpdate > ARP_MAX_AGE )	break;
		
		Mutex_Release( &glARP_Cache4 );
		LOG("Return %x:%x:%x:%x:%x:%x",
			gaARP_Cache4[i].MAC.B[0], gaARP_Cache4[i].MAC.B[1],
			gaARP_Cache4[i].MAC.B[2], gaARP_Cache4[i].MAC.B[3],
			gaARP_Cache4[i].MAC.B[4], gaARP_Cache4[i].MAC.B[5]
			);
		LEAVE('-');
		return gaARP_Cache4[i].MAC;
	}
	Mutex_Release( &glARP_Cache4 );
	
	lastID = giARP_LastUpdateID;
	
	// Create request
	Log_Log("ARP4", "Asking for address %i.%i.%i.%i",
		Address.B[0], Address.B[1], Address.B[2], Address.B[3]
		);
	req.HWType = htons(0x0001);	// Ethernet
	req.Type   = htons(0x0800);
	req.HWSize = 6;
	req.SWSize = 4;
	req.Request = htons(1);
	req.SourceMac = Interface->Adapter->MacAddr;
	req.SourceIP = Interface->IP4.Address;
	req.DestMac = cMAC_BROADCAST;
	req.DestIP = Address;
	
	// Send Request
	Link_SendPacket(Interface->Adapter, 0x0806, req.DestMac, sizeof(struct sArpRequest4), &req);
	
	// Wait for a reply
	for(;;)
	{
		while(lastID == giARP_LastUpdateID)	Threads_Yield();
		lastID = giARP_LastUpdateID;
		
		Mutex_Acquire( &glARP_Cache4 );
		for( i = 0; i < giARP_Cache4Space; i++ )
		{
			if(gaARP_Cache4[i].IP.L != Address.L)	continue;
			
			Mutex_Release( &glARP_Cache4 );
			return gaARP_Cache4[i].MAC;
		}
		Mutex_Release( &glARP_Cache4 );
	}
}

/**
 * \brief Updates the ARP Cache entry for an IPv4 Address
 */
void ARP_UpdateCache4(tIPv4 SWAddr, tMacAddr HWAddr)
{
	 int	i;
	 int	free = -1;
	 int	oldest = 0;
	
	// Find an entry for the IP address in the cache
	Mutex_Acquire(&glARP_Cache4);
	for( i = giARP_Cache4Space; i--; )
	{
		if(gaARP_Cache4[oldest].LastUpdate > gaARP_Cache4[i].LastUpdate) {
			oldest = i;
		}
		if( gaARP_Cache4[i].IP.L == SWAddr.L )	break;
		if( gaARP_Cache4[i].LastUpdate == 0 && free == -1 )	free = i;
	}
	// If there was no match, we need to make one
	if(i == -1) {
		if(free != -1)
			i = free;
		else
			i = oldest;
		gaARP_Cache4[i].IP = SWAddr;
	}
	
	Log_Log("ARP4", "Caching %i.%i.%i.%i (%02x:%02x:%02x:%02x:%02x:%02x) in %i",
		SWAddr.B[0], SWAddr.B[1], SWAddr.B[2], SWAddr.B[3],
		HWAddr.B[0], HWAddr.B[1], HWAddr.B[2], HWAddr.B[3], HWAddr.B[4], HWAddr.B[5],
		i
		);
		
	gaARP_Cache4[i].MAC = HWAddr;
	gaARP_Cache4[i].LastUpdate = now();
	giARP_LastUpdateID ++;
	Mutex_Release(&glARP_Cache4);
}

#if ARPv6
/**
 * \brief Updates the ARP Cache entry for an IPv6 Address
 */
void ARP_UpdateCache6(tIPv6 SWAddr, tMacAddr HWAddr)
{
	 int	i;
	 int	free = -1;
	 int	oldest = 0;
	
	// Find an entry for the MAC address in the cache
	Mutex_Acquire(&glARP_Cache6);
	for( i = giARP_Cache6Space; i--; )
	{
		if(gaARP_Cache6[oldest].LastUpdate > gaARP_Cache6[i].LastUpdate) {
			oldest = i;
		}
		if( MAC_EQU(gaARP_Cache6[i].MAC, HWAddr) )	break;
		if( gaARP_Cache6[i].LastUpdate == 0 && free == -1 )	free = i;
	}
	// If there was no match, we need to make one
	if(i == -1) {
		if(free != -1)
			i = free;
		else
			i = oldest;
		gaARP_Cache6[i].MAC = HWAddr;
	}
	
	gaARP_Cache6[i].IP = SWAddr;
	gaARP_Cache6[i].LastUpdate = now();
	giARP_LastUpdateID ++;
	Mutex_Release(&glARP_Cache6);
}
#endif

/**
 * \fn void ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer)
 * \brief Called when an ARP packet is recieved
 */
void ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer)
{
	tArpRequest4	*req4 = Buffer;
	#if ARPv6
	tArpRequest6	*req6 = Buffer;
	#endif
	tInterface	*iface;
	
	// Sanity Check Packet
	if( Length < (int)sizeof(tArpRequest4) ) {
		Log_Log("ARP", "Recieved undersized packet");
		return ;
	}
	if( ntohs(req4->Type) != 0x0800 ) {
		Log_Log("ARP", "Recieved a packet with a bad type (0x%x)", ntohs(req4->Type));
		return ;
	}
	if( req4->HWSize != 6 ) {
		Log_Log("ARP", "Recieved a packet with HWSize != 6 (%i)", req4->HWSize);
		return;
	}
	#if ARP_DETECT_SPOOFS
	if( !MAC_EQU(req4->SourceMac, From) ) {
		Log_Log("ARP", "ARP spoofing detected "
			"(%02x%02x:%02x%02x:%02x%02x != %02x%02x:%02x%02x:%02x%02x)",
			req4->SourceMac.B[0], req4->SourceMac.B[1], req4->SourceMac.B[2],
			req4->SourceMac.B[3], req4->SourceMac.B[4], req4->SourceMac.B[5],
			From.B[0], From.B[1], From.B[2],
			From.B[3], From.B[4], From.B[5]
			);
		return;
	}
	#endif
	
	Log_Debug("ARP", "Request ID %i", ntohs(req4->Request));
	
	switch( ntohs(req4->Request) )
	{
	case 1:	// You want my IP?
		Log_Debug("ARP", "ARP Request Address class %i", req4->SWSize);
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			Log_Debug("ARP", "From MAC %02x:%02x:%02x:%02x:%02x:%02x",
				req4->SourceMac.B[0], req4->SourceMac.B[1],
				req4->SourceMac.B[2], req4->SourceMac.B[3],
				req4->SourceMac.B[4], req4->SourceMac.B[5]);
			//Log_Debug("ARP", "to MAC %02x:%02x:%02x:%02x:%02x:%02x",
			//	req4->DestMac.B[0], req4->DestMac.B[1],
			//	req4->DestMac.B[2], req4->DestMac.B[3],
			//	req4->DestMac.B[4], req4->DestMac.B[5]);
			Log_Debug("ARP", "ARP Request IPv4 Address %i.%i.%i.%i from %i.%i.%i.%i",
				req4->DestIP.B[0], req4->DestIP.B[1], req4->DestIP.B[2],
				req4->DestIP.B[3],
				req4->SourceIP.B[0], req4->SourceIP.B[1],
				req4->SourceIP.B[2], req4->SourceIP.B[3]);
			iface = IPv4_GetInterface(Adapter, req4->DestIP, 0);
			if( iface )
			{
				Log_Debug("ARP", "Caching sender's IP Address");
				ARP_UpdateCache4(req4->SourceIP, req4->SourceMac);
				
				req4->DestIP = req4->SourceIP;
				req4->DestMac = req4->SourceMac;
				req4->SourceIP = iface->IP4.Address;
				req4->SourceMac = Adapter->MacAddr;
				req4->Request = htons(2);
				Log_Debug("ARP", "Sending back us (%02x:%02x:%02x:%02x:%02x:%02x)",
					req4->SourceMac.B[0], req4->SourceMac.B[1],
					req4->SourceMac.B[2], req4->SourceMac.B[3],
					req4->SourceMac.B[4], req4->SourceMac.B[5]);
				Link_SendPacket(Adapter, 0x0806, req4->DestMac, sizeof(tArpRequest4), req4);
			}
			break;
		#if ARPv6
		case 6:
			if( Length < (int)sizeof(tArpRequest6) ) {
				Log_Log("ARP", "Recieved undersized packet (IPv6)");
				return ;
			}
			Log_Debug("ARP", "ARP Request IPv6 Address %08x:%08x:%08x:%08x",
				ntohl(req6->DestIP.L[0]), ntohl(req6->DestIP.L[1]),
				ntohl(req6->DestIP.L[2]), ntohl(req6->DestIP.L[3])
				);
			iface = IPv6_GetInterface(Adapter, req6->DestIP, 0);
			if( iface )
			{
				req6->DestIP = req6->SourceIP;
				req6->DestMac = req6->SourceMac;
				req6->SourceIP = iface->IP6.Address;
				req6->SourceMac = Adapter->MacAddr;
				req6->Request = htons(2);
				Log_Debug("ARP", "Sending back us (%02x:%02x:%02x:%02x:%02x:%02x)",
					req4->SourceMac.B[0], req4->SourceMac.B[1],
					req4->SourceMac.B[2], req4->SourceMac.B[3],
					req4->SourceMac.B[4], req4->SourceMac.B[5]);
				Link_SendPacket(Adapter, 0x0806, req6->DestMac, sizeof(tArpRequest6), req6);
			}
			break;
		#endif
		default:
			Log_Debug("ARP", "Unknown Protocol Address size (%i)", req4->SWSize);
			return ;
		}
		
		break;
	
	case 2:	// Ooh! A response!		
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			ARP_UpdateCache4( req4->SourceIP, From );
			break;
		#if ARPv6
		case 6:
			if( Length < (int)sizeof(tArpRequest6) ) {
				Log_Debug("ARP", "Recieved undersized packet (IPv6)");
				return ;
			}
			ARP_UpdateCache6( req6->SourceIP, From );
			break;
		#endif
		default:
			Log_Debug("ARP", "Unknown Protocol Address size (%i)", req4->SWSize);
			return ;
		}
		
		break;
	
	default:
		Log_Warning("ARP", "Unknown Request ID %i", ntohs(req4->Request));
		break;
	}
}

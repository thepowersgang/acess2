/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 * - Part of the IPv4 protocol
 */
#define DEBUG	0
#include "ipstack.h"
#include "arp.h"
#include "link.h"
#include "ipv4.h"	// For IPv4_Netmask
#include "include/adapters_int.h"	// for MAC addr
#include <semaphore.h>
#include <timers.h>
#include "hwaddr_cache.h"

#define ARPv6	0
#define	ARP_CACHE_SIZE	128
#define	ARP_MAX_AGE		(60*60*1000)	// 1Hr

// === IMPORTS ===
extern tInterface	*IPv4_GetInterface(tAdapter *Adapter, tIPv4 Address, int Broadcast);
#if ARPv6
extern tInterface	*IPv6_GetInterface(tAdapter *Adapter, tIPv6 Address, int Broadcast);
#endif

// === PROTOTYPES ===
 int	ARP_Initialise();
void	ARP_int_GetPacket(tAdapter *Adapter, tMacAddr From, int Length, void *Buffer);

// === GLOBALS ===

// === CODE ===
/**
 * \fn int ARP_Initialise()
 * \brief Initalise the ARP section
 */
int ARP_Initialise()
{
	Link_RegisterType(0x0806, ARP_int_GetPacket);
	return 1;
}

void ARP_Request4(tInterface *Interface, tIPv4 Address)
{
	struct sArpRequest4	req;
	// Create request
	Log_Log("ARP4", "Asking for address %i.%i.%i.%i",
		Address.B[0], Address.B[1], Address.B[2], Address.B[3]
		);
	req.HWType = htons(0x0001);	// Ethernet
	req.Type   = htons(0x0800);
	req.HWSize = 6;
	req.SWSize = 4;
	req.Request = htons(1);
	memcpy(&req.SourceMac, Interface->Adapter->HWAddr, 6);	// TODO: Remove hard size
	req.SourceIP = *(tIPv4*)Interface->Address;
	req.DestMac = cMAC_BROADCAST;
	req.DestIP = Address;

	// Assumes only a header and footer at link layer
	tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(3);
	IPStack_Buffer_AppendSubBuffer(buffer, sizeof(struct sArpRequest4), 0, &req, NULL, NULL);

	// Send Request
	Link_SendPacket(Interface->Adapter, 0x0806, req.DestMac, buffer);
}

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
	
	switch( ntohs(req4->Request) )
	{
	case 1:	// You want my IP?
		// Check what type of IP it is
		switch( req4->SWSize )
		{
		case 4:
			iface = IPv4_GetInterface(Adapter, req4->DestIP, 0);
			if( iface )
			{
				Log_Debug("ARP", "ARP Request IPv4 Address %i.%i.%i.%i from %i.%i.%i.%i"
					" (%02x:%02x:%02x:%02x:%02x:%02x)",
					req4->DestIP.B[0], req4->DestIP.B[1], req4->DestIP.B[2],
					req4->DestIP.B[3],
					req4->SourceIP.B[0], req4->SourceIP.B[1],
					req4->SourceIP.B[2], req4->SourceIP.B[3],
					req4->SourceMac.B[0], req4->SourceMac.B[1],
					req4->SourceMac.B[2], req4->SourceMac.B[3],
					req4->SourceMac.B[4], req4->SourceMac.B[5]);
				// Someone has ARPed us, let's cache them
				HWCache_Set(Adapter, 4, &req4->SourceIP, &req4->SourceMac);
				
				req4->DestIP = req4->SourceIP;
				req4->DestMac = req4->SourceMac;
				req4->SourceIP = *(tIPv4*)iface->Address;;
				memcpy(&req4->SourceMac, Adapter->HWAddr, 6);	// TODO: Remove hard size
				req4->Request = htons(2);
				Log_Debug("ARP", "Sending back us (%02x:%02x:%02x:%02x:%02x:%02x)",
					req4->SourceMac.B[0], req4->SourceMac.B[1],
					req4->SourceMac.B[2], req4->SourceMac.B[3],
					req4->SourceMac.B[4], req4->SourceMac.B[5]);
				
				// Assumes only a header and footer at link layer
				tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(3);
				IPStack_Buffer_AppendSubBuffer(buffer,
					sizeof(struct sArpRequest4), 0, req4,
					NULL, NULL);
				Link_SendPacket(Adapter, 0x0806, req4->DestMac, buffer);
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
				req6->SourceIP = *(tIPv6*)iface->Address;
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
			HWCache_Set(Adapter, 4, &req4->SourceIP, &From);
			break;
		#if ARPv6
		case 6:
			if( Length < (int)sizeof(tArpRequest6) ) {
				Log_Debug("ARP", "Recieved undersized packet (IPv6)");
				return ;
			}
			HWCache_Set(Adapter, 6, &req6->SourceIP, &From );
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

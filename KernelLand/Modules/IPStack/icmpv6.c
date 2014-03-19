/*
 * Acess2 IP Stack
 *
 * icmpv6.c
 * - Internet Control Message Protocol v6
 */
#include "ipstack.h"
#include "ipv6.h"
#include "ipv4.h"	// For the IP Checksum
#include "icmpv6.h"
#include "link.h"
#include "hwaddr_cache.h"
#include "include/adapters_int.h"

// === GLOBALS ===
void	ICMPv6_GetPacket(tInterface *Interface, void *Address, int Length, const void *Buffer);
 int	ICMPv6_ND_GetOpt(size_t Length, const void *Buffer, Uint8 OptID, const void **Ptr);

// === CODE ===
void ICMPv6_Initialise(void)
{
	
}

void ICMPv6_GetPacket(tInterface *Interface, void *Address, int Length, const void *Buffer)
{
	if( Length < sizeof(tICMPv6_Header))
		return ;
	const tICMPv6_Header	*hdr = Buffer;
	
	// Validate checksum
	if( IPv4_Checksum(Buffer, Length) != 0 )
		return ;

	if( hdr->Type == ICMPV6_INFO_NEIGHBOUR_ADVERTISMENT )
	{
		if( Length < sizeof(tICMPv6_Header) + sizeof(tICMPv6_NA) )
			return ;
		// TODO: Check that IP.HopLimt == 255
		const tICMPv6_NA	*na = (const void*)(hdr+1);
		if( !(na->Flags & (1 << 1)) ) {
			// Unsolicited, TODO
			return ;
		}
		if( na->Flags & (1 << 2) ) {
			// Override, force update
		}
		const tICMPv6_Opt_LinkAddr	*la = NULL;
		if( ICMPv6_ND_GetOpt(Length-sizeof(*hdr)+sizeof(*na), na+1, ICMPV6_OPTION_SRCLINK, (const void**)&la) )
			return ;
		if( !la ) {
			LOG("No link address on neighbor advertisement");
			return;
		}
		
		HWCache_Set(Interface->Adapter, 6, Address, (const tMacAddr*)la->Address);
	}
	else {
		// Return unhandled?
	}
	
	switch(hdr->Type)
	{
	case ICMPV6_INFO_ROUTER_SOLICITATION:
		// TODO: If routing is active, send out RA
		break;
	case ICMPV6_INFO_ROUTER_ADVERTISEMENT: {
		// TODO: If routing INACTIVE, and interface is ::/0, set address
		break; }
	case ICMPV6_INFO_NEIGHBOUR_SOLICITATION:
		if( Length < sizeof(tICMPv6_Header) + sizeof(tICMPv6_NS) )
			return ;
		// TODO: Check that IP.HopLimt == 255
		break;
	case ICMPV6_INFO_NEIGHBOUR_ADVERTISMENT:
		//HWCache_Set(Interface->Adapter, 
		break;
	}
}

int ICMPv6_ND_GetOpt(size_t Length, const void *Buffer, Uint8 OptID, const void **Ptr)
{
	const struct {
		Uint8	Type;
		Uint8	Length;
		Uint16	_resvd1;
		Uint32	_resvd2;
	}	*opts = Buffer;
	
	while( Length >= sizeof(*opts))
	{
		if( opts->Length == 0 )
			return 1;
		if( opts->Length * 8 > Length )
			return 1;
		
		if( opts->Type == OptID ) {
			*Ptr = opts;
			return 0;
		}
		opts += opts->Length;
	}
	return 0;
}

void ICMPv6_SendNS(tInterface *Interface, const void *Address)
{
	const Uint8	*addr8 = Address;
	struct {
		tIPv6Header	ip;
		tICMPv6_Header	icmp;
		tICMPv6_NS	ns;
		tICMPv6_Opt_LinkAddr	linkaddr;
		Uint8	our_mac[6];
	} PACKED pkt;
	// Custom-crafted IPv6 header
	pkt.ip.Version = 6;
	pkt.ip.TrafficClass = 0;
	pkt.ip.FlowLabel = 0;
	pkt.ip.Head = htonl(pkt.ip.Head);
	pkt.ip.PayloadLength = htons(sizeof(pkt)-sizeof(pkt.ip));
	pkt.ip.NextHeader = IPV6PROT_ICMPV6;
	pkt.ip.HopLimit = 255;	// Max value
	pkt.ip.Source = *(tIPv6*)Interface->Address;
	pkt.ip.Destination = (tIPv6){.B={0xFF,0x02, 0,0, 0,0, 0,0,  0,0, 0,1,0xFF,addr8[13], addr8[14],addr8[15]}};
	
	pkt.icmp.Type = ICMPV6_INFO_NEIGHBOUR_SOLICITATION;
	pkt.icmp.Code = 0;
	pkt.icmp.Checksum = 0;	// populated later

	pkt.ns.Reserved = 0;
	pkt.ns.TargetAddress = *(const tIPv6*)Address;
	
	pkt.linkaddr.Type = ICMPV6_OPTION_SRCLINK;
	pkt.linkaddr.Length = 1;	// 1 * 8 bytes
	memcpy(pkt.our_mac, Interface->Adapter->HWAddr, 6);

	pkt.icmp.Checksum = IPv4_Checksum(&pkt, sizeof(pkt));


	tMacAddr	to = {.B={0x33, 0x33, addr8[12], addr8[13], addr8[14], addr8[15]}};

	tIPStackBuffer *buffer = IPStack_Buffer_CreateBuffer(2);
	IPStack_Buffer_AppendSubBuffer(buffer, sizeof(pkt), 0, &pkt, NULL, NULL);
	Link_SendPacket(Interface->Adapter, IPV6_ETHERNET_ID, to, buffer);
	IPStack_Buffer_DestroyBuffer(buffer);
}

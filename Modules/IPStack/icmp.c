/*
 * Acess2 IP Stack
 * - ICMP Handling
 */
#include "ipstack.h"
#include "ipv4.h"
#include "icmp.h"

// === PROTOTYPES ===
void	ICMP_Initialise();
void	ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);

// === GLOBALS ===

// === CODE ===
/**
 * \fn void ICMP_Initialise()
 * \brief Initialise the ICMP Layer
 */
void ICMP_Initialise()
{
	IPv4_RegisterCallback(IP4PROT_ICMP, ICMP_GetPacket);
}

/**
 * \fn void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tICMPHeader	*hdr = Buffer;
	
	Log("[ICMP ] hdr->Type = %i", hdr->Type);
	Log("[ICMP ] hdr->Code = %i", hdr->Code);
	Log("[ICMP ] hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
	Log("[ICMP ] hdr->ID = 0x%x", ntohs(hdr->ID));
	Log("[ICMP ] hdr->Sequence = 0x%x", ntohs(hdr->Sequence));
}

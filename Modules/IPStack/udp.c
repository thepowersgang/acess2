/*
 * Acess2 IP Stack
 * - UDP Handling
 */
#include "ipstack.h"
#include "udp.h"

// === PROTOTYPES ===
void	UDP_Initialise();
void	UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
// --- Channel
tVFS_Node	*UDP_Channel_Init(tInterface *Interface);
Uint64	UDP_Channel_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	UDP_Channel_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	UDP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	UDP_Channel_Close(tVFS_Node *Node);

// === GLOBALS ===

// === CODE ===
/**
 * \fn void TCP_Initialise()
 * \brief Initialise the TCP Layer
 */
void UDP_Initialise()
{
	IPv4_RegisterCallback(IP4PROT_UDP, UDP_GetPacket);
}

/**
 * \fn void UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tUDPHeader	*hdr = Buffer;
	
	Log("[UDP  ] hdr->SourcePort = %i", ntohs(hdr->SourcePort));
	Log("[UDP  ] hdr->DestPort = %i", ntohs(hdr->DestPort));
	Log("[UDP  ] hdr->Length = %i", ntohs(hdr->Length));
	Log("[UDP  ] hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
}

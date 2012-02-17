/*
 * Acess2 IP Stack
 * - UDP Handling
 */
#include "ipstack.h"
#include <api_drv_common.h>
#include "udp.h"

#define UDP_ALLOC_BASE	0xC000

// === PROTOTYPES ===
void	UDP_Initialise();
void	UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
void	UDP_Unreachable(tInterface *Interface, int Code, void *Address, int Length, void *Buffer);
void	UDP_SendPacketTo(tUDPChannel *Channel, int AddrType, const void *Address, Uint16 Port, const void *Data, size_t Length);
// --- Client Channels
tVFS_Node	*UDP_Channel_Init(tInterface *Interface);
size_t	UDP_Channel_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	UDP_Channel_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
 int	UDP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	UDP_Channel_Close(tVFS_Node *Node);
// --- Helpers
Uint16	UDP_int_AllocatePort();
 int	UDP_int_MarkPortAsUsed(Uint16 Port);
void	UDP_int_FreePort(Uint16 Port);

// === GLOBALS ===
tVFS_NodeType	gUDP_NodeType = {
	.Read = UDP_Channel_Read,
	.Write = UDP_Channel_Write,
	.IOCtl = UDP_Channel_IOCtl,
	.Close = UDP_Channel_Close
};
tMutex	glUDP_Channels;
tUDPChannel	*gpUDP_Channels;

tMutex	glUDP_Ports;
Uint32	gUDP_Ports[0x10000/32];

tSocketFile	gUDP_SocketFile = {NULL, "udp", UDP_Channel_Init};

// === CODE ===
/**
 * \fn void TCP_Initialise()
 * \brief Initialise the TCP Layer
 */
void UDP_Initialise()
{
	IPStack_AddFile(&gUDP_SocketFile);
	//IPv4_RegisterCallback(IP4PROT_UDP, UDP_GetPacket, UDP_Unreachable);
	IPv4_RegisterCallback(IP4PROT_UDP, UDP_GetPacket);
}

/**
 * \brief Scan a list of tUDPChannels and find process the first match
 * \return 0 if no match was found, -1 on error and 1 if a match was found
 */
int UDP_int_ScanList(tUDPChannel *List, tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tUDPHeader	*hdr = Buffer;
	tUDPChannel	*chan;
	tUDPPacket	*pack;
	 int	len;
	
	for(chan = List;
		chan;
		chan = chan->Next)
	{
		// Match local endpoint
		if(chan->Interface && chan->Interface != Interface)	continue;
		if(chan->LocalPort != ntohs(hdr->DestPort))	continue;
		
		// Check for remote port restriction
		if(chan->Remote.Port && chan->Remote.Port != ntohs(hdr->SourcePort))
			continue;
		// Check for remote address restriction
		if(chan->RemoteMask)
		{
			if(chan->Remote.AddrType != Interface->Type)
				continue;
			if(!IPStack_CompareAddress(Interface->Type, Address,
				&chan->Remote.Addr, chan->RemoteMask)
				)
				continue;
		}
		
		Log_Log("UDP", "Recieved packet for %p", chan);
		// Create the cached packet
		len = ntohs(hdr->Length);
		pack = malloc(sizeof(tUDPPacket) + len);
		pack->Next = NULL;
		memcpy(&pack->Remote.Addr, Address, IPStack_GetAddressSize(Interface->Type));
		pack->Remote.Port = ntohs(hdr->SourcePort);
		pack->Remote.AddrType = Interface->Type;
		pack->Length = len;
		memcpy(pack->Data, hdr->Data, len);
		
		// Add the packet to the channel's queue
		SHORTLOCK(&chan->lQueue);
		if(chan->Queue)
			chan->QueueEnd->Next = pack;
		else
			chan->QueueEnd = chan->Queue = pack;
		SHORTREL(&chan->lQueue);
		VFS_MarkAvaliable(&chan->Node, 1);
		Mutex_Release(&glUDP_Channels);
		return 1;
	}
	return 0;
}

/**
 * \fn void UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tUDPHeader	*hdr = Buffer;
	
	Log_Debug("UDP", "hdr->SourcePort = %i", ntohs(hdr->SourcePort));
	Log_Debug("UDP", "hdr->DestPort = %i", ntohs(hdr->DestPort));
	Log_Debug("UDP", "hdr->Length = %i", ntohs(hdr->Length));
	Log_Debug("UDP", "hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
	
	// Check registered connections
	Mutex_Acquire(&glUDP_Channels);
	UDP_int_ScanList(gpUDP_Channels, Interface, Address, Length, Buffer);
	Mutex_Release(&glUDP_Channels);
}

/**
 * \brief Handle an ICMP Unrechable Error
 */
void UDP_Unreachable(tInterface *Interface, int Code, void *Address, int Length, void *Buffer)
{
	
}

/**
 * \brief Send a packet
 * \param Channel	Channel to send the packet from
 * \param Data	Packet data
 * \param Length	Length in bytes of packet data
 */
void UDP_SendPacketTo(tUDPChannel *Channel, int AddrType, const void *Address, Uint16 Port, const void *Data, size_t Length)
{
	tUDPHeader	*hdr;

	if(Channel->Interface && Channel->Interface->Type != AddrType)	return ;
	
	switch(AddrType)
	{
	case 4:
		// Create the packet
		hdr = malloc(sizeof(tUDPHeader)+Length);
		hdr->SourcePort = htons( Channel->LocalPort );
		hdr->DestPort = htons( Port );
		hdr->Length = htons( sizeof(tUDPHeader) + Length );
		hdr->Checksum = 0;	// Checksum can be zero on IPv4
		memcpy(hdr->Data, Data, Length);
		// Pass on the the IPv4 Layer
		// TODO: What if Channel->Interface is NULL here?
		IPv4_SendPacket(Channel->Interface, *(tIPv4*)Address, IP4PROT_UDP, 0, sizeof(tUDPHeader)+Length, hdr);
		// Free allocated packet
		free(hdr);
		break;
	}
}

// --- Client Channels
tVFS_Node *UDP_Channel_Init(tInterface *Interface)
{
	tUDPChannel	*new;
	new = calloc( sizeof(tUDPChannel), 1 );
	new->Interface = Interface;
	new->Node.ImplPtr = new;
	new->Node.NumACLs = 1;
	new->Node.ACLs = &gVFS_ACL_EveryoneRW;
	new->Node.Type = &gUDP_NodeType;
	
	Mutex_Acquire(&glUDP_Channels);
	new->Next = gpUDP_Channels;
	gpUDP_Channels = new;
	Mutex_Release(&glUDP_Channels);
	
	return &new->Node;
}

/**
 * \brief Read from the channel file (wait for a packet)
 */
size_t UDP_Channel_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	tUDPChannel	*chan = Node->ImplPtr;
	tUDPPacket	*pack;
	tUDPEndpoint	*ep;
	 int	ofs, addrlen;
	
	if(chan->LocalPort == 0) {
		Log_Notice("UDP", "Channel %p sent with no local port", chan);
		return 0;
	}
	
	while(chan->Queue == NULL)	Threads_Yield();
	
	for(;;)
	{
		VFS_SelectNode(Node, VFS_SELECT_READ, NULL, "UDP_Channel_Read");
		SHORTLOCK(&chan->lQueue);
		if(chan->Queue == NULL) {
			SHORTREL(&chan->lQueue);
			continue;
		}
		pack = chan->Queue;
		chan->Queue = pack->Next;
		if(!chan->Queue) {
			chan->QueueEnd = NULL;
			VFS_MarkAvaliable(Node, 0);	// Nothing left
		}
		SHORTREL(&chan->lQueue);
		break;
	}

	// Check that the header fits
	addrlen = IPStack_GetAddressSize(pack->Remote.AddrType);
	ep = Buffer;
	ofs = 4 + addrlen;
	if(Length < ofs) {
		free(pack);
		Log_Notice("UDP", "Insuficient space for header in buffer (%i < %i)", (int)Length, ofs);
		return 0;
	}
	
	// Fill header
	ep->Port = pack->Remote.Port;
	ep->AddrType = pack->Remote.AddrType;
	memcpy(&ep->Addr, &pack->Remote.Addr, addrlen);
	
	// Copy packet data
	if(Length > ofs + pack->Length)	Length = ofs + pack->Length;
	memcpy((char*)Buffer + ofs, pack->Data, Length - ofs);

	// Free cached packet
	free(pack);
	
	return Length;
}

/**
 * \brief Write to the channel file (send a packet)
 */
size_t UDP_Channel_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	tUDPChannel	*chan = Node->ImplPtr;
	const tUDPEndpoint	*ep;
	const void	*data;
	 int	ofs;
	if(chan->LocalPort == 0)	return 0;
	
	ep = Buffer;	
	ofs = 2 + 2 + IPStack_GetAddressSize( ep->AddrType );

	data = (const char *)Buffer + ofs;

	UDP_SendPacketTo(chan, ep->AddrType, &ep->Addr, ep->Port, data, (size_t)Length - ofs);
	
	return 0;
}

/**
 * \brief Names for channel IOCtl Calls
 */
static const char *casIOCtls_Channel[] = {
	DRV_IOCTLNAMES,
	"getset_localport",
	"getset_remoteport",
	"getset_remotemask",
	"set_remoteaddr",
	NULL
	};
/**
 * \brief Channel IOCtls
 */
int UDP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tUDPChannel	*chan = Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "UDP Channel", 0x100, casIOCtls_Channel);
	
	case 4:	// getset_localport (returns bool success)
		if(!Data)	LEAVE_RET('i', chan->LocalPort);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		// Set port
		chan->LocalPort = *(Uint16*)Data;
		// Permissions check (Ports lower than 1024 are root-only)
		if(chan->LocalPort != 0 && chan->LocalPort < 1024) {
			if( Threads_GetUID() != 0 ) {
				LOG("Attempt by non-superuser to listen on port %i", chan->LocalPort);
				chan->LocalPort = 0;
				LEAVE_RET('i', -1);
			}
		}
		// Allocate a random port if requested
		if( chan->LocalPort == 0 )
			chan->LocalPort = UDP_int_AllocatePort();
		else
		{
			// Else, mark the requested port as used
			if( UDP_int_MarkPortAsUsed(chan->LocalPort) == 0 ) {
				LOG("Port %i us currently in use", chan->LocalPort);
				chan->LocalPort = 0;
				LEAVE_RET('i', 0);
			}
			LEAVE_RET('i', 1);
		}
		LEAVE_RET('i', 1);
	
	case 5:	// getset_remoteport (returns bool success)
		if(!Data)	LEAVE_RET('i', chan->Remote.Port);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		chan->Remote.Port = *(Uint16*)Data;
		return 1;
	
	case 6:	// getset_remotemask (returns bool success)
		if(!Data)	LEAVE_RET('i', chan->RemoteMask);
		if(!CheckMem(Data, sizeof(int)))	LEAVE_RET('i', -1);
		if( !chan->Interface ) {
			LOG("Can't set remote mask on NULL interface");
			LEAVE_RET('i', -1);
		}
		if( *(int*)Data > IPStack_GetAddressSize(chan->Interface->Type) )
			LEAVE_RET('i', -1);
		chan->RemoteMask = *(int*)Data;
		return 1;	

	case 7:	// set_remoteaddr (returns bool success)
		if( !chan->Interface ) {
			LOG("Can't set remote address on NULL interface");
			LEAVE_RET('i', -1);
		}
		if(!CheckMem(Data, IPStack_GetAddressSize(chan->Interface->Type))) {
			LOG("Invalid pointer");
			LEAVE_RET('i', -1);
		}
		memcpy(&chan->Remote.Addr, Data, IPStack_GetAddressSize(chan->Interface->Type));
		return 0;
	}
	LEAVE_RET('i', 0);
}

/**
 * \brief Close and destroy an open channel
 */
void UDP_Channel_Close(tVFS_Node *Node)
{
	tUDPChannel	*chan = Node->ImplPtr;
	tUDPChannel	*prev;
	
	// Remove from the main list first
	Mutex_Acquire(&glUDP_Channels);
	if(gpUDP_Channels == chan)
		gpUDP_Channels = gpUDP_Channels->Next;
	else
	{
		for(prev = gpUDP_Channels;
			prev->Next && prev->Next != chan;
			prev = prev->Next);
		if(!prev->Next)
			Log_Warning("UDP", "Bookeeping Fail, channel %p is not in main list", chan);
		else
			prev->Next = prev->Next->Next;
	}
	Mutex_Release(&glUDP_Channels);
	
	// Clear Queue
	SHORTLOCK(&chan->lQueue);
	while(chan->Queue)
	{
		tUDPPacket	*tmp;
		tmp = chan->Queue;
		chan->Queue = tmp->Next;
		free(tmp);
	}
	SHORTREL(&chan->lQueue);
	
	// Free channel structure
	free(chan);
}

/**
 * \return Port Number on success, or zero on failure
 */
Uint16 UDP_int_AllocatePort()
{
	 int	i;
	Mutex_Acquire(&glUDP_Ports);
	// Fast Search
	for( i = UDP_ALLOC_BASE; i < 0x10000; i += 32 )
		if( gUDP_Ports[i/32] != 0xFFFFFFFF )
			break;
	if(i == 0x10000)	return 0;
	for( ;; i++ )
	{
		if( !(gUDP_Ports[i/32] & (1 << (i%32))) )
			return i;
	}
	Mutex_Release(&glUDP_Ports);
}

/**
 * \brief Allocate a specific port
 * \return Boolean Success
 */
int UDP_int_MarkPortAsUsed(Uint16 Port)
{
	Mutex_Acquire(&glUDP_Ports);
	if( gUDP_Ports[Port/32] & (1 << (Port%32)) ) {
		return 0;
		Mutex_Release(&glUDP_Ports);
	}
	gUDP_Ports[Port/32] |= 1 << (Port%32);
	Mutex_Release(&glUDP_Ports);
	return 1;
}

/**
 * \brief Free an allocated port
 */
void UDP_int_FreePort(Uint16 Port)
{
	Mutex_Acquire(&glUDP_Ports);
	gUDP_Ports[Port/32] &= ~(1 << (Port%32));
	Mutex_Release(&glUDP_Ports);
}

/*
 * Acess2 IP Stack
 * - By John Hodge (thePowersGang)
 *
 * udp.c
 * - UDP Protocol handling
 */
#define DEBUG	1
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
size_t	UDP_Channel_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	UDP_Channel_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	UDP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	UDP_Channel_Close(tVFS_Node *Node);
// --- Helpers
Uint16	UDP_int_AllocatePort(tUDPChannel *Channel);
 int	UDP_int_ClaimPort(tUDPChannel *Channel, Uint16 Port);
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
	
	for(chan = List; chan; chan = chan->Next)
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
	
	Log_Debug("UDP", "%i bytes :%i->:%i (Cksum 0x%04x)",
		ntohs(hdr->Length), ntohs(hdr->SourcePort), ntohs(hdr->Length), ntohs(hdr->Checksum));
	
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
	tUDPHeader	hdr;

	if(Channel->Interface && Channel->Interface->Type != AddrType)	return ;
	
	switch(AddrType)
	{
	case 4:
		// Create the packet
		hdr.SourcePort = htons( Channel->LocalPort );
		hdr.DestPort = htons( Port );
		hdr.Length = htons( sizeof(tUDPHeader) + Length );
		hdr.Checksum = 0;	// Checksum can be zero on IPv4
		// Pass on the the IPv4 Layer
		tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(2 + IPV4_BUFFERS);
		IPStack_Buffer_AppendSubBuffer(buffer, Length, 0, Data, NULL, NULL);
		IPStack_Buffer_AppendSubBuffer(buffer, sizeof(hdr), 0, &hdr, NULL, NULL);
		// TODO: What if Channel->Interface is NULL here?
		IPv4_SendPacket(Channel->Interface, *(tIPv4*)Address, IP4PROT_UDP, 0, buffer);
		IPStack_Buffer_DestroyBuffer(buffer);
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
size_t UDP_Channel_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
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
		tTime	timeout_z = 0, *timeout = (Flags & VFS_IOFLAG_NOBLOCK) ? &timeout_z : NULL;
		int rv = VFS_SelectNode(Node, VFS_SELECT_READ, timeout, "UDP_Channel_Read");
		if( rv ) {
			errno = (Flags & VFS_IOFLAG_NOBLOCK) ? EWOULDBLOCK : EINTR;
		}
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
size_t UDP_Channel_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tUDPChannel	*chan = Node->ImplPtr;
	const tUDPEndpoint	*ep;
	const void	*data;
	 int	ofs;
	
	if(chan->LocalPort == 0) {
		Log_Notice("UDP", "Write to channel %p with zero local port", chan);
		return 0;
	}
	
	ep = Buffer;	
	ofs = 2 + 2 + IPStack_GetAddressSize( ep->AddrType );

	data = (const char *)Buffer + ofs;

	UDP_SendPacketTo(chan, ep->AddrType, &ep->Addr, ep->Port, data, (size_t)Length - ofs);
	
	return Length;
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
	
	case 4:	{ // getset_localport (returns bool success)
		if(!Data)	LEAVE_RET('i', chan->LocalPort);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		// Set port
		int req_port = *(Uint16*)Data;
		// Permissions check (Ports lower than 1024 are root-only)
		if(req_port != 0 && req_port < 1024) {
			if( Threads_GetUID() != 0 ) {
				LOG("Attempt by non-superuser to listen on port %i", req_port);
				LEAVE_RET('i', -1);
			}
		}
		// Allocate a random port if requested
		if( req_port == 0 )
			UDP_int_AllocatePort(chan);
		// Else, mark the requested port as used
		else if( UDP_int_ClaimPort(chan, req_port) ) {
			LOG("Port %i is currently in use", req_port);
			LEAVE_RET('i', 0);
		}
		LEAVE_RET('i', chan->LocalPort);
		}
	
	case 5:	// getset_remoteport (returns bool success)
		if(!Data)	LEAVE_RET('i', chan->Remote.Port);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		chan->Remote.Port = *(Uint16*)Data;
		LEAVE('i', chan->Remote.Port);
		return chan->Remote.Port;
	
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
		LEAVE('i', chan->RemoteMask);
		return chan->RemoteMask;	

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
		LEAVE('i', 0);
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
Uint16 UDP_int_AllocatePort(tUDPChannel *Channel)
{
	Mutex_Acquire(&glUDP_Ports);
	 int	i;
	// Fast Search
	for( int base = UDP_ALLOC_BASE; base < 0x10000; base += 32 )
	{
		if( gUDP_Ports[base/32] == 0xFFFFFFFF )
			continue ;
		for( int i = 0; i < 32; i++ )
		{
			if( gUDP_Ports[base/32] & (1 << i) )
				continue ;
			gUDP_Ports[base/32] |= (1 << i);
			Mutex_Release(&glUDP_Ports);
			// If claim succeeds, good
			if( UDP_int_ClaimPort(Channel, base + i) == 0 )
				return base + i;
			// otherwise keep looking
			Mutex_Acquire(&glUDP_Ports);
			break;
		}
	}
	Mutex_Release(&glUDP_Ports);
	return 0;
}

/**
 * \brief Allocate a specific port
 * \return Boolean Success
 */
int UDP_int_ClaimPort(tUDPChannel *Channel, Uint16 Port)
{
	// Search channel list for a connection with same (or wildcard)
	// interface, and same port
	Mutex_Acquire(&glUDP_Channels);
	for( tUDPChannel *ch = gpUDP_Channels; ch; ch = ch->Next)
	{
		if( ch == Channel )
			continue ;
		if( ch->Interface && ch->Interface != Channel->Interface )
			continue ;
		if( ch->LocalPort != Port )
			continue ;
		Mutex_Release(&glUDP_Channels);
		return 1;
	}
	Channel->LocalPort = Port;
	Mutex_Release(&glUDP_Channels);
	return 0;
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

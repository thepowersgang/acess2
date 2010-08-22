/*
 * Acess2 IP Stack
 * - UDP Handling
 */
#include "ipstack.h"
#include <tpl_drv_common.h>
#include "udp.h"

#define UDP_ALLOC_BASE	0xC000

// === PROTOTYPES ===
void	UDP_Initialise();
void	UDP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
void	UDP_Unreachable(tInterface *Interface, int Code, void *Address, int Length, void *Buffer);
void	UDP_SendPacket(tUDPChannel *Channel, void *Data, size_t Length);
// --- Listening Server
tVFS_Node	*UDP_Server_Init(tInterface *Interface);
char	*UDP_Server_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*UDP_Server_FindDir(tVFS_Node *Node, const char *Name);
 int	UDP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	UDP_Server_Close(tVFS_Node *Node);
// --- Client Channels
tVFS_Node	*UDP_Channel_Init(tInterface *Interface);
Uint64	UDP_Channel_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	UDP_Channel_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	UDP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	UDP_Channel_Close(tVFS_Node *Node);
// --- Helpers
Uint16	UDP_int_AllocatePort();
 int	UDP_int_MarkPortAsUsed(Uint16 Port);
void	UDP_int_FreePort(Uint16 Port);

// === GLOBALS ===
tMutex	glUDP_Servers;
tUDPServer	*gpUDP_Servers;

tMutex	glUDP_Channels;
tUDPChannel	*gpUDP_Channels;

tMutex	glUDP_Ports;
Uint32	gUDP_Ports[0x10000/32];

tSocketFile	gUDP_ServerFile = {NULL, "udps", UDP_Server_Init};
tSocketFile	gUDP_ClientFile = {NULL, "udpc", UDP_Channel_Init};

// === CODE ===
/**
 * \fn void TCP_Initialise()
 * \brief Initialise the TCP Layer
 */
void UDP_Initialise()
{
	IPStack_AddFile(&gUDP_ServerFile);
	IPStack_AddFile(&gUDP_ClientFile);
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
		if(chan->Interface != Interface)	continue;
		if(chan->LocalPort != ntohs(hdr->DestPort))	continue;
		if(chan->RemotePort != ntohs(hdr->SourcePort))	continue;
		
		if(Interface->Type == 4) {
			if(!IP4_EQU(chan->RemoteAddr.v4, *(tIPv4*)Address))	continue;
		}
		else if(Interface->Type == 6) {
			if(!IP6_EQU(chan->RemoteAddr.v6, *(tIPv6*)Address))	continue;
		}
		else {
			Warning("[UDP  ] Address type %i unknown", Interface->Type);
			Mutex_Release(&glUDP_Channels);
			return -1;
		}
		
		Log("[UDP  ] Recieved packet for %p", chan);
		// Create the cached packet
		len = ntohs(hdr->Length);
		pack = malloc(sizeof(tUDPPacket) + len);
		pack->Next = NULL;
		pack->Length = len;
		memcpy(pack->Data, hdr->Data, len);
		
		// Add the packet to the channel's queue
		SHORTLOCK(&chan->lQueue);
		if(chan->Queue)
			chan->QueueEnd->Next = pack;
		else
			chan->QueueEnd = chan->Queue = pack;
		SHORTREL(&chan->lQueue);
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
	tUDPServer	*srv;
	 int	ret;
	
	Log("[UDP  ] hdr->SourcePort = %i", ntohs(hdr->SourcePort));
	Log("[UDP  ] hdr->DestPort = %i", ntohs(hdr->DestPort));
	Log("[UDP  ] hdr->Length = %i", ntohs(hdr->Length));
	Log("[UDP  ] hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
	
	// Check registered connections
	Mutex_Acquire(&glUDP_Channels);
	ret = UDP_int_ScanList(gpUDP_Channels, Interface, Address, Length, Buffer);
	Mutex_Release(&glUDP_Channels);
	if(ret != 0)	return ;
	
	
	// TODO: Server/Listener
	Mutex_Acquire(&glUDP_Servers);
	for(srv = gpUDP_Servers;
		srv;
		srv = srv->Next)
	{
		if(srv->Interface != Interface)	continue;
		if(srv->ListenPort != ntohs(hdr->DestPort))	continue;
		ret = UDP_int_ScanList(srv->Channels, Interface, Address, Length, Buffer);
		if(ret != 0)	break;
		
		// Add connection
		Warning("[UDP  ] TODO - Add channel on connection");
		//TODO
	}
	Mutex_Release(&glUDP_Servers);
	
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
void UDP_SendPacket(tUDPChannel *Channel, void *Data, size_t Length)
{
	tUDPHeader	*hdr;
	
	switch(Channel->Interface->Type)
	{
	case 4:
		// Create the packet
		hdr = malloc(sizeof(tUDPHeader)+Length);
		hdr->SourcePort = htons( Channel->LocalPort );
		hdr->DestPort = htons( Channel->RemotePort );
		hdr->Length = htons( sizeof(tUDPHeader) + Length );
		hdr->Checksum = 0;	// Checksum can be zero on IPv4
		memcpy(hdr->Data, Data, Length);
		// Pass on the the IPv4 Layer
		IPv4_SendPacket(Channel->Interface, Channel->RemoteAddr.v4, IP4PROT_UDP, 0, sizeof(tUDPHeader)+Length, hdr);
		// Free allocated packet
		free(hdr);
		break;
	}
}

// --- Listening Server
tVFS_Node *UDP_Server_Init(tInterface *Interface)
{
	tUDPServer	*new;
	new = calloc( sizeof(tUDPServer), 1 );
	if(!new)	return NULL;
	
	new->Node.ImplPtr = new;
	new->Node.Flags = VFS_FFLAG_DIRECTORY;
	new->Node.NumACLs = 1;
	new->Node.ACLs = &gVFS_ACL_EveryoneRX;
	new->Node.ReadDir = UDP_Server_ReadDir;
	new->Node.FindDir = UDP_Server_FindDir;
	new->Node.IOCtl = UDP_Server_IOCtl;
	new->Node.Close = UDP_Server_Close;
	
	Mutex_Acquire(&glUDP_Servers);
	new->Next = gpUDP_Servers;
	gpUDP_Servers = new;
	Mutex_Release(&glUDP_Servers);
	
	return &new->Node;
}

/**
 * \brief Wait for a connection and return its ID in a string
 */
char *UDP_Server_ReadDir(tVFS_Node *Node, int ID)
{
	tUDPServer	*srv = Node->ImplPtr;
	tUDPChannel	*chan;
	char	*ret;
	
	if( srv->ListenPort == 0 )	return NULL;
	
	// Lock (so another thread can't collide with us here) and wait for a connection
	Mutex_Acquire( &srv->Lock );
	while( srv->NewChannels == NULL )	Threads_Yield();
	// Pop the connection off the new list
	chan = srv->NewChannels;
	srv->NewChannels = chan->Next;
	// Release the lock
	Mutex_Release( &srv->Lock );
	
	// Create the ID string and return it
	ret = malloc(11+1);
	sprintf(ret, "%i", chan->Node.ImplInt);
	
	return ret;
}

/**
 * \brief Take a string and find the channel
 */
tVFS_Node *UDP_Server_FindDir(tVFS_Node *Node, const char *Name)
{
	tUDPServer	*srv = Node->ImplPtr;
	tUDPChannel	*chan;
	 int	id = atoi(Name);
	
	for(chan = srv->Channels;
		chan;
		chan = chan->Next)
	{
		if( chan->Node.ImplInt < id )	continue;
		if( chan->Node.ImplInt > id )	break;	// Go sorted lists!
		
		return &chan->Node;
	}
	
	return NULL;
}

/**
 * \brief Names for server IOCtl Calls
 */
static const char *casIOCtls_Server[] = {
	DRV_IOCTLNAMES,
	"getset_listenport",
	NULL
	};
/**
 * \brief Channel IOCtls
 */
int UDP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tUDPServer	*srv = Node->ImplPtr;
	
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "UDP Server", 0x100, casIOCtls_Server);
	
	case 4:	// getset_localport (returns bool success)
		if(!Data)	LEAVE_RET('i', srv->ListenPort);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		// Set port
		srv->ListenPort = *(Uint16*)Data;
		// Permissions check (Ports lower than 1024 are root-only)
		if(srv->ListenPort != 0 && srv->ListenPort < 1024) {
			if( Threads_GetUID() != 0 ) {
				LOG("Attempt by non-superuser to listen on port %i", srv->ListenPort);
				srv->ListenPort = 0;
				LEAVE_RET('i', -1);
			}
		}
		// Allocate a random port if requested
		if( srv->ListenPort == 0 )
			srv->ListenPort = UDP_int_AllocatePort();
		else
		{
			// Else, mark the requested port as used
			if( UDP_int_MarkPortAsUsed(srv->ListenPort) == 0 ) {
				LOG("Port %i us currently in use", srv->ListenPort);
				srv->ListenPort = 0;
				LEAVE_RET('i', -1);
			}
			LEAVE_RET('i', 1);
		}
		LEAVE_RET('i', 1);
	
	default:
		LEAVE_RET('i', -1);
	}
	LEAVE_RET('i', 0);
}

void UDP_Server_Close(tVFS_Node *Node)
{
	tUDPServer	*srv = Node->ImplPtr;
	tUDPServer	*prev;
	tUDPChannel	*chan;
	tUDPPacket	*tmp;
	
	
	// Remove from the main list first
	Mutex_Acquire(&glUDP_Servers);
	if(gpUDP_Servers == srv)
		gpUDP_Servers = gpUDP_Servers->Next;
	else
	{
		for(prev = gpUDP_Servers;
			prev->Next && prev->Next != srv;
			prev = prev->Next);
		if(!prev->Next)
			Warning("[UDP  ] Bookeeping Fail, server %p is not in main list", srv);
		else
			prev->Next = prev->Next->Next;
	}
	Mutex_Release(&glUDP_Servers);
	
	
	Mutex_Acquire(&srv->Lock);
	for(chan = srv->Channels;
		chan;
		chan = chan->Next)
	{
		// Clear Queue
		SHORTLOCK(&chan->lQueue);
		while(chan->Queue)
		{
			tmp = chan->Queue;
			chan->Queue = tmp->Next;
			free(tmp);
		}
		SHORTREL(&chan->lQueue);
		
		// Free channel structure
		free(chan);
	}
	Mutex_Release(&srv->Lock);
	
	free(srv);
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
	new->Node.Read = UDP_Channel_Read;
	new->Node.Write = UDP_Channel_Write;
	new->Node.IOCtl = UDP_Channel_IOCtl;
	new->Node.Close = UDP_Channel_Close;
	
	Mutex_Acquire(&glUDP_Channels);
	new->Next = gpUDP_Channels;
	gpUDP_Channels = new;
	Mutex_Release(&glUDP_Channels);
	
	return &new->Node;
}

/**
 * \brief Read from the channel file (wait for a packet)
 */
Uint64 UDP_Channel_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tUDPChannel	*chan = Node->ImplPtr;
	tUDPPacket	*pack;
	
	if(chan->LocalPort == 0)	return 0;
	if(chan->RemotePort == 0)	return 0;
	
	while(chan->Queue == NULL)	Threads_Yield();
	
	for(;;)
	{
		SHORTLOCK(&chan->lQueue);
		if(chan->Queue == NULL) {
			SHORTREL(&chan->lQueue);
			continue;
		}
		pack = chan->Queue;
		chan->Queue = pack->Next;
		if(!chan->Queue)	chan->QueueEnd = NULL;
		SHORTREL(&chan->lQueue);
		break;
	}
	
	// Clip length to packet length
	if(Length > pack->Length)	Length = pack->Length;
	// Copy packet data from cache
	memcpy(Buffer, pack->Data, Length);
	// Free cached packet
	free(pack);	
	
	return Length;
}

/**
 * \brief Write to the channel file (send a packet)
 */
Uint64 UDP_Channel_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tUDPChannel	*chan = Node->ImplPtr;
	if(chan->RemotePort == 0)	return 0;
	
	UDP_SendPacket(chan, Buffer, (size_t)Length);
	
	return 0;
}

/**
 * \brief Names for channel IOCtl Calls
 */
static const char *casIOCtls_Channel[] = {
	DRV_IOCTLNAMES,
	"getset_localport",
	"getset_remoteport",
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
		if(!Data)	LEAVE_RET('i', chan->RemotePort);
		if(!CheckMem( Data, sizeof(Uint16) ) ) {
			LOG("Invalid pointer %p", Data);
			LEAVE_RET('i', -1);
		}
		chan->RemotePort = *(Uint16*)Data;
		return 1;
	
	case 6:	// set_remoteaddr (returns bool success)
		switch(chan->Interface->Type)
		{
		case 4:
			if(!CheckMem(Data, sizeof(tIPv4))) {
				LOG("Invalid pointer %p", Data);
				LEAVE_RET('i', -1);
			}
			chan->RemoteAddr.v4 = *(tIPv4*)Data;
			break;
		}
		break;
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
			Warning("[UDP  ] Bookeeping Fail, channel %p is not in main list", chan);
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

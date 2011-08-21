/*
 * Acess2 IP Stack
 * - SCTP (Stream Control Transmission Protocol) Handling
 */
#include "ipstack.h"
#include <tpl_drv_common.h>
#include "sctp.h"

#define SCTP_ALLOC_BASE	0xC000

// === PROTOTYPES ===
void	SCTP_Initialise();
void	SCTP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
void	SCTP_Unreachable(tInterface *Interface, int Code, void *Address, int Length, void *Buffer);
void	SCTP_SendPacket(tSCTPChannel *Channel, void *Data, size_t Length);
// --- Listening Server
tVFS_Node	*SCTP_Server_Init(tInterface *Interface);
char	*SCTP_Server_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*SCTP_Server_FindDir(tVFS_Node *Node, const char *Name);
 int	SCTP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	SCTP_Server_Close(tVFS_Node *Node);
// --- Client Channels
tVFS_Node	*SCTP_Channel_Init(tInterface *Interface);
Uint64	SCTP_Channel_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	SCTP_Channel_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	SCTP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	SCTP_Channel_Close(tVFS_Node *Node);
// --- Helpers
Uint16	SCTP_int_AllocatePort();
 int	SCTP_int_MarkPortAsUsed(Uint16 Port);
void	SCTP_int_FreePort(Uint16 Port);

// === GLOBALS ===
tMutex	glSCTP_Servers;
tSCTPServer	*gpSCTP_Servers;

tMutex	glSCTP_Channels;
tSCTPChannel	*gpSCTP_Channels;

tMutex	glSCTP_Ports;
Uint32	gSCTP_Ports[0x10000/32];

tSocketFile	gSCTP_ServerFile = {NULL, "sctps", SCTP_Server_Init};
tSocketFile	gSCTP_ClientFile = {NULL, "sctpc", SCTP_Channel_Init};

// === CODE ===
/**
 * \fn void TCP_Initialise()
 * \brief Initialise the TCP Layer
 */
void SCTP_Initialise()
{
	IPStack_AddFile(&gSCTP_ServerFile);
	IPStack_AddFile(&gSCTP_ClientFile);
	//IPv4_RegisterCallback(IP4PROT_SCTP, SCTP_GetPacket, SCTP_Unreachable);
	IPv4_RegisterCallback(IP4PROT_SCTP, SCTP_GetPacket);
}

/**
 * \brief Scan a list of tSCTPChannels and find process the first match
 * \return 0 if no match was found, -1 on error and 1 if a match was found
 */
int SCTP_int_ScanList(tSCTPChannel *List, tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tSCTPHeader	*hdr = Buffer;
	tSCTPChannel	*chan;
	tSCTPPacket	*pack;
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
			Log_Warning("SCTP", "Address type %i unknown", Interface->Type);
			Mutex_Release(&glSCTP_Channels);
			return -1;
		}
		
		Log_Log("SCTP", "Recieved packet for %p", chan);
		// Create the cached packet
		len = ntohs(hdr->Length);
		pack = malloc(sizeof(tSCTPPacket) + len);
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
		Mutex_Release(&glSCTP_Channels);
		return 1;
	}
	return 0;
}

/**
 * \fn void SCTP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void SCTP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tSCTPHeader	*hdr = Buffer;
	tSCTPServer	*srv;
	 int	ret;
	
	Log_Log("SCTP", "hdr->SourcePort = %i", ntohs(hdr->SourcePort));
	Log_Log("SCTP", "hdr->DestPort = %i", ntohs(hdr->DestPort));
	Log_Log("SCTP", "hdr->VerifcationTag = %i", ntohs(hdr->VerifcationTag));
	Log_Log("SCTP", "hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
	
	// Check registered connections
	Mutex_Acquire(&glSCTP_Channels);
	ret = SCTP_int_ScanList(gpSCTP_Channels, Interface, Address, Length, Buffer);
	Mutex_Release(&glSCTP_Channels);
	if(ret != 0)	return ;
	
	
	// TODO: Server/Listener
	Mutex_Acquire(&glSCTP_Servers);
	for(srv = gpSCTP_Servers;
		srv;
		srv = srv->Next)
	{
		if(srv->Interface != Interface)	continue;
		if(srv->ListenPort != ntohs(hdr->DestPort))	continue;
		ret = SCTP_int_ScanList(srv->Channels, Interface, Address, Length, Buffer);
		if(ret != 0)	break;
		
		// Add connection
		Log_Warning("SCTP", "TODO - Add channel on connection");
		//TODO
	}
	Mutex_Release(&glSCTP_Servers);
	
}

/**
 * \brief Handle an ICMP Unrechable Error
 */
void SCTP_Unreachable(tInterface *Interface, int Code, void *Address, int Length, void *Buffer)
{
	
}

/**
 * \brief Send a packet
 * \param Channel	Channel to send the packet from
 * \param Data	Packet data
 * \param Length	Length in bytes of packet data
 */
void SCTP_SendPacket(tSCTPChannel *Channel, void *Data, size_t Length)
{
	tSCTPHeader	*hdr;
	
	switch(Channel->Interface->Type)
	{
	case 4:
		// Create the packet
		hdr = malloc(sizeof(tSCTPHeader)+Length);
		hdr->SourcePort = htons( Channel->LocalPort );
		hdr->DestPort = htons( Channel->RemotePort );
		hdr->Length = htons( sizeof(tSCTPHeader) + Length );
		hdr->Checksum = 0;	// Checksum can be zero on IPv4
		memcpy(hdr->Data, Data, Length);
		// Pass on the the IPv4 Layer
		IPv4_SendPacket(Channel->Interface, Channel->RemoteAddr.v4, IP4PROT_SCTP, 0, sizeof(tSCTPHeader)+Length, hdr);
		// Free allocated packet
		free(hdr);
		break;
	}
}

// --- Listening Server
tVFS_Node *SCTP_Server_Init(tInterface *Interface)
{
	tSCTPServer	*new;
	new = calloc( sizeof(tSCTPServer), 1 );
	if(!new)	return NULL;
	
	new->Node.ImplPtr = new;
	new->Node.Flags = VFS_FFLAG_DIRECTORY;
	new->Node.NumACLs = 1;
	new->Node.ACLs = &gVFS_ACL_EveryoneRX;
	new->Node.ReadDir = SCTP_Server_ReadDir;
	new->Node.FindDir = SCTP_Server_FindDir;
	new->Node.IOCtl = SCTP_Server_IOCtl;
	new->Node.Close = SCTP_Server_Close;
	
	Mutex_Acquire(&glSCTP_Servers);
	new->Next = gpSCTP_Servers;
	gpSCTP_Servers = new;
	Mutex_Release(&glSCTP_Servers);
	
	return &new->Node;
}

/**
 * \brief Wait for a connection and return its ID in a string
 */
char *SCTP_Server_ReadDir(tVFS_Node *Node, int ID)
{
	tSCTPServer	*srv = Node->ImplPtr;
	tSCTPChannel	*chan;
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
tVFS_Node *SCTP_Server_FindDir(tVFS_Node *Node, const char *Name)
{
	tSCTPServer	*srv = Node->ImplPtr;
	tSCTPChannel	*chan;
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
int SCTP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tSCTPServer	*srv = Node->ImplPtr;
	
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "SCTP Server", 0x100, casIOCtls_Server);
	
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
			srv->ListenPort = SCTP_int_AllocatePort();
		else
		{
			// Else, mark the requested port as used
			if( SCTP_int_MarkPortAsUsed(srv->ListenPort) == 0 ) {
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

void SCTP_Server_Close(tVFS_Node *Node)
{
	tSCTPServer	*srv = Node->ImplPtr;
	tSCTPServer	*prev;
	tSCTPChannel	*chan;
	tSCTPPacket	*tmp;
	
	
	// Remove from the main list first
	Mutex_Acquire(&glSCTP_Servers);
	if(gpSCTP_Servers == srv)
		gpSCTP_Servers = gpSCTP_Servers->Next;
	else
	{
		for(prev = gpSCTP_Servers;
			prev->Next && prev->Next != srv;
			prev = prev->Next);
		if(!prev->Next)
			Log_Warning("SCTP", "Bookeeping Fail, server %p is not in main list", srv);
		else
			prev->Next = prev->Next->Next;
	}
	Mutex_Release(&glSCTP_Servers);
	
	
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
tVFS_Node *SCTP_Channel_Init(tInterface *Interface)
{
	tSCTPChannel	*new;
	new = calloc( sizeof(tSCTPChannel), 1 );
	new->Interface = Interface;
	new->Node.ImplPtr = new;
	new->Node.NumACLs = 1;
	new->Node.ACLs = &gVFS_ACL_EveryoneRW;
	new->Node.Read = SCTP_Channel_Read;
	new->Node.Write = SCTP_Channel_Write;
	new->Node.IOCtl = SCTP_Channel_IOCtl;
	new->Node.Close = SCTP_Channel_Close;
	
	Mutex_Acquire(&glSCTP_Channels);
	new->Next = gpSCTP_Channels;
	gpSCTP_Channels = new;
	Mutex_Release(&glSCTP_Channels);
	
	return &new->Node;
}

/**
 * \brief Read from the channel file (wait for a packet)
 */
Uint64 SCTP_Channel_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tSCTPChannel	*chan = Node->ImplPtr;
	tSCTPPacket	*pack;
	
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
Uint64 SCTP_Channel_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tSCTPChannel	*chan = Node->ImplPtr;
	if(chan->RemotePort == 0)	return 0;
	
	SCTP_SendPacket(chan, Buffer, (size_t)Length);
	
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
int SCTP_Channel_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tSCTPChannel	*chan = Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "SCTP Channel", 0x100, casIOCtls_Channel);
	
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
			chan->LocalPort = SCTP_int_AllocatePort();
		else
		{
			// Else, mark the requested port as used
			if( SCTP_int_MarkPortAsUsed(chan->LocalPort) == 0 ) {
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
void SCTP_Channel_Close(tVFS_Node *Node)
{
	tSCTPChannel	*chan = Node->ImplPtr;
	tSCTPChannel	*prev;
	
	// Remove from the main list first
	Mutex_Acquire(&glSCTP_Channels);
	if(gpSCTP_Channels == chan)
		gpSCTP_Channels = gpSCTP_Channels->Next;
	else
	{
		for(prev = gpSCTP_Channels;
			prev->Next && prev->Next != chan;
			prev = prev->Next);
		if(!prev->Next)
			Log_Warning("SCTP", "Bookeeping Fail, channel %p is not in main list", chan);
		else
			prev->Next = prev->Next->Next;
	}
	Mutex_Release(&glSCTP_Channels);
	
	// Clear Queue
	SHORTLOCK(&chan->lQueue);
	while(chan->Queue)
	{
		tSCTPPacket	*tmp;
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
Uint16 SCTP_int_AllocatePort()
{
	 int	i;
	Mutex_Acquire(&glSCTP_Ports);
	// Fast Search
	for( i = SCTP_ALLOC_BASE; i < 0x10000; i += 32 )
		if( gSCTP_Ports[i/32] != 0xFFFFFFFF )
			break;
	if(i == 0x10000)	return 0;
	for( ;; i++ )
	{
		if( !(gSCTP_Ports[i/32] & (1 << (i%32))) )
			return i;
	}
	Mutex_Release(&glSCTP_Ports);
}

/**
 * \brief Allocate a specific port
 * \return Boolean Success
 */
int SCTP_int_MarkPortAsUsed(Uint16 Port)
{
	Mutex_Acquire(&glSCTP_Ports);
	if( gSCTP_Ports[Port/32] & (1 << (Port%32)) ) {
		return 0;
		Mutex_Release(&glSCTP_Ports);
	}
	gSCTP_Ports[Port/32] |= 1 << (Port%32);
	Mutex_Release(&glSCTP_Ports);
	return 1;
}

/**
 * \brief Free an allocated port
 */
void SCTP_int_FreePort(Uint16 Port)
{
	Mutex_Acquire(&glSCTP_Ports);
	gSCTP_Ports[Port/32] &= ~(1 << (Port%32));
	Mutex_Release(&glSCTP_Ports);
}

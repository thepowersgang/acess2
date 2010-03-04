/*
 * Acess2 IP Stack
 * - TCP Handling
 */
#include "ipstack.h"
#include "ipv4.h"
#include "tcp.h"

#define TCP_MIN_DYNPORT	0xC000
#define TCP_MAX_HALFOPEN	1024	// Should be enough

// === PROTOTYPES ===
void	TCP_Initialise();
void	TCP_StartConnection(tTCPConnection *Conn);
void	TCP_SendPacket(tTCPConnection *Conn, size_t Length, tTCPHeader *Data);
void	TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
void	TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header, int Length);
void	TCP_INT_AppendRecieved(tTCPConnection *Connection, tTCPStoredPacket *Ptk);
void	TCP_INT_UpdateRecievedFromFuture(tTCPConnection *Connection);
Uint16	TCP_GetUnusedPort();
 int	TCP_AllocatePort(Uint16 Port);
 int	TCP_DeallocatePort(Uint16 Port);
// --- Server
tVFS_Node	*TCP_Server_Init(tInterface *Interface);
char	*TCP_Server_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*TCP_Server_FindDir(tVFS_Node *Node, char *Name);
 int	TCP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Server_Close(tVFS_Node *Node);
// --- Client
tVFS_Node	*TCP_Client_Init(tInterface *Interface);
Uint64	TCP_Client_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	TCP_Client_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	TCP_Client_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Client_Close(tVFS_Node *Node);

// === TEMPLATES ===
tSocketFile	gTCP_ServerFile = {NULL, "tcps", TCP_Server_Init};
tSocketFile	gTCP_ClientFile = {NULL, "tcpc", TCP_Client_Init};

// === GLOBALS ===
 int	giTCP_NumHalfopen = 0;
tSpinlock	glTCP_Listeners;
tTCPListener	*gTCP_Listeners;
tSpinlock	glTCP_OutbountCons;
tTCPConnection	*gTCP_OutbountCons;
Uint32	gaTCP_PortBitmap[0x800];
 int	giTCP_NextOutPort = TCP_MIN_DYNPORT;

// === CODE ===
/**
 * \fn void TCP_Initialise()
 * \brief Initialise the TCP Layer
 */
void TCP_Initialise()
{
	IPStack_AddFile(&gTCP_ServerFile);
	IPStack_AddFile(&gTCP_ClientFile);
	IPv4_RegisterCallback(IP4PROT_TCP, TCP_GetPacket);
}

/**
 * \brief Open a connection to another host using TCP
 */
void TCP_StartConnection(tTCPConnection *Conn)
{
	tTCPHeader	hdr;

	hdr.SourcePort = Conn->LocalPort;
	hdr.DestPort = Conn->RemotePort;
	Conn->NextSequenceSend = rand();
	hdr.SequenceNumber = Conn->NextSequenceSend;
	hdr.DataOffset = (sizeof(tTCPHeader)/4) << 4;
	hdr.Flags = TCP_FLAG_SYN;
	hdr.WindowSize = 0;	// TODO
	hdr.Checksum = 0;	// TODO
	hdr.UrgentPointer = 0;
	// SEND PACKET
	TCP_SendPacket( Conn, sizeof(tTCPHeader), &hdr );
	return ;
}

/**
 * \brief Sends a packet from the specified connection, calculating the checksums
 * \param Conn	Connection
 * \param Length	Length of data
 * \param Data	Packet data
 */
void TCP_SendPacket( tTCPConnection *Conn, size_t Length, tTCPHeader *Data )
{
	size_t	buflen;
	Uint32	*buf;
	switch( Conn->Interface->Type )
	{
	case 4:	// Append IPv4 Pseudo Header
		buflen = 4 + 4 + 4 + ((Length+1)&~1);
		buf = malloc( buflen );
		buf[0] = Conn->Interface->IP4.Address.L;
		buf[1] = Conn->RemoteIP.v4.L;
		buf[2] = (htons(Length)<<16) | (6<<8) | 0;
		Data->Checksum = 0;
		memcpy( &buf[3], Data, Length );
		Data->Checksum = IPv4_Checksum( buf, buflen );
		free(buf);
		IPv4_SendPacket(Conn->Interface, Conn->RemoteIP.v4, IP4PROT_TCP, 0, Length, Data);
		break;
	}
}

/**
 * \fn void TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tTCPHeader	*hdr = Buffer;
	tTCPListener	*srv;
	tTCPConnection	*conn;

	Log("[TCP  ] SourcePort = %i, DestPort = %i",
		ntohs(hdr->SourcePort), ntohs(hdr->DestPort));
	Log("[TCP  ] SequenceNumber = 0x%x", ntohl(hdr->SequenceNumber));
	Log("[TCP  ] AcknowlegementNumber = 0x%x", ntohl(hdr->AcknowlegementNumber));
	Log("[TCP  ] DataOffset = %i", hdr->DataOffset >> 4);
	Log("[TCP  ] Flags = {");
	Log("[TCP  ]   CWR = %B, ECE = %B",
		!!(hdr->Flags & TCP_FLAG_CWR), !!(hdr->Flags & TCP_FLAG_ECE));
	Log("[TCP  ]   URG = %B, ACK = %B",
		!!(hdr->Flags & TCP_FLAG_URG), !!(hdr->Flags & TCP_FLAG_ACK));
	Log("[TCP  ]   PSH = %B, RST = %B",
		!!(hdr->Flags & TCP_FLAG_PSH), !!(hdr->Flags & TCP_FLAG_RST));
	Log("[TCP  ]   SYN = %B, FIN = %B",
		!!(hdr->Flags & TCP_FLAG_SYN), !!(hdr->Flags & TCP_FLAG_FIN));
	Log("[TCP  ] }");
	Log("[TCP  ] WindowSize = %i", htons(hdr->WindowSize));
	Log("[TCP  ] Checksum = 0x%x", htons(hdr->Checksum));
	Log("[TCP  ] UrgentPointer = 0x%x", htons(hdr->UrgentPointer));

	if( Length > (hdr->DataOffset >> 4)*4 )
	{
		Debug_HexDump(
			"[TCP  ] Packet Data = ",
			(Uint8*)hdr + (hdr->DataOffset >> 4)*4,
			Length - (hdr->DataOffset >> 4)*4
			);
	}

	// Check Servers
	{
		for( srv = gTCP_Listeners; srv; srv = srv->Next )
		{
			// Check if the server is active
			if(srv->Port == 0)	continue;
			// Check the interface
			if(srv->Interface && srv->Interface != Interface)	continue;
			// Check the destination port
			if(srv->Port != htons(hdr->DestPort))	continue;
			
			Log("[TCP  ] Matches server %p", srv);
			// Is this in an established connection?
			for( conn = srv->Connections; conn; conn = conn->Next )
			{
				Log("[TCP  ] conn->Interface(%p) == Interface(%p)",
					conn->Interface, Interface);
				// Check that it is coming in on the same interface
				if(conn->Interface != Interface)	continue;

				// Check Source Port
				Log("[TCP  ] conn->RemotePort(%i) == hdr->SourcePort(%i)",
					conn->RemotePort, ntohs(hdr->SourcePort));
				if(conn->RemotePort != ntohs(hdr->SourcePort))	continue;

				// Check Source IP
				if(conn->Interface->Type == 6 && !IP6_EQU(conn->RemoteIP.v6, *(tIPv6*)Address))
					continue;
				if(conn->Interface->Type == 4 && !IP4_EQU(conn->RemoteIP.v4, *(tIPv4*)Address))
					continue;

				Log("[TCP  ] Matches connection %p", conn);
				// We have a response!
				TCP_INT_HandleConnectionPacket(conn, hdr, Length);

				return;
			}

			Log("[TCP  ] Opening Connection");
			// Open a new connection (well, check that it's a SYN)
			if(hdr->Flags != TCP_FLAG_SYN) {
				Log("[TCP  ] Packet is not a SYN");
				return ;
			}
			
			// TODO: Check for halfopen max
			
			conn = calloc(1, sizeof(tTCPConnection));
			conn->State = TCP_ST_HALFOPEN;
			conn->LocalPort = srv->Port;
			conn->RemotePort = ntohs(hdr->SourcePort);
			conn->Interface = Interface;
			
			switch(Interface->Type)
			{
			case 4:	conn->RemoteIP.v4 = *(tIPv4*)Address;	break;
			case 6:	conn->RemoteIP.v6 = *(tIPv6*)Address;	break;
			}
			
			conn->NextSequenceRcv = ntohl( hdr->SequenceNumber ) + 1;
			// + (Length-(hdr->DataOffset>>4)*4);
			conn->NextSequenceSend = rand();
			
			// Create node
			conn->Node.NumACLs = 1;
			conn->Node.ACLs = &gVFS_ACL_EveryoneRW;
			conn->Node.ImplInt = srv->NextID ++;
			conn->Node.Read = TCP_Client_Read;
			conn->Node.Write = TCP_Client_Write;
			//conn->Node.Close = TCP_SrvConn_Close;
			
			// Hmm... Theoretically, this lock will never have to wait,
			// as the interface is locked to the watching thread, and this
			// runs in the watching thread. But, it's a good idea to have
			// it, just in case
			// Oh, wait, there is a case where a wildcard can be used
			// (srv->Interface == NULL) so having the lock is a good idea
			LOCK(&srv->lConnections);
			if( !srv->Connections )
				srv->Connections = conn;
			else
				srv->ConnectionsTail->Next = conn;
			srv->ConnectionsTail = conn;
			if(!srv->NewConnections)
				srv->NewConnections = conn;
			RELEASE(&srv->lConnections);

			// Send the SYN ACK
			hdr->Flags |= TCP_FLAG_ACK;
			hdr->AcknowlegementNumber = htonl(conn->NextSequenceRcv);
			hdr->SequenceNumber = htonl(conn->NextSequenceSend);
			hdr->DestPort = hdr->SourcePort;
			hdr->SourcePort = htons(srv->Port);
			hdr->DataOffset = (sizeof(tTCPHeader)/4) << 4;
			TCP_SendPacket( conn, sizeof(tTCPHeader), hdr );

			return ;
		}
	}


	// Check Open Connections
	{
		for( conn = gTCP_OutbountCons; conn; conn = conn->Next )
		{
			// Check that it is coming in on the same interface
			if(conn->Interface != Interface)	continue;

			// Check Source Port
			if(conn->RemotePort != ntohs(hdr->SourcePort))	continue;

			// Check Source IP
			if(conn->Interface->Type == 6 && !IP6_EQU(conn->RemoteIP.v6, *(tIPv6*)Address))
				continue;
			if(conn->Interface->Type == 4 && !IP4_EQU(conn->RemoteIP.v4, *(tIPv4*)Address))
				continue;

			TCP_INT_HandleConnectionPacket(conn, hdr, Length);
			return ;
		}
	}
	
	Log("[TCP  ] No Match");
}

/**
 * \brief Handles a packet sent to a specific connection
 */
void TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header, int Length)
{	
	tTCPStoredPacket	*pkt;
	 int	dataLen;
	
	Connection->State = TCP_ST_OPEN;
	if(Header->Flags & TCP_FLAG_SYN) {
		Connection->NextSequenceRcv = Header->SequenceNumber + 1;
	}
	
	// Get length of data
	dataLen = Length - (Header->DataOffset>>4)*4;
	Log("[TCP  ] HandleConnectionPacket - dataLen = %i", dataLen);
	
	if(Header->Flags & TCP_FLAG_ACK) {
		// TODO: Process an ACKed Packet
		Log("[TCP  ] Conn %p, Packet 0x%x ACKed", Connection, Header->AcknowlegementNumber);
	}
	
	if(dataLen == 0)	return ;
	
	// NOTES:
	// Flags
	//    PSH - Has Data?
	// /NOTES
	
	// Allocate and fill cached packet
	pkt = malloc( dataLen + sizeof(tTCPStoredPacket) );
	pkt->Next = NULL;
	pkt->Sequence = ntohl(Header->SequenceNumber);
	pkt->Length = dataLen;
	memcpy(pkt->Data, (Uint8*)Header + (Header->DataOffset>>4)*4, dataLen);
	
	// Is this packet the next expected packet?
	if( pkt->Sequence != Connection->NextSequenceRcv )
	{
		tTCPStoredPacket	*tmp, *prev;
		
		Log("[TCP  ] Out of sequence packet (0x%08x != 0x%08x)",
			pkt->Sequence, Connection->NextSequenceRcv);
		
		// No? Well, let's cache it and look at it later
		LOCK( &Connection->lFuturePackets );
		for(tmp = Connection->FuturePackets;
			tmp;
			prev = tmp, tmp = tmp->Next)
		{
			if(tmp->Sequence > pkt->Sequence)	break;
		}
		if(prev)
			prev->Next = pkt;
		else
			Connection->FuturePackets = pkt;
		pkt->Next = tmp;
		RELEASE( &Connection->lFuturePackets );
	}
	else
	{
		// Ooh, Goodie! Add it to the recieved list
		TCP_INT_AppendRecieved(Connection, pkt);
		Connection->NextSequenceRcv ++;
		
		// TODO: This should be moved out of the watcher thread,
		// so that a single lost packet on one connection doesn't cause
		// all connections on the interface to lag.
		TCP_INT_UpdateRecievedFromFuture(Connection);
	}
	
	// TODO: Check ACK code validity
	Header->AcknowlegementNumber = ntohl(pkt->Sequence);
	Header->SequenceNumber = ntohl(Connection->NextSequenceSend);
	Header->Flags &= TCP_FLAG_SYN;
	Header->Flags = TCP_FLAG_ACK;
	TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
}

/**
 * \brief Appends a packet to the recieved list
 */
void TCP_INT_AppendRecieved(tTCPConnection *Connection, tTCPStoredPacket *Pkt)
{
	LOCK( &Connection->lRecievedPackets );
	if(Connection->RecievedPackets)
	{
		Connection->RecievedPacketsTail->Next = Pkt;
		Connection->RecievedPacketsTail = Pkt;
	}
	else
	{
		Connection->RecievedPackets = Pkt;
		Connection->RecievedPacketsTail = Pkt;
	}
	RELEASE( &Connection->lRecievedPackets );
}

/**
 * \brief Updates the connections recieved list from the future list
 */
void TCP_INT_UpdateRecievedFromFuture(tTCPConnection *Connection)
{
	tTCPStoredPacket	*pkt, *prev;
	for(;;)
	{
		prev = NULL;
		// Look for the next expected packet in the cache.
		LOCK( &Connection->lFuturePackets );
		for(pkt = Connection->FuturePackets;
			pkt && pkt->Sequence < Connection->NextSequenceRcv;
			prev = pkt, pkt = pkt->Next);
		
		// If we can't find the expected next packet, stop looking
		if(!pkt || pkt->Sequence > Connection->NextSequenceRcv) {
			RELEASE( &Connection->lFuturePackets );
			return;
		}
		
		// Delete packet from future list
		if(prev)
			prev->Next = pkt->Next;
		else
			Connection->FuturePackets = pkt->Next;
		
		// Release list
		RELEASE( &Connection->lFuturePackets );
		
		// Looks like we found one
		TCP_INT_AppendRecieved(Connection, pkt);
		Connection->NextSequenceRcv ++;
	}
}

/**
 * \fn Uint16 TCP_GetUnusedPort()
 * \brief Gets an unused port and allocates it
 */
Uint16 TCP_GetUnusedPort()
{
	Uint16	ret;

	// Get Next outbound port
	ret = giTCP_NextOutPort++;
	while( gaTCP_PortBitmap[ret/32] & (1 << (ret%32)) )
	{
		ret ++;
		giTCP_NextOutPort++;
		if(giTCP_NextOutPort == 0x10000) {
			ret = giTCP_NextOutPort = TCP_MIN_DYNPORT;
		}
	}

	// Mark the new port as used
	gaTCP_PortBitmap[ret/32] |= 1 << (ret%32);

	return ret;
}

/**
 * \fn int TCP_AllocatePort(Uint16 Port)
 * \brief Marks a port as used
 */
int TCP_AllocatePort(Uint16 Port)
{
	// Check if the port has already been allocated
	if( gaTCP_PortBitmap[Port/32] & (1 << (Port%32)) )
		return 0;

	// Allocate
	gaTCP_PortBitmap[Port/32] |= 1 << (Port%32);

	return 1;
}

/**
 * \fn int TCP_DeallocatePort(Uint16 Port)
 * \brief Marks a port as unused
 */
int TCP_DeallocatePort(Uint16 Port)
{
	// Check if the port has already been allocated
	if( !(gaTCP_PortBitmap[Port/32] & (1 << (Port%32))) )
		return 0;

	// Allocate
	gaTCP_PortBitmap[Port/32] &= ~(1 << (Port%32));

	return 1;
}

// --- Server
tVFS_Node *TCP_Server_Init(tInterface *Interface)
{
	tTCPListener	*srv = malloc( sizeof(tTCPListener) );

	srv->Interface = Interface;
	srv->Port = 0;
	srv->NextID = 0;
	srv->Connections = NULL;
	srv->Next = NULL;
	srv->Node.Flags = VFS_FFLAG_DIRECTORY;
	srv->Node.Size = -1;
	srv->Node.ImplPtr = srv;
	srv->Node.NumACLs = 1;
	srv->Node.ACLs = &gVFS_ACL_EveryoneRW;
	srv->Node.ReadDir = TCP_Server_ReadDir;
	srv->Node.FindDir = TCP_Server_FindDir;
	srv->Node.IOCtl = TCP_Server_IOCtl;
	srv->Node.Close = TCP_Server_Close;

	LOCK(&glTCP_Listeners);
	srv->Next = gTCP_Listeners;
	gTCP_Listeners = srv;
	RELEASE(&glTCP_Listeners);

	return &srv->Node;
}

/**
 * \brief Wait for a new connection and return the connection ID
 * \note Blocks until a new connection is made
 * \param Node	Server node
 * \param Pos	Position (ignored)
 */
char *TCP_Server_ReadDir(tVFS_Node *Node, int Pos)
{
	tTCPListener	*srv = Node->ImplPtr;
	tTCPConnection	*conn;
	char	*ret;

	Log("[TCP  ] Thread %i waiting for a connection", Threads_GetTID());
	for(;;)
	{
		LOCK( &srv->lConnections );
		if( srv->NewConnections != NULL )	break;
		RELEASE( &srv->lConnections );
		Threads_Yield();
		continue;
	}
	

	// Increment the new list (the current connection is still on the 
	// normal list
	conn = srv->NewConnections;
	srv->NewConnections = conn->Next;
	
	RELEASE( &srv->lConnections );

	ret = malloc(9);
	itoa(ret, Node->ImplInt, 16, 8, '0');
	Log("TCP_Server_ReadDir: RETURN '%s'", ret);
	return ret;
}

/**
 * \brief Gets a client connection node
 * \param Node	Server node
 * \param Name	Hexadecimal ID of the node
 */
tVFS_Node *TCP_Server_FindDir(tVFS_Node *Node, char *Name)
{
	tTCPConnection	*conn;
	tTCPListener	*srv = Node->ImplPtr;
	char	tmp[9];
	 int	id = atoi(Name);
	
	// Sanity Check
	itoa(tmp, id, 16, 8, '0');
	if(strcmp(tmp, Name) != 0)	return NULL;
	
	// Search
	LOCK( &srv->lConnections );
	for(conn = srv->Connections;
		conn && conn->Node.ImplInt != id;
		conn = conn->Next);
	RELEASE( &srv->lConnections );
	
	// If not found, ret NULL
	if(!conn)	return NULL;
	
	// Return node
	return &conn->Node;
}

/**
 * \brief Handle IOCtl calls
 */
int TCP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tTCPListener	*srv = Node->ImplPtr;

	switch(ID)
	{
	case 4:	// Get/Set Port
		if(!Data)	// Get Port
			return srv->Port;

		if(srv->Port)	// Wait, you can't CHANGE the port
			return -1;

		if(!CheckMem(Data, sizeof(Uint16)))	// Sanity check
			return -1;

		// Permissions check
		if(Threads_GetUID() != 0
		&& *(Uint16*)Data != 0
		&& *(Uint16*)Data < 1024)
			return -1;

		// TODO: Check if a port is in use

		// Set Port
		srv->Port = *(Uint16*)Data;
		if(srv->Port == 0)	// Allocate a random port
			srv->Port = TCP_GetUnusedPort();
		else	// Else, mark this as used
			TCP_AllocatePort(srv->Port);
		
		Log("[TCP  ] Server %p listening on port %i", srv, srv->Port);
		
		return srv->Port;
	}
	return 0;
}

void TCP_Server_Close(tVFS_Node *Node)
{
	free(Node->ImplPtr);
}

// --- Client
/**
 * \brief Create a client node
 */
tVFS_Node *TCP_Client_Init(tInterface *Interface)
{
	tTCPConnection	*conn = malloc( sizeof(tTCPConnection) );

	conn->State = TCP_ST_CLOSED;
	conn->Interface = Interface;
	conn->LocalPort = 0;
	conn->RemotePort = 0;
	memset( &conn->RemoteIP, 0, sizeof(conn->RemoteIP) );

	conn->Node.ImplPtr = conn;
	conn->Node.NumACLs = 1;
	conn->Node.ACLs = &gVFS_ACL_EveryoneRW;
	conn->Node.Read = TCP_Client_Read;
	conn->Node.Write = TCP_Client_Write;
	conn->Node.IOCtl = TCP_Client_IOCtl;
	conn->Node.Close = TCP_Client_Close;

	LOCK(&glTCP_OutbountCons);
	conn->Next = gTCP_OutbountCons;
	gTCP_OutbountCons = conn;
	RELEASE(&glTCP_OutbountCons);

	return &conn->Node;
}

/**
 * \brief Wait for a packet and return it
 * \note If \a Length is smaller than the size of the packet, the rest
 *       of the packet's data will be discarded.
 */
Uint64 TCP_Client_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tTCPConnection	*conn = Node->ImplPtr;
	tTCPStoredPacket	*pkt;
	
	Log("TCP_Client_Read: (Length=%i)", Length);
	
	// Check if connection is open
	if( conn->State != TCP_ST_OPEN )	return 0;
	
	// Poll packets
	for(;;)
	{		
		// Lock list and check if there is a packet
		LOCK( &conn->lRecievedPackets );
		if( conn->RecievedPackets == NULL ) {
			// If not, release the lock, yield and try again
			RELEASE( &conn->lRecievedPackets );
			Threads_Yield();
			continue;
		}
		
		// Get packet pointer
		pkt = conn->RecievedPackets;
		conn->RecievedPackets = pkt->Next;
		// Release the lock (we don't need it any more)
		RELEASE( &conn->lRecievedPackets );
		
		Log("TCP_Client_Read: pkt->Length = %i", pkt->Length);
		
		// Copy Data
		if(Length > pkt->Length)	Length = pkt->Length;
		memcpy(Buffer, pkt->Data, Length);
		
		// Free packet and return
		free(pkt);
		return Length;
	}
}

Uint64 TCP_Client_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	return 0;
}

/**
 * \brief Control a client socket
 */
int TCP_Client_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tTCPConnection	*conn = Node->ImplPtr;

	switch(ID)
	{
	case 4:	// Get/Set local port
		if(!Data)
			return conn->LocalPort;
		if(conn->State != TCP_ST_CLOSED)
			return -1;
		if(!CheckMem(Data, sizeof(Uint16)))
			return -1;

		if(Threads_GetUID() != 0 && *(Uint16*)Data < 1024)
			return -1;

		conn->LocalPort = *(Uint16*)Data;
		return 0;

	case 5:	// Get/Set remote port
		if(!Data)	return conn->RemotePort;
		if(conn->State != TCP_ST_CLOSED)	return -1;
		if(!CheckMem(Data, sizeof(Uint16)))	return -1;
		conn->RemotePort = *(Uint16*)Data;
		return 0;

	case 6:	// Set Remote IP
		if( conn->State != TCP_ST_CLOSED )
			return -1;
		if( conn->Interface->Type == 4 )
		{
			if(!CheckMem(Data, sizeof(tIPv4)))	return -1;
			conn->RemoteIP.v4 = *(tIPv4*)Data;
		}
		else if( conn->Interface->Type == 6 )
		{
			if(!CheckMem(Data, sizeof(tIPv6)))	return -1;
			conn->RemoteIP.v6 = *(tIPv6*)Data;
		}
		return 0;

	case 7:	// Connect
		if(conn->LocalPort == -1)
			conn->LocalPort = TCP_GetUnusedPort();
		if(conn->RemotePort == -1)
			return 0;

		TCP_StartConnection(conn);
		return 1;
	}

	return 0;
}

void TCP_Client_Close(tVFS_Node *Node)
{
	free(Node->ImplPtr);
}

/*
 * Acess2 IP Stack
 * - TCP Handling
 */
#define DEBUG	1
#include "ipstack.h"
#include "ipv4.h"
#include "tcp.h"

#define USE_SELECT	1
#define	CACHE_FUTURE_PACKETS_IN_BYTES	1	// Use a ring buffer to cache out of order packets

#define TCP_MIN_DYNPORT	0xC000
#define TCP_MAX_HALFOPEN	1024	// Should be enough

#define TCP_MAX_PACKET_SIZE	1024
#define TCP_WINDOW_SIZE	0x2000
#define TCP_RECIEVE_BUFFER_SIZE	0x4000

// === PROTOTYPES ===
void	TCP_Initialise(void);
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
tVFS_Node	*TCP_Server_FindDir(tVFS_Node *Node, const char *Name);
 int	TCP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Server_Close(tVFS_Node *Node);
// --- Client
tVFS_Node	*TCP_Client_Init(tInterface *Interface);
Uint64	TCP_Client_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	TCP_Client_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	TCP_Client_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Client_Close(tVFS_Node *Node);
// --- Helpers
 int	WrapBetween(Uint32 Lower, Uint32 Value, Uint32 Higher, Uint32 MaxValue);

// === TEMPLATES ===
tSocketFile	gTCP_ServerFile = {NULL, "tcps", TCP_Server_Init};
tSocketFile	gTCP_ClientFile = {NULL, "tcpc", TCP_Client_Init};

// === GLOBALS ===
 int	giTCP_NumHalfopen = 0;
tShortSpinlock	glTCP_Listeners;
tTCPListener	*gTCP_Listeners;
tShortSpinlock	glTCP_OutbountCons;
tTCPConnection	*gTCP_OutbountCons;
Uint32	gaTCP_PortBitmap[0x800];
 int	giTCP_NextOutPort = TCP_MIN_DYNPORT;

// === CODE ===
/**
 * \brief Initialise the TCP Layer
 * 
 * Registers the client and server files and the GetPacket callback
 */
void TCP_Initialise(void)
{
	IPStack_AddFile(&gTCP_ServerFile);
	IPStack_AddFile(&gTCP_ClientFile);
	IPv4_RegisterCallback(IP4PROT_TCP, TCP_GetPacket);
}

/**
 * \brief Sends a packet from the specified connection, calculating the checksums
 * \param Conn	Connection
 * \param Length	Length of data
 * \param Data	Packet data (cast as a TCP Header)
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
		buf[0] = ((tIPv4*)Conn->Interface->Address)->L;
		buf[1] = Conn->RemoteIP.v4.L;
		buf[2] = (htons(Length)<<16) | (6<<8) | 0;
		Data->Checksum = 0;
		memcpy( &buf[3], Data, Length );
		Data->Checksum = htons( IPv4_Checksum( buf, buflen ) );
		free(buf);
		IPv4_SendPacket(Conn->Interface, Conn->RemoteIP.v4, IP4PROT_TCP, 0, Length, Data);
		break;
	}
}

/**
 * \brief Handles a packet from the IP Layer
 * \param Interface	Interface the packet arrived from
 * \param Address	Pointer to the addres structure
 * \param Length	Size of packet in bytes
 * \param Buffer	Packet data
 */
void TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tTCPHeader	*hdr = Buffer;
	tTCPListener	*srv;
	tTCPConnection	*conn;

	Log_Log("TCP", "TCP_GetPacket: SourcePort = %i, DestPort = %i",
		ntohs(hdr->SourcePort), ntohs(hdr->DestPort));
/*
	Log_Log("TCP", "TCP_GetPacket: SequenceNumber = 0x%x", ntohl(hdr->SequenceNumber));
	Log_Log("TCP", "TCP_GetPacket: AcknowlegementNumber = 0x%x", ntohl(hdr->AcknowlegementNumber));
	Log_Log("TCP", "TCP_GetPacket: DataOffset = %i", hdr->DataOffset >> 4);
	Log_Log("TCP", "TCP_GetPacket: WindowSize = %i", htons(hdr->WindowSize));
	Log_Log("TCP", "TCP_GetPacket: Checksum = 0x%x", htons(hdr->Checksum));
	Log_Log("TCP", "TCP_GetPacket: UrgentPointer = 0x%x", htons(hdr->UrgentPointer));
*/
	Log_Log("TCP", "TCP_GetPacket: Flags = %s%s%s%s%s%s%s%s",
		(hdr->Flags & TCP_FLAG_CWR) ? "CWR " : "",
		(hdr->Flags & TCP_FLAG_ECE) ? "ECE " : "",
		(hdr->Flags & TCP_FLAG_URG) ? "URG " : "",
		(hdr->Flags & TCP_FLAG_ACK) ? "ACK " : "",
		(hdr->Flags & TCP_FLAG_PSH) ? "PSH " : "",
		(hdr->Flags & TCP_FLAG_RST) ? "RST " : "",
		(hdr->Flags & TCP_FLAG_SYN) ? "SYN " : "",
		(hdr->Flags & TCP_FLAG_FIN) ? "FIN " : ""
		);

	if( Length > (hdr->DataOffset >> 4)*4 )
	{
		Log_Log("TCP", "TCP_GetPacket: SequenceNumber = 0x%x", ntohl(hdr->SequenceNumber));
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
			
			Log_Log("TCP", "TCP_GetPacket: Matches server %p", srv);
			// Is this in an established connection?
			for( conn = srv->Connections; conn; conn = conn->Next )
			{
				Log_Log("TCP", "TCP_GetPacket: conn->Interface(%p) == Interface(%p)",
					conn->Interface, Interface);
				// Check that it is coming in on the same interface
				if(conn->Interface != Interface)	continue;

				// Check Source Port
				Log_Log("TCP", "TCP_GetPacket: conn->RemotePort(%i) == hdr->SourcePort(%i)",
					conn->RemotePort, ntohs(hdr->SourcePort));
				if(conn->RemotePort != ntohs(hdr->SourcePort))	continue;

				// Check Source IP
				if(conn->Interface->Type == 6 && !IP6_EQU(conn->RemoteIP.v6, *(tIPv6*)Address))
					continue;
				if(conn->Interface->Type == 4 && !IP4_EQU(conn->RemoteIP.v4, *(tIPv4*)Address))
					continue;

				Log_Log("TCP", "TCP_GetPacket: Matches connection %p", conn);
				// We have a response!
				TCP_INT_HandleConnectionPacket(conn, hdr, Length);

				return;
			}

			Log_Log("TCP", "TCP_GetPacket: Opening Connection");
			// Open a new connection (well, check that it's a SYN)
			if(hdr->Flags != TCP_FLAG_SYN) {
				Log_Log("TCP", "TCP_GetPacket: Packet is not a SYN");
				return ;
			}
			
			// TODO: Check for halfopen max
			
			conn = calloc(1, sizeof(tTCPConnection));
			conn->State = TCP_ST_SYN_RCVD;
			conn->LocalPort = srv->Port;
			conn->RemotePort = ntohs(hdr->SourcePort);
			conn->Interface = Interface;
			
			switch(Interface->Type)
			{
			case 4:	conn->RemoteIP.v4 = *(tIPv4*)Address;	break;
			case 6:	conn->RemoteIP.v6 = *(tIPv6*)Address;	break;
			}
			
			conn->RecievedBuffer = RingBuffer_Create( TCP_RECIEVE_BUFFER_SIZE );
			
			conn->NextSequenceRcv = ntohl( hdr->SequenceNumber ) + 1;
			conn->NextSequenceSend = rand();
			
			// Create node
			conn->Node.NumACLs = 1;
			conn->Node.ACLs = &gVFS_ACL_EveryoneRW;
			conn->Node.ImplPtr = conn;
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
			SHORTLOCK(&srv->lConnections);
			if( !srv->Connections )
				srv->Connections = conn;
			else
				srv->ConnectionsTail->Next = conn;
			srv->ConnectionsTail = conn;
			if(!srv->NewConnections)
				srv->NewConnections = conn;
			SHORTREL(&srv->lConnections);

			// Send the SYN ACK
			hdr->Flags |= TCP_FLAG_ACK;
			hdr->AcknowlegementNumber = htonl(conn->NextSequenceRcv);
			hdr->SequenceNumber = htonl(conn->NextSequenceSend);
			hdr->DestPort = hdr->SourcePort;
			hdr->SourcePort = htons(srv->Port);
			hdr->DataOffset = (sizeof(tTCPHeader)/4) << 4;
			TCP_SendPacket( conn, sizeof(tTCPHeader), hdr );
			conn->NextSequenceSend ++;
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
	
	Log_Log("TCP", "TCP_GetPacket: No Match");
}

/**
 * \brief Handles a packet sent to a specific connection
 * \param Connection	TCP Connection pointer
 * \param Header	TCP Packet pointer
 * \param Length	Length of the packet
 */
void TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header, int Length)
{	
	tTCPStoredPacket	*pkt;
	 int	dataLen;
	
	// Silently drop once finished
	// TODO: Check if this needs to be here
	if( Connection->State == TCP_ST_FINISHED ) {
		Log_Log("TCP", "Packet ignored - connection finnished");
		return ;
	}
	
	// Syncronise sequence values
	if(Header->Flags & TCP_FLAG_SYN) {
		// TODO: What if the packet also has data?
		Connection->NextSequenceRcv = ntohl(Header->SequenceNumber);
	}
	
	// Ackowledge a sent packet
	if(Header->Flags & TCP_FLAG_ACK) {
		// TODO: Process an ACKed Packet
		Log_Log("TCP", "Conn %p, Sent packet 0x%x ACKed", Connection, Header->AcknowlegementNumber);
	}
	
	// Get length of data
	dataLen = Length - (Header->DataOffset>>4)*4;
	Log_Log("TCP", "HandleConnectionPacket - dataLen = %i", dataLen);
	
	// 
	// State Machine
	//
	switch( Connection->State )
	{
	// Pre-init conneciton?
	case TCP_ST_CLOSED:
		Log_Log("TCP", "Packets to a closed connection?!");
		break;
	
	// --- Init States ---
	// SYN sent, expecting SYN-ACK Connection Opening
	case TCP_ST_SYN_SENT:
		if( Header->Flags & TCP_FLAG_SYN )
		{
			Connection->NextSequenceRcv ++;
			Header->DestPort = Header->SourcePort;
			Header->SourcePort = htons(Connection->LocalPort);
			Header->AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
			Header->SequenceNumber = htonl(Connection->NextSequenceSend);
			Header->WindowSize = htons(TCP_WINDOW_SIZE);
			Header->Flags = TCP_FLAG_ACK;
			Header->DataOffset = (sizeof(tTCPHeader)/4) << 4;
			TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
			
			if( Header->Flags & TCP_FLAG_ACK )
			{	
				Log_Log("TCP", "ACKing SYN-ACK");
				Connection->State = TCP_ST_OPEN;
			}
			else
			{
				Log_Log("TCP", "ACKing SYN");
				Connection->State = TCP_ST_SYN_RCVD;
			}
		}
		break;
	
	// SYN-ACK sent, expecting ACK
	case TCP_ST_SYN_RCVD:
		if( Header->Flags & TCP_FLAG_ACK )
		{
			// TODO: Handle max half-open limit
			Connection->State = TCP_ST_OPEN;
			Log_Log("TCP", "Connection fully opened");
		}
		break;
		
	// --- Established State ---
	case TCP_ST_OPEN:
		// - Handle State changes
		//
		if( Header->Flags & TCP_FLAG_FIN ) {
			Log_Log("TCP", "Conn %p closed, recieved FIN", Connection);
			VFS_MarkError(&Connection->Node, 1);
			Connection->State = TCP_ST_CLOSE_WAIT;
//			Header->Flags &= ~TCP_FLAG_FIN;
			// CLOSE WAIT requires the client to close (or does it?)
			#if 0
			
			#endif
		}
	
		// Check for an empty packet
		if(dataLen == 0) {
			if( Header->Flags == TCP_FLAG_ACK )
			{
				Log_Log("TCP", "ACK only packet");
				return ;
			}
			Connection->NextSequenceRcv ++;	// TODO: Is this right? (empty packet counts as one byte)
			Log_Log("TCP", "Empty Packet, inc and ACK the current sequence number");
			Header->DestPort = Header->SourcePort;
			Header->SourcePort = htons(Connection->LocalPort);
			Header->AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
			Header->SequenceNumber = htonl(Connection->NextSequenceSend);
			Header->Flags |= TCP_FLAG_ACK;
			TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
			return ;
		}
		
		// NOTES:
		// Flags
		//    PSH - Has Data?
		// /NOTES
		
		// Allocate and fill cached packet
		pkt = malloc( sizeof(tTCPStoredPacket) + dataLen );
		pkt->Next = NULL;
		pkt->Sequence = ntohl(Header->SequenceNumber);
		pkt->Length = dataLen;
		memcpy(pkt->Data, (Uint8*)Header + (Header->DataOffset>>4)*4, dataLen);
		
		Log_Log("TCP", "0x%08x <= 0x%08x < 0x%08x",
			Connection->NextSequenceRcv,
			pkt->Sequence,
			Connection->NextSequenceRcv + TCP_WINDOW_SIZE
			);
		
		// Is this packet the next expected packet?
		if( pkt->Sequence == Connection->NextSequenceRcv )
		{
			// Ooh, Goodie! Add it to the recieved list
			TCP_INT_AppendRecieved(Connection, pkt);
			free(pkt);
			Log_Log("TCP", "0x%08x += %i", Connection->NextSequenceRcv, dataLen);
			Connection->NextSequenceRcv += dataLen;
			
			// TODO: This should be moved out of the watcher thread,
			// so that a single lost packet on one connection doesn't cause
			// all connections on the interface to lag.
			TCP_INT_UpdateRecievedFromFuture(Connection);
		
			// ACK Packet
			Header->DestPort = Header->SourcePort;
			Header->SourcePort = htons(Connection->LocalPort);
			Header->AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
			Header->SequenceNumber = htonl(Connection->NextSequenceSend);
			Header->WindowSize = htons(TCP_WINDOW_SIZE);
			Header->Flags &= TCP_FLAG_SYN;	// Eliminate all flags save for SYN
			Header->Flags |= TCP_FLAG_ACK;	// Add ACK
			Log_Log("TCP", "Sending ACK for 0x%08x", Connection->NextSequenceRcv);
			TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
			//Connection->NextSequenceSend ++;
		}
		// Check if the packet is in window
		else if( WrapBetween(Connection->NextSequenceRcv, pkt->Sequence,
				Connection->NextSequenceRcv+TCP_WINDOW_SIZE, 0xFFFFFFFF) )
		{
			#if CACHE_FUTURE_PACKETS_IN_BYTES
			Uint32	index;
			 int	i;
			
			index = pkt->Sequence % TCP_WINDOW_SIZE;
			for( i = 0; i < pkt->Length; i ++ )
			{
				Connection->FuturePacketValidBytes[index/8] |= 1 << (index%8);
				Connection->FuturePacketValidBytes[index] = pkt->Data[i];
				// Do a wrap increment
				index ++;
				if(index == TCP_WINDOW_SIZE)	index = 0;
			}
			#else
			tTCPStoredPacket	*tmp, *prev = NULL;
			
			Log_Log("TCP", "We missed a packet, caching",
				pkt->Sequence, Connection->NextSequenceRcv);
			
			// No? Well, let's cache it and look at it later
			SHORTLOCK( &Connection->lFuturePackets );
			for(tmp = Connection->FuturePackets;
				tmp;
				prev = tmp, tmp = tmp->Next)
			{
				if(tmp->Sequence >= pkt->Sequence)	break;
			}
			
			// Add if before first, or sequences don't match 
			if( !tmp || tmp->Sequence != pkt->Sequence )
			{
				if(prev)
					prev->Next = pkt;
				else
					Connection->FuturePackets = pkt;
				pkt->Next = tmp;
			}
			// Replace if larger
			else if(pkt->Length > tmp->Length)
			{
				if(prev)
					prev->Next = pkt;
				pkt->Next = tmp->Next;
				free(tmp);
			}
			else
			{
				free(pkt);
			}
			SHORTREL( &Connection->lFuturePackets );
			#endif
		}
		// Badly out of sequence packet
		else
		{
			Log_Log("TCP", "Fully out of sequence packet (0x%08x not between 0x%08x and 0x%08x), dropped",
				pkt->Sequence, Connection->NextSequenceRcv, Connection->NextSequenceRcv+TCP_WINDOW_SIZE);
			free(pkt);
			// TODO: Spec says we should send an empty ACK with the current state
		}
		break;
	
	// --- Remote close states
	case TCP_ST_CLOSE_WAIT:
		
		// Ignore everything, CLOSE_WAIT is terminated by the client
		Log_Debug("TCP", "CLOSE WAIT - Ignoring packets");
		
		break;
	
	// LAST-ACK - Waiting for the ACK of FIN (from CLOSE WAIT)
	case TCP_ST_LAST_ACK:
		if( Header->Flags & TCP_FLAG_ACK )
		{
			Connection->State = TCP_ST_FINISHED;	// Connection completed
			Log_Log("TCP", "LAST-ACK to CLOSED - Connection remote closed");
			// TODO: Destrory the TCB
		}
		break;
	
	// --- Local close States
	case TCP_ST_FIN_WAIT1:
		if( Header->Flags & TCP_FLAG_FIN )
		{
			Connection->State = TCP_ST_CLOSING;
			Log_Debug("TCP", "Conn %p closed, sent FIN and recieved FIN", Connection);
			VFS_MarkError(&Connection->Node, 1);
			
			// ACK Packet
			Header->DestPort = Header->SourcePort;
			Header->SourcePort = htons(Connection->LocalPort);
			Header->AcknowlegementNumber = Header->SequenceNumber;
			Header->SequenceNumber = htonl(Connection->NextSequenceSend);
			Header->WindowSize = htons(TCP_WINDOW_SIZE);
			Header->Flags = TCP_FLAG_ACK;
			TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
			break ;
		}
		
		// TODO: Make sure that the packet is actually ACKing the FIN
		if( Header->Flags & TCP_FLAG_ACK )
		{
			Connection->State = TCP_ST_FIN_WAIT2;
			Log_Debug("TCP", "Conn %p closed, sent FIN ACKed", Connection);
			VFS_MarkError(&Connection->Node, 1);
			return ;
		}
		break;
	
	case TCP_ST_FIN_WAIT2:
		if( Header->Flags & TCP_FLAG_FIN )
		{
			Connection->State = TCP_ST_TIME_WAIT;
			Log_Debug("TCP", "FIN sent and recieved, ACKing and going into TIME WAIT %p FINWAIT-2 -> TIME WAIT", Connection);
			// Send ACK
			Header->DestPort = Header->SourcePort;
			Header->SourcePort = htons(Connection->LocalPort);
			Header->AcknowlegementNumber = Header->SequenceNumber;
			Header->SequenceNumber = htonl(Connection->NextSequenceSend);
			Header->WindowSize = htons(TCP_WINDOW_SIZE);
			Header->Flags = TCP_FLAG_ACK;
			TCP_SendPacket( Connection, sizeof(tTCPHeader), Header );
		}
		break;
	
	case TCP_ST_CLOSING:
		// TODO: Make sure that the packet is actually ACKing the FIN
		if( Header->Flags & TCP_FLAG_ACK )
		{
			Connection->State = TCP_ST_TIME_WAIT;
			Log_Debug("TCP", "Conn %p CLOSING -> TIME WAIT", Connection);
			VFS_MarkError(&Connection->Node, 1);
			return ;
		}
		break;
	
	// --- Closed (or near closed) states) ---
	case TCP_ST_TIME_WAIT:
		Log_Log("TCP", "Packets on Time-Wait, ignored");
		break;
	
	case TCP_ST_FINISHED:
		Log_Log("TCP", "Packets when CLOSED, ignoring");
		break;
	
	//default:
	//	Log_Warning("TCP", "Unhandled TCP state %i", Connection->State);
	//	break;
	}
	
}

/**
 * \brief Appends a packet to the recieved list
 * \param Connection	Connection structure
 * \param Pkt	Packet structure on heap
 */
void TCP_INT_AppendRecieved(tTCPConnection *Connection, tTCPStoredPacket *Pkt)
{
	Mutex_Acquire( &Connection->lRecievedPackets );
	if(Connection->RecievedBuffer->Length + Pkt->Length > Connection->RecievedBuffer->Space )
	{
		Log_Error("TCP", "Buffer filled, packet dropped (%s)",
		//	TCP_INT_DumpConnection(Connection)
			""
			);
		return ;
	}
	
	RingBuffer_Write( Connection->RecievedBuffer, Pkt->Data, Pkt->Length );

	#if USE_SELECT
	VFS_MarkAvaliable(&Connection->Node, 1);
	#endif
	
	Mutex_Release( &Connection->lRecievedPackets );
}

/**
 * \brief Updates the connections recieved list from the future list
 * \param Connection	Connection structure
 * 
 * Updates the recieved packets list with packets from the future (out 
 * of order) packets list that are now able to be added in direct
 * sequence.
 */
void TCP_INT_UpdateRecievedFromFuture(tTCPConnection *Connection)
{
	#if CACHE_FUTURE_PACKETS_IN_BYTES
	 int	i, length = 0;
	Uint32	index;
	
	// Calculate length of contiguous bytes
	length = Connection->HighestSequenceRcvd - Connection->NextSequenceRcv;
	index = Connection->NextSequenceRcv % TCP_WINDOW_SIZE;
	for( i = 0; i < length; i ++ )
	{
		if( Connection->FuturePacketValidBytes[i / 8] == 0xFF ) {
			i += 7;	index += 7;
			continue;
		}
		else if( !(Connection->FuturePacketValidBytes[i / 8] & (1 << (i%8))) )
			break;
		
		index ++;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
	}
	length = i;
	
	index = Connection->NextSequenceRcv % TCP_WINDOW_SIZE;
	
	// Write data to to the ring buffer
	if( TCP_WINDOW_SIZE - index > length )
	{
		// Simple case
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData + index, length );
	}
	else
	{
		 int	endLen = TCP_WINDOW_SIZE - index;
		// 2-part case
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData + index, endLen );
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData, endLen - length );
	}
	
	// Mark (now saved) bytes as invalid
	// - Align index
	while(index % 8 && length)
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketData[index/8] &= ~(1 << (index%8));
		index ++;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
		length --;
	}
	while( length > 7 )
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketValidBytes[index/8] = 0;
		length -= 8;
		index += 8;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
	}
	while(length)
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketData[index/8] &= ~(1 << (index%8));
		index ++;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
		length --;
	}
	
	#else
	tTCPStoredPacket	*pkt;
	for(;;)
	{
		SHORTLOCK( &Connection->lFuturePackets );
		
		// Clear out duplicates from cache
		// - If a packet has just been recieved, and it is expected, then
		//   (since NextSequenceRcv = rcvd->Sequence + rcvd->Length) all
		//   packets in cache that are smaller than the next expected
		//   are now defunct.
		pkt = Connection->FuturePackets;
		while(pkt && pkt->Sequence < Connection->NextSequenceRcv)
		{
			tTCPStoredPacket	*next = pkt->Next;
			free(pkt);
			pkt = next;
		}
		
		// If there's no packets left in cache, stop looking
		if(!pkt || pkt->Sequence > Connection->NextSequenceRcv) {
			SHORTREL( &Connection->lFuturePackets );
			return;
		}
		
		// Delete packet from future list
		Connection->FuturePackets = pkt->Next;
		
		// Release list
		SHORTREL( &Connection->lFuturePackets );
		
		// Looks like we found one
		TCP_INT_AppendRecieved(Connection, pkt);
		Connection->NextSequenceRcv += pkt->Length;
		free(pkt);
	}
	#endif
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
	tTCPListener	*srv;
	
	srv = malloc( sizeof(tTCPListener) );

	if( srv == NULL ) {
		Log_Warning("TCP", "malloc failed for listener (%i) bytes", sizeof(tTCPListener));
		return NULL;
	}

	srv->Interface = Interface;
	srv->Port = 0;
	srv->NextID = 0;
	srv->Connections = NULL;
	srv->ConnectionsTail = NULL;
	srv->NewConnections = NULL;
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

	SHORTLOCK(&glTCP_Listeners);
	srv->Next = gTCP_Listeners;
	gTCP_Listeners = srv;
	SHORTREL(&glTCP_Listeners);

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
	
	ENTER("pNode iPos", Node, Pos);

	Log_Log("TCP", "Thread %i waiting for a connection", Threads_GetTID());
	for(;;)
	{
		SHORTLOCK( &srv->lConnections );
		if( srv->NewConnections != NULL )	break;
		SHORTREL( &srv->lConnections );
		Threads_Yield();	// TODO: Sleep until poked
		continue;
	}
	

	// Increment the new list (the current connection is still on the 
	// normal list)
	conn = srv->NewConnections;
	srv->NewConnections = conn->Next;
	
	SHORTREL( &srv->lConnections );
	
	LOG("conn = %p", conn);
	LOG("srv->Connections = %p", srv->Connections);
	LOG("srv->NewConnections = %p", srv->NewConnections);
	LOG("srv->ConnectionsTail = %p", srv->ConnectionsTail);

	ret = malloc(9);
	itoa(ret, conn->Node.ImplInt, 16, 8, '0');
	Log_Log("TCP", "Thread %i got '%s'", Threads_GetTID(), ret);
	LEAVE('s', ret);
	return ret;
}

/**
 * \brief Gets a client connection node
 * \param Node	Server node
 * \param Name	Hexadecimal ID of the node
 */
tVFS_Node *TCP_Server_FindDir(tVFS_Node *Node, const char *Name)
{
	tTCPConnection	*conn;
	tTCPListener	*srv = Node->ImplPtr;
	char	tmp[9];
	 int	id = atoi(Name);
	
	ENTER("pNode sName", Node, Name);
	
	// Sanity Check
	itoa(tmp, id, 16, 8, '0');
	if(strcmp(tmp, Name) != 0) {
		LOG("'%s' != '%s' (%08x)", Name, tmp, id);
		LEAVE('n');
		return NULL;
	}
	
	Log_Debug("TCP", "srv->Connections = %p", srv->Connections);
	Log_Debug("TCP", "srv->NewConnections = %p", srv->NewConnections);
	Log_Debug("TCP", "srv->ConnectionsTail = %p", srv->ConnectionsTail);
	
	// Search
	SHORTLOCK( &srv->lConnections );
	for(conn = srv->Connections;
		conn;
		conn = conn->Next)
	{
		LOG("conn->Node.ImplInt = %i", conn->Node.ImplInt);
		if(conn->Node.ImplInt == id)	break;
	}
	SHORTREL( &srv->lConnections );
	
	// If not found, ret NULL
	if(!conn) {
		LOG("Connection %i not found", id);
		LEAVE('n');
		return NULL;
	}
	
	// Return node
	LEAVE('p', &conn->Node);
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
		
		Log_Log("TCP", "Server %p listening on port %i", srv, srv->Port);
		
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
	tTCPConnection	*conn = calloc( sizeof(tTCPConnection) + TCP_WINDOW_SIZE + TCP_WINDOW_SIZE/8, 1 );

	conn->State = TCP_ST_CLOSED;
	conn->Interface = Interface;
	conn->LocalPort = -1;
	conn->RemotePort = -1;

	conn->Node.ImplPtr = conn;
	conn->Node.NumACLs = 1;
	conn->Node.ACLs = &gVFS_ACL_EveryoneRW;
	conn->Node.Read = TCP_Client_Read;
	conn->Node.Write = TCP_Client_Write;
	conn->Node.IOCtl = TCP_Client_IOCtl;
	conn->Node.Close = TCP_Client_Close;

	conn->RecievedBuffer = RingBuffer_Create( TCP_RECIEVE_BUFFER_SIZE );
	#if 0
	conn->SentBuffer = RingBuffer_Create( TCP_SEND_BUFFER_SIZE );
	Semaphore_Init(conn->SentBufferSpace, 0, TCP_SEND_BUFFER_SIZE, "TCP SentBuffer", conn->Name);
	#endif
	
	#if CACHE_FUTURE_PACKETS_IN_BYTES
	// Future recieved data (ahead of the expected sequence number)
	conn->FuturePacketData = (Uint8*)conn + sizeof(tTCPConnection);
	conn->FuturePacketValidBytes = conn->FuturePacketData + TCP_WINDOW_SIZE;
	#endif

	SHORTLOCK(&glTCP_OutbountCons);
	conn->Next = gTCP_OutbountCons;
	gTCP_OutbountCons = conn;
	SHORTREL(&glTCP_OutbountCons);

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
	size_t	len;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	LOG("conn = %p {State:%i}", conn, conn->State);
	
	// Check if connection is estabilishing
	// - TODO: Sleep instead (maybe using VFS_SelectNode to wait for the
	//   data to be availiable
	while( conn->State == TCP_ST_SYN_RCVD || conn->State == TCP_ST_SYN_SENT )
		Threads_Yield();
	
	// If the conneciton is not open, then clean out the recieved buffer
	if( conn->State != TCP_ST_OPEN )
	{
		Mutex_Acquire( &conn->lRecievedPackets );
		len = RingBuffer_Read( Buffer, conn->RecievedBuffer, Length );
		Mutex_Release( &conn->lRecievedPackets );
		
		if( len == 0 ) {
			VFS_MarkAvaliable(Node, 0);
			LEAVE('i', -1);
			return -1;
		}
		
		LEAVE('i', len);
		return len;
	}
	
	// Wait
	VFS_SelectNode(Node, VFS_SELECT_READ|VFS_SELECT_ERROR, NULL, "TCP_Client_Read");
	
	// Lock list and read as much as possible (up to `Length`)
	Mutex_Acquire( &conn->lRecievedPackets );
	len = RingBuffer_Read( Buffer, conn->RecievedBuffer, Length );
	
	if( len == 0 || conn->RecievedBuffer->Length == 0 ) {
		LOG("Marking as none avaliable (len = %i)", len);
		VFS_MarkAvaliable(Node, 0);
	}
		
	// Release the lock (we don't need it any more)
	Mutex_Release( &conn->lRecievedPackets );

	LEAVE('i', len);
	return len;
}

/**
 * \brief Send a data packet on a connection
 */
void TCP_INT_SendDataPacket(tTCPConnection *Connection, size_t Length, void *Data)
{
	char	buf[sizeof(tTCPHeader)+Length];
	tTCPHeader	*packet = (void*)buf;
	
	packet->SourcePort = htons(Connection->LocalPort);
	packet->DestPort = htons(Connection->RemotePort);
	packet->DataOffset = (sizeof(tTCPHeader)/4)*16;
	packet->WindowSize = TCP_WINDOW_SIZE;
	
	packet->AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
	packet->SequenceNumber = htonl(Connection->NextSequenceSend);
	packet->Flags = TCP_FLAG_PSH|TCP_FLAG_ACK;	// Hey, ACK if you can!
	
	memcpy(packet->Options, Data, Length);
	
	Log_Debug("TCP", "Send sequence 0x%08x", Connection->NextSequenceSend);
	Debug_HexDump("[TCP     ] TCP_INT_SendDataPacket: Data = ",
		Data, Length);
	
	TCP_SendPacket( Connection, sizeof(tTCPHeader)+Length, packet );
	
	Connection->NextSequenceSend += Length;
}

/**
 * \brief Send some bytes on a connection
 */
Uint64 TCP_Client_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tTCPConnection	*conn = Node->ImplPtr;
	size_t	rem = Length;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
//	#if DEBUG
//	Debug_HexDump("TCP_Client_Write: Buffer = ",
//		Buffer, Length);
//	#endif
	
	// Check if connection is open
	while( conn->State == TCP_ST_SYN_RCVD || conn->State == TCP_ST_SYN_SENT )
		Threads_Yield();
	
	if( conn->State != TCP_ST_OPEN ) {
		VFS_MarkError(Node, 1);
		LEAVE('i', -1);
		return -1;
	}
	
	do
	{
		 int	len = (rem < TCP_MAX_PACKET_SIZE) ? rem : TCP_MAX_PACKET_SIZE;
		
		#if 0
		// Wait for space in the buffer
		Semaphore_Signal( &Connection->SentBufferSpace, len );
		
		// Save data to buffer (and update the length read by the ammount written)
		len = RingBuffer_Write( &Connection->SentBuffer, Buffer, len);
		#endif
		
		// Send packet
		TCP_INT_SendDataPacket(conn, len, Buffer);
		
		Buffer += len;
		rem -= len;
	} while( rem > 0 );
	
	LEAVE('i', Length);
	return Length;
}

/**
 * \brief Open a connection to another host using TCP
 * \param Conn	Connection structure
 */
void TCP_StartConnection(tTCPConnection *Conn)
{
	tTCPHeader	hdr = {0};

	Conn->State = TCP_ST_SYN_SENT;

	hdr.SourcePort = htons(Conn->LocalPort);
	hdr.DestPort = htons(Conn->RemotePort);
	Conn->NextSequenceSend = rand();
	hdr.SequenceNumber = htonl(Conn->NextSequenceSend);
	hdr.DataOffset = (sizeof(tTCPHeader)/4) << 4;
	hdr.Flags = TCP_FLAG_SYN;
	hdr.WindowSize = htons(TCP_WINDOW_SIZE);	// Max
	hdr.Checksum = 0;	// TODO
	
	TCP_SendPacket( Conn, sizeof(tTCPHeader), &hdr );
	
	Conn->NextSequenceSend ++;
	Conn->State = TCP_ST_SYN_SENT;

	return ;
}

/**
 * \brief Control a client socket
 */
int TCP_Client_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tTCPConnection	*conn = Node->ImplPtr;
	
	ENTER("pNode iID pData", Node, ID, Data);

	switch(ID)
	{
	case 4:	// Get/Set local port
		if(!Data)
			LEAVE_RET('i', conn->LocalPort);
		if(conn->State != TCP_ST_CLOSED)
			LEAVE_RET('i', -1);
		if(!CheckMem(Data, sizeof(Uint16)))
			LEAVE_RET('i', -1);

		if(Threads_GetUID() != 0 && *(Uint16*)Data < 1024)
			LEAVE_RET('i', -1);

		conn->LocalPort = *(Uint16*)Data;
		LEAVE_RET('i', conn->LocalPort);

	case 5:	// Get/Set remote port
		if(!Data)	LEAVE_RET('i', conn->RemotePort);
		if(conn->State != TCP_ST_CLOSED)	LEAVE_RET('i', -1);
		if(!CheckMem(Data, sizeof(Uint16)))	LEAVE_RET('i', -1);
		conn->RemotePort = *(Uint16*)Data;
		LEAVE_RET('i', conn->RemotePort);

	case 6:	// Set Remote IP
		if( conn->State != TCP_ST_CLOSED )
			LEAVE_RET('i', -1);
		if( conn->Interface->Type == 4 )
		{
			if(!CheckMem(Data, sizeof(tIPv4)))	LEAVE_RET('i', -1);
			conn->RemoteIP.v4 = *(tIPv4*)Data;
		}
		else if( conn->Interface->Type == 6 )
		{
			if(!CheckMem(Data, sizeof(tIPv6)))	LEAVE_RET('i', -1);
			conn->RemoteIP.v6 = *(tIPv6*)Data;
		}
		LEAVE_RET('i', 0);

	case 7:	// Connect
		if(conn->LocalPort == 0xFFFF)
			conn->LocalPort = TCP_GetUnusedPort();
		if(conn->RemotePort == -1)
			LEAVE_RET('i', 0);

		{
			tTime	timeout_end = now() + conn->Interface->TimeoutDelay;
	
			TCP_StartConnection(conn);
			// TODO: Wait for connection to open
			while( conn->State == TCP_ST_SYN_SENT && timeout_end > now() ) {
				Threads_Yield();
			}
			if( conn->State == TCP_ST_SYN_SENT )
				LEAVE_RET('i', 0);
		}

		LEAVE_RET('i', 1);
	
	// Get recieve buffer length
	case 8:
		LEAVE_RET('i', conn->RecievedBuffer->Length);
	}

	return 0;
}

void TCP_Client_Close(tVFS_Node *Node)
{
	tTCPConnection	*conn = Node->ImplPtr;
	tTCPHeader	packet;
	
	ENTER("pNode", Node);
	
	if( conn->State == TCP_ST_CLOSE_WAIT || conn->State == TCP_ST_OPEN )
	{
		packet.SourcePort = htons(conn->LocalPort);
		packet.DestPort = htons(conn->RemotePort);
		packet.DataOffset = (sizeof(tTCPHeader)/4)*16;
		packet.WindowSize = TCP_WINDOW_SIZE;
		
		packet.AcknowlegementNumber = 0;
		packet.SequenceNumber = htonl(conn->NextSequenceSend);
		packet.Flags = TCP_FLAG_FIN;
		
		TCP_SendPacket( conn, sizeof(tTCPHeader), &packet );
	}
	
	switch( conn->State )
	{
	case TCP_ST_CLOSE_WAIT:
		conn->State = TCP_ST_LAST_ACK;
		break;
	case TCP_ST_OPEN:
		conn->State = TCP_ST_FIN_WAIT1;
		while( conn->State == TCP_ST_FIN_WAIT1 )	Threads_Yield();
		break;
	default:
		Log_Warning("TCP", "Unhandled connection state in TCP_Client_Close");
		break;
	}
	
	free(conn);
	
	LEAVE('-');
}

/**
 * \brief Checks if a value is between two others (after taking into account wrapping)
 */
int WrapBetween(Uint32 Lower, Uint32 Value, Uint32 Higher, Uint32 MaxValue)
{
	if( MaxValue < 0xFFFFFFFF )
	{
		Lower %= MaxValue + 1;
		Value %= MaxValue + 1;
		Higher %= MaxValue + 1;
	}
	
	// Simple Case, no wrap ?
	//       Lower Value Higher
	// | ... + ... + ... + ... |

	if( Lower < Higher ) {
		return Lower < Value && Value < Higher;
	}
	// Higher has wrapped below lower
	
	// Value > Lower ?
	//       Higher Lower Value
	// | ... +  ... + ... + ... |
	if( Value > Lower ) {
		return 1;
	}
	
	// Value < Higher ?
	//       Value Higher Lower
	// | ... + ... +  ... + ... |
	if( Value < Higher ) {
		return 1;
	}
	
	return 0;
}

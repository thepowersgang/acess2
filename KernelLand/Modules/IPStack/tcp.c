/*
 * Acess2 IP Stack
 * - TCP Handling
 */
#define DEBUG	0
#include "ipstack.h"
#include "ipv4.h"
#include "ipv6.h"
#include "tcp.h"

#define HEXDUMP_INCOMING	0
#define HEXDUMP_OUTGOING	0

#define TCP_MIN_DYNPORT	0xC000
#define TCP_MAX_HALFOPEN	1024	// Should be enough

#define TCP_MAX_PACKET_SIZE	1024
#define TCP_WINDOW_SIZE	0x2000
#define TCP_RECIEVE_BUFFER_SIZE	0x8000
#define TCP_DACK_THRESHOLD	4096
#define TCP_DACK_TIMEOUT	100

#define TCP_DEBUG	0	// Set to non-0 to enable TCP packet logging

// === PROTOTYPES ===
void	TCP_Initialise(void);
void	TCP_StartConnection(tTCPConnection *Conn);
void	TCP_SendPacket(tTCPConnection *Conn, tTCPHeader *Header, size_t DataLen, const void *Data);
void	TCP_int_SendPacket(tInterface *Interface, const void *Dest, tTCPHeader *Header, size_t Length, const void *Data);
void	TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
 int	TCP_INT_HandleServerPacket(tInterface *Interface, tTCPListener *Server, const void *Address, tTCPHeader *Header, size_t Length);
 int	TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header, int Length);
int	TCP_INT_AppendRecieved(tTCPConnection *Connection, const void *Data, size_t Length);
void	TCP_INT_UpdateRecievedFromFuture(tTCPConnection *Connection);
void	TCP_int_SendDelayedACK(void *ConnPtr);
void	TCP_INT_SendACK(tTCPConnection *Connection, const char *Reason);
Uint16	TCP_GetUnusedPort();
 int	TCP_AllocatePort(Uint16 Port);
 int	TCP_DeallocatePort(Uint16 Port);
tTCPConnection	*TCP_int_CreateConnection(tInterface *Interface, enum eTCPConnectionState State);
void	TCP_int_FreeTCB(tTCPConnection *Connection);
// --- Server
tVFS_Node	*TCP_Server_Init(tInterface *Interface);
 int	TCP_Server_ReadDir(tVFS_Node *Node, int Pos, char Name[FILENAME_MAX]);
tVFS_Node	*TCP_Server_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
 int	TCP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Server_Close(tVFS_Node *Node);
// --- Client
tVFS_Node	*TCP_Client_Init(tInterface *Interface);
size_t	TCP_Client_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	TCP_Client_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	TCP_Client_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	TCP_Client_Close(tVFS_Node *Node);
// --- Helpers
 int	WrapBetween(Uint32 Lower, Uint32 Value, Uint32 Higher, Uint32 MaxValue);
Uint32	GetRelative(Uint32 Base, Uint32 Value);

// === TEMPLATES ===
tSocketFile	gTCP_ServerFile = {NULL, "tcps", TCP_Server_Init};
tSocketFile	gTCP_ClientFile = {NULL, "tcpc", TCP_Client_Init};
tVFS_NodeType	gTCP_ServerNodeType = {
	.TypeName = "TCP Server",
	.ReadDir = TCP_Server_ReadDir,
	.FindDir = TCP_Server_FindDir,
	.IOCtl   = TCP_Server_IOCtl,
	.Close   = TCP_Server_Close
	};
tVFS_NodeType	gTCP_ClientNodeType = {
	.TypeName = "TCP Client/Connection",
	.Flags = VFS_NODETYPEFLAG_STREAM,
	.Read  = TCP_Client_Read,
	.Write = TCP_Client_Write,
	.IOCtl = TCP_Client_IOCtl,
	.Close = TCP_Client_Close
	};

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
	giTCP_NextOutPort += rand()%128;
	IPStack_AddFile(&gTCP_ServerFile);
	IPStack_AddFile(&gTCP_ClientFile);
	IPv4_RegisterCallback(IP4PROT_TCP, TCP_GetPacket);
	IPv6_RegisterCallback(IP4PROT_TCP, TCP_GetPacket);
}

/**
 * \brief Sends a packet from the specified connection, calculating the checksums
 * \param Conn	Connection
 * \param Length	Length of data
 * \param Data	Packet data (cast as a TCP Header)
 */
void TCP_SendPacket( tTCPConnection *Conn, tTCPHeader *Header, size_t Length, const void *Data )
{
	TCP_int_SendPacket(Conn->Interface, &Conn->RemoteIP, Header, Length, Data);
}

Uint16 TCP_int_CalculateChecksum(int AddrType, const void *LAddr, const void *RAddr,
	size_t HeaderLength, const tTCPHeader *Header, size_t DataLength, const void *Data)
{
	size_t packlen = HeaderLength + DataLength;
	Uint16	checksum[3];

	switch(AddrType)
	{
	case 4: {
		Uint32	buf[3];
		buf[0] = ((tIPv4*)LAddr)->L;
		buf[1] = ((tIPv4*)RAddr)->L;
		buf[2] = htonl( (packlen) | (IP4PROT_TCP<<16) | (0<<24) );
		checksum[0] = htons( ~IPv4_Checksum(buf, sizeof(buf)) );	// Partial checksum
		break; }
	case 6: {
		Uint32	buf[4+4+1+1];
		memcpy(&buf[0], LAddr, 16);
		memcpy(&buf[4], RAddr, 16);
		buf[8] = htonl(packlen);
		buf[9] = htonl(IP4PROT_TCP);
		checksum[0] = htons( ~IPv4_Checksum(buf, sizeof(buf)) );	// Partial checksum
		break; }
	default:
		return 0;
	}
	checksum[1] = htons( ~IPv4_Checksum(Header, HeaderLength) );
	checksum[2] = htons( ~IPv4_Checksum(Data, DataLength) );

	return htons( IPv4_Checksum(checksum, sizeof(checksum)) );
}

void TCP_int_SendPacket(tInterface *Interface, const void *Dest, tTCPHeader *Header, size_t Length, const void *Data )
{
	tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(2 + IPV4_BUFFERS);
	if( Length > 0 )
		IPStack_Buffer_AppendSubBuffer(buffer, Length, 0, Data, NULL, NULL);
	IPStack_Buffer_AppendSubBuffer(buffer, sizeof(*Header), 0, Header, NULL, NULL);

	#if TCP_DEBUG
	Log_Log("TCP", "TCP_int_SendPacket: <Local>:%i to [%s]:%i (%i data), Flags = %s%s%s%s%s%s%s%s",
		ntohs(Header->SourcePort),
		IPStack_PrintAddress(Interface->Type, Dest),
		ntohs(Header->DestPort),
		Length,
		(Header->Flags & TCP_FLAG_CWR) ? "CWR " : "",
		(Header->Flags & TCP_FLAG_ECE) ? "ECE " : "",
		(Header->Flags & TCP_FLAG_URG) ? "URG " : "",
		(Header->Flags & TCP_FLAG_ACK) ? "ACK " : "",
		(Header->Flags & TCP_FLAG_PSH) ? "PSH " : "",
		(Header->Flags & TCP_FLAG_RST) ? "RST " : "",
		(Header->Flags & TCP_FLAG_SYN) ? "SYN " : "",
		(Header->Flags & TCP_FLAG_FIN) ? "FIN " : ""
		);
	#endif

	Header->Checksum = 0;
	Header->Checksum = TCP_int_CalculateChecksum(Interface->Type, Interface->Address, Dest,
		sizeof(tTCPHeader), Header, Length, Data);
	
	// TODO: Fragment packet
	
	switch( Interface->Type )
	{
	case 4:
		IPv4_SendPacket(Interface, *(tIPv4*)Dest, IP4PROT_TCP, 0, buffer);
		break;
	case 6:
		IPv6_SendPacket(Interface, *(tIPv6*)Dest, IP4PROT_TCP, buffer);
		break;
	}
}

void TCP_int_SendRSTTo(tInterface *Interface, const void *Address, size_t Length, const tTCPHeader *Header)
{
	tTCPHeader	out_hdr = {0};
	
	out_hdr.DataOffset = (sizeof(out_hdr)/4) << 4;
	out_hdr.DestPort = Header->SourcePort;
	out_hdr.SourcePort = Header->DestPort;

	size_t	data_len = Length - (Header->DataOffset>>4)*4;
	out_hdr.AcknowlegementNumber = htonl( ntohl(Header->SequenceNumber) + data_len );
	if( Header->Flags & TCP_FLAG_ACK ) {
		out_hdr.Flags = TCP_FLAG_RST;
		out_hdr.SequenceNumber = Header->AcknowlegementNumber;
	}
	else {
		out_hdr.Flags = TCP_FLAG_RST|TCP_FLAG_ACK;
		out_hdr.SequenceNumber = 0;
	}
	TCP_int_SendPacket(Interface, Address, &out_hdr, 0, NULL);
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

	#if TCP_DEBUG
	Log_Log("TCP", "TCP_GetPacket: <Local>:%i from [%s]:%i, Flags = %s%s%s%s%s%s%s%s",
		ntohs(hdr->DestPort),
		IPStack_PrintAddress(Interface->Type, Address),
		ntohs(hdr->SourcePort),
		(hdr->Flags & TCP_FLAG_CWR) ? "CWR " : "",
		(hdr->Flags & TCP_FLAG_ECE) ? "ECE " : "",
		(hdr->Flags & TCP_FLAG_URG) ? "URG " : "",
		(hdr->Flags & TCP_FLAG_ACK) ? "ACK " : "",
		(hdr->Flags & TCP_FLAG_PSH) ? "PSH " : "",
		(hdr->Flags & TCP_FLAG_RST) ? "RST " : "",
		(hdr->Flags & TCP_FLAG_SYN) ? "SYN " : "",
		(hdr->Flags & TCP_FLAG_FIN) ? "FIN " : ""
		);
	#endif

	if( Length > (hdr->DataOffset >> 4)*4 )
	{
		LOG("SequenceNumber = 0x%x", ntohl(hdr->SequenceNumber));
#if HEXDUMP_INCOMING
		Debug_HexDump(
			"TCP_GetPacket: Packet Data = ",
			(Uint8*)hdr + (hdr->DataOffset >> 4)*4,
			Length - (hdr->DataOffset >> 4)*4
			);
#endif
	}

	// Check Servers
	for( tTCPListener *srv = gTCP_Listeners; srv; srv = srv->Next )
	{
		// Check if the server is active
		if(srv->Port == 0)	continue;
		// Check the interface
		if(srv->Interface && srv->Interface != Interface)	continue;
		// Check the destination port
		if(srv->Port != htons(hdr->DestPort))	continue;
		
		Log_Log("TCP", "TCP_GetPacket: Matches server %p", srv);
		// Is this in an established connection?
		for( tTCPConnection *conn = srv->Connections; conn; conn = conn->Next )
		{
			// Check that it is coming in on the same interface
			if(conn->Interface != Interface)	continue;
			// Check Source Port
			if(conn->RemotePort != ntohs(hdr->SourcePort))	continue;
			// Check Source IP
			if( IPStack_CompareAddress(conn->Interface->Type, &conn->RemoteIP, Address, -1) == 0 )
				continue ;

			Log_Log("TCP", "TCP_GetPacket: Matches connection %p", conn);
			// We have a response!
			if( TCP_INT_HandleConnectionPacket(conn, hdr, Length) == 0 )
				return;
			break ;
		}

		TCP_INT_HandleServerPacket(Interface, srv, Address, hdr, Length);
		return ;
	}

	// Check Open Connections
	{
		for( tTCPConnection *conn = gTCP_OutbountCons; conn; conn = conn->Next )
		{
			// Check that it is coming in on the same interface
			if(conn->Interface != Interface)	continue;

			// Check Source Port
			if(conn->RemotePort != ntohs(hdr->SourcePort))	continue;

			// Check Source IP
			if( IPStack_CompareAddress(conn->Interface->Type, &conn->RemoteIP, Address, -1) == 0 )
				continue ;

			// Handle or fall through
			if( TCP_INT_HandleConnectionPacket(conn, hdr, Length) == 0 )
				return ;
			break;
		}
	}
	
	Log_Log("TCP", "TCP_GetPacket: No Match");
	// If not a RST, send a RST
	if( !(hdr->Flags & TCP_FLAG_RST) )
	{
		TCP_int_SendRSTTo(Interface, Address, Length, hdr);
	}
}

/*
 * Handle packets in LISTEN state
 */
int TCP_INT_HandleServerPacket(tInterface *Interface, tTCPListener *Server, const void *Address, tTCPHeader *Header, size_t Length)
{
	if( Header->Flags & TCP_FLAG_RST ) {
		LOG("RST, ignore");
		return 0;
	}
	else if( Header->Flags & TCP_FLAG_ACK ) {
		LOG("ACK, send RST");
		TCP_int_SendRSTTo(Interface, Address, Length, Header);
		return 0;
	}
	else if( !(Header->Flags & TCP_FLAG_SYN) ) {
		LOG("Other, ignore");
		return 0;
	}
	
	Log_Log("TCP", "TCP_GetPacket: Opening Connection");
	
	// TODO: Check security (a TCP Option)
	// TODO: Check SEG.PRC 
	// TODO: Check for halfopen max
	
	tTCPConnection *conn = TCP_int_CreateConnection(Interface, TCP_ST_SYN_RCVD);
	conn->LocalPort = Server->Port;
	conn->RemotePort = ntohs(Header->SourcePort);
	
	switch(Interface->Type)
	{
	case 4:	conn->RemoteIP.v4 = *(tIPv4*)Address;	break;
	case 6:	conn->RemoteIP.v6 = *(tIPv6*)Address;	break;
	default:	ASSERTC(Interface->Type,==,4);	return 0;
	}
	
	conn->NextSequenceRcv = ntohl( Header->SequenceNumber ) + 1;
	conn->HighestSequenceRcvd = conn->NextSequenceRcv;
	conn->NextSequenceSend = rand();
	conn->LastACKSequence = ntohl( Header->SequenceNumber );
	
	conn->Node.ImplInt = Server->NextID ++;
	conn->Node.Size = -1;
	
	// Hmm... Theoretically, this lock will never have to wait,
	// as the interface is locked to the watching thread, and this
	// runs in the watching thread. But, it's a good idea to have
	// it, just in case
	// Oh, wait, there is a case where a wildcard can be used
	// (Server->Interface == NULL) so having the lock is a good idea
	SHORTLOCK(&Server->lConnections);
	conn->Server = Server;
	conn->Prev = Server->ConnectionsTail;
	if(Server->Connections) {
		ASSERT(Server->ConnectionsTail);
		Server->ConnectionsTail->Next = conn;
	}
	else {
		ASSERT(!Server->ConnectionsTail);
		Server->Connections = conn;
	}
	Server->ConnectionsTail = conn;
	if(!Server->NewConnections)
		Server->NewConnections = conn;
	VFS_MarkAvaliable( &Server->Node, 1 );
	SHORTREL(&Server->lConnections);
	Semaphore_Signal(&Server->WaitingConnections, 1);

	// Send the SYN ACK
	Header->Flags = TCP_FLAG_ACK|TCP_FLAG_SYN;
	Header->AcknowlegementNumber = htonl(conn->NextSequenceRcv);
	Header->SequenceNumber = htonl(conn->NextSequenceSend);
	Header->DestPort = Header->SourcePort;
	Header->SourcePort = htons(Server->Port);
	Header->DataOffset = (sizeof(tTCPHeader)/4) << 4;
	TCP_SendPacket( conn, Header, 0, NULL );
	conn->NextSequenceSend ++;
	return 0;
}

/**
 * \brief Handles a packet sent to a specific connection
 * \param Connection	TCP Connection pointer
 * \param Header	TCP Packet pointer
 * \param Length	Length of the packet
 */
int TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header, int Length)
{
	 int	dataLen;
	Uint32	sequence_num;
	
	// Silently drop once finished
	// TODO: Check if this needs to be here
	if( Connection->State == TCP_ST_FINISHED ) {
		Log_Log("TCP", "Packet ignored - connection finnished");
		return 1;
	}
	if( Connection->State == TCP_ST_FORCE_CLOSE ) {
		Log_Log("TCP", "Packet ignored - connection reset");
		return 1;
	}
	
	// Syncronise sequence values
	if(Header->Flags & TCP_FLAG_SYN) {
		// TODO: What if the packet also has data?
		if( Connection->LastACKSequence != Connection->NextSequenceRcv )
			TCP_INT_SendACK(Connection, "SYN");
		Connection->NextSequenceRcv = ntohl(Header->SequenceNumber);
		// TODO: Process HighestSequenceRcvd
		// HACK!
		if( Connection->HighestSequenceRcvd == 0 )
			Connection->HighestSequenceRcvd = Connection->NextSequenceRcv;
		Connection->LastACKSequence = Connection->NextSequenceRcv;
	}
	
	// Ackowledge a sent packet
	if(Header->Flags & TCP_FLAG_ACK) {
		// TODO: Process an ACKed Packet
		LOG("Conn %p, Sent packet 0x%x ACKed", Connection, Header->AcknowlegementNumber);
	}
	
	// Get length of data
	dataLen = Length - (Header->DataOffset>>4)*4;
	LOG("dataLen = %i", dataLen);
	#if TCP_DEBUG
	Log_Debug("TCP", "State %i, dataLen = %x", Connection->State, dataLen);
	#endif
	
	// 
	// State Machine
	//
	switch( Connection->State )
	{
	// Pre-init connection?
	case TCP_ST_CLOSED:
		Log_Log("TCP", "Packets to a closed connection?!");
		break;
	
	// --- Init States ---
	// SYN sent, expecting SYN-ACK Connection Opening
	case TCP_ST_SYN_SENT:
		if( Header->Flags & TCP_FLAG_SYN )
		{
			if( Connection->HighestSequenceRcvd == Connection->NextSequenceRcv )
				Connection->HighestSequenceRcvd ++;
			Connection->NextSequenceRcv ++;
			
			if( Header->Flags & TCP_FLAG_ACK )
			{	
				Log_Log("TCP", "ACKing SYN-ACK");
				Connection->State = TCP_ST_ESTABLISHED;
				VFS_MarkFull(&Connection->Node, 0);
				TCP_INT_SendACK(Connection, "SYN-ACK");
			}
			else
			{
				Log_Log("TCP", "ACKing SYN");
				Connection->State = TCP_ST_SYN_RCVD;
				TCP_INT_SendACK(Connection, "SYN");
			}
		}
		break;
	
	// SYN-ACK sent, expecting ACK
	case TCP_ST_SYN_RCVD:
		if( Header->Flags & TCP_FLAG_RST )
		{
			Log_Log("TCP", "RST Received, closing");
			Connection->State = TCP_ST_FORCE_CLOSE;
			VFS_MarkError(&Connection->Node, 1);
			return 0;
		}
		if( Header->Flags & TCP_FLAG_ACK )
		{
			// TODO: Handle max half-open limit
			Log_Log("TCP", "Connection fully opened");
			Connection->State = TCP_ST_ESTABLISHED;
			VFS_MarkFull(&Connection->Node, 0);
		}
		break;
		
	// --- Established State ---
	case TCP_ST_ESTABLISHED:
		// - Handle State changes
		//
		if( Header->Flags & TCP_FLAG_RST )
		{
			Log_Log("TCP", "Conn %p closed, received RST");
			// Error outstanding transactions
			Connection->State = TCP_ST_FORCE_CLOSE;
			VFS_MarkError(&Connection->Node, 1);
			return 0;
		}
		if( Header->Flags & TCP_FLAG_FIN ) {
			Log_Log("TCP", "Conn %p closed, recieved FIN", Connection);
			VFS_MarkError(&Connection->Node, 1);
			Connection->NextSequenceRcv ++;
			TCP_INT_SendACK(Connection, "FIN Received");
			Connection->State = TCP_ST_CLOSE_WAIT;
			// CLOSE WAIT requires the client to close
			return 0;
		}
	
		// Check for an empty packet
		if(dataLen == 0) {
			if( Header->Flags == TCP_FLAG_ACK )
			{
				Log_Log("TCP", "ACK only packet");
				return 0;
			}
			// TODO: Is this right? (empty packet counts as one byte)
			if( Connection->HighestSequenceRcvd == Connection->NextSequenceRcv )
				Connection->HighestSequenceRcvd ++;
			Connection->NextSequenceRcv ++;
			Log_Log("TCP", "Empty Packet, inc and ACK the current sequence number");
			TCP_INT_SendACK(Connection, "Empty");
			return 0;
		}
		
		// NOTES:
		// Flags
		//    PSH - Has Data?
		// /NOTES
		
		sequence_num = ntohl(Header->SequenceNumber);
		
		LOG("0x%08x <= 0x%08x < 0x%08x",
			Connection->NextSequenceRcv,
			ntohl(Header->SequenceNumber),
			Connection->NextSequenceRcv + TCP_WINDOW_SIZE
			);
		
		// Is this packet the next expected packet?
		if( sequence_num == Connection->NextSequenceRcv )
		{
			 int	rv;
			// Ooh, Goodie! Add it to the recieved list
			rv = TCP_INT_AppendRecieved(Connection,
				(Uint8*)Header + (Header->DataOffset>>4)*4,
				dataLen
				);
			if(rv != 0) {
				Log_Notice("TCP", "TCP_INT_AppendRecieved rv %i", rv);
				break;
			}
			LOG("0x%08x += %i", Connection->NextSequenceRcv, dataLen);
			if( Connection->HighestSequenceRcvd == Connection->NextSequenceRcv )
				Connection->HighestSequenceRcvd += dataLen;
			Connection->NextSequenceRcv += dataLen;
			
			// TODO: This should be moved out of the watcher thread,
			// so that a single lost packet on one connection doesn't cause
			// all connections on the interface to lag.
			// - Meh, no real issue, as the cache shouldn't be that large
			TCP_INT_UpdateRecievedFromFuture(Connection);

			#if 1
			// - Only send an ACK if we've had a burst
			Uint32	bytes_since_last_ack = Connection->NextSequenceRcv - Connection->LastACKSequence;
			LOG("bytes_since_last_ack = 0x%x", bytes_since_last_ack);
			if( bytes_since_last_ack > TCP_DACK_THRESHOLD )
			{
				TCP_INT_SendACK(Connection, "DACK Burst");
				// - Extend TCP deferred ACK timer
				Time_RemoveTimer(Connection->DeferredACKTimer);
			}
			// - Schedule the deferred ACK timer (if already scheduled, this is a NOP)
			Time_ScheduleTimer(Connection->DeferredACKTimer, TCP_DACK_TIMEOUT);
			#else
			TCP_INT_SendACK(Connection, "RX");
			#endif
		}
		// Check if the packet is in window
		else if( sequence_num - Connection->NextSequenceRcv < TCP_WINDOW_SIZE )
		{
			Uint8	*dataptr = (Uint8*)Header + (Header->DataOffset>>4)*4;
			Uint32	index = sequence_num % TCP_WINDOW_SIZE;
			Uint32	max = Connection->NextSequenceRcv % TCP_WINDOW_SIZE;
			if( !(Connection->FuturePacketValidBytes[index/8] & (1 << (index%8))) )
				TCP_INT_SendACK(Connection, "Lost packet");
			for( int i = 0; i < dataLen; i ++ )
			{
				Connection->FuturePacketValidBytes[index/8] |= 1 << (index%8);
				Connection->FuturePacketData[index] = dataptr[i];
				// Do a wrap increment
				index ++;
				if(index == TCP_WINDOW_SIZE)	index = 0;
				if(index == max)	break;
			}
			Uint32	rel_highest = Connection->HighestSequenceRcvd - Connection->NextSequenceRcv;
			Uint32	rel_this = index - Connection->NextSequenceRcv;
			LOG("Updating highest this(0x%x) > highest(%x)", rel_this, rel_highest);
			if( rel_this > rel_highest )
			{
				Connection->HighestSequenceRcvd = index;
			}
		}
		// Badly out of sequence packet
		else
		{
			Log_Log("TCP", "Fully out of sequence packet (0x%08x not between 0x%08x and 0x%08x), dropped",
				sequence_num, Connection->NextSequenceRcv, Connection->NextSequenceRcv+TCP_WINDOW_SIZE);
			// Spec says we should send an empty ACK with the current state
			TCP_INT_SendACK(Connection, "Bad Seq");
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
			TCP_int_FreeTCB(Connection);
		}
		break;
	
	// --- Local close States
	case TCP_ST_FIN_WAIT1:
		if( Header->Flags & TCP_FLAG_FIN )
		{
			Connection->State = TCP_ST_CLOSING;
			Log_Debug("TCP", "Conn %p closed, sent FIN and recieved FIN", Connection);
			VFS_MarkError(&Connection->Node, 1);
			
			TCP_INT_SendACK(Connection, "FINWAIT-1 FIN");
			break ;
		}
		
		// TODO: Make sure that the packet is actually ACKing the FIN
		if( Header->Flags & TCP_FLAG_ACK )
		{
			Connection->State = TCP_ST_FIN_WAIT2;
			Log_Debug("TCP", "Conn %p closed, sent FIN ACKed", Connection);
			VFS_MarkError(&Connection->Node, 1);
			return 0;
		}
		break;
	
	case TCP_ST_FIN_WAIT2:
		if( Header->Flags & TCP_FLAG_FIN )
		{
			Connection->State = TCP_ST_TIME_WAIT;
			Log_Debug("TCP", "Conn %p FINWAIT-2 -> TIME WAIT", Connection);
			TCP_INT_SendACK(Connection, "FINWAIT-2 FIN");
		}
		break;
	
	case TCP_ST_CLOSING:
		// TODO: Make sure that the packet is actually ACKing the FIN
		if( Header->Flags & TCP_FLAG_ACK )
		{
			Connection->State = TCP_ST_TIME_WAIT;
			Log_Debug("TCP", "Conn %p CLOSING -> TIME WAIT", Connection);
			VFS_MarkError(&Connection->Node, 1);
			return 0;
		}
		break;
	
	// --- Closed (or near closed) states) ---
	case TCP_ST_TIME_WAIT:
		Log_Log("TCP", "Packets on Time-Wait, ignored");
		break;
	
	case TCP_ST_FINISHED:
		Log_Log("TCP", "Packets when CLOSED, ignoring");
		break;
	case TCP_ST_FORCE_CLOSE:
		Log_Log("TCP", "Packets when force CLOSED, ignoring");
		return 1;
	
	//default:
	//	Log_Warning("TCP", "Unhandled TCP state %i", Connection->State);
	//	break;
	}
	
	return 0;
	
}

/**
 * \brief Appends a packet to the recieved list
 * \param Connection	Connection structure
 * \param Data	Packet contents
 * \param Length	Length of \a Data
 */
int TCP_INT_AppendRecieved(tTCPConnection *Connection, const void *Data, size_t Length)
{
	Mutex_Acquire( &Connection->lRecievedPackets );

	if(Connection->RecievedBuffer->Length + Length > Connection->RecievedBuffer->Space )
	{
		VFS_MarkAvaliable(&Connection->Node, 1);
		Log_Error("TCP", "Buffer filled, packet dropped (:%i) - %i + %i > %i",
			Connection->LocalPort, Connection->RecievedBuffer->Length, Length,
			Connection->RecievedBuffer->Space
			);
		Mutex_Release( &Connection->lRecievedPackets );
		return 1;
	}
	
	RingBuffer_Write( Connection->RecievedBuffer, Data, Length );

	VFS_MarkAvaliable(&Connection->Node, 1);
	
	Mutex_Release( &Connection->lRecievedPackets );
	return 0;
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
	// Calculate length of contiguous bytes
	const size_t	length = Connection->HighestSequenceRcvd - Connection->NextSequenceRcv;
	Uint32	index = Connection->NextSequenceRcv % TCP_WINDOW_SIZE;
	size_t	runlength = length;
	LOG("HSR=0x%x,NSR=0x%x", Connection->HighestSequenceRcvd, Connection->NextSequenceRcv);
	if( Connection->HighestSequenceRcvd == Connection->NextSequenceRcv )
	{
		return ;
	}
	LOG("length=%u, index=0x%x", length, index);
	for( int i = 0; i < length; i ++ )
	{
		 int	bit = index % 8;
		Uint8	bitfield_byte = Connection->FuturePacketValidBytes[index / 8];
		if( (bitfield_byte & (1 << bit)) == 0 ) {
			runlength = i;
			LOG("Hit missing, break");
			break;
		}

		if( bitfield_byte == 0xFF ) {
			 int	inc = 8 - bit;
			i += inc - 1;
			index += inc;
		}
		else {
			index ++;
		}
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
	}
	
	index = Connection->NextSequenceRcv % TCP_WINDOW_SIZE;
	Connection->NextSequenceRcv += runlength;
	
	// Write data to to the ring buffer
	if( TCP_WINDOW_SIZE - index > runlength )
	{
		// Simple case
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData + index, runlength );
	}
	else
	{
		 int	endLen = TCP_WINDOW_SIZE - index;
		// 2-part case
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData + index, endLen );
		RingBuffer_Write( Connection->RecievedBuffer, Connection->FuturePacketData, endLen - runlength );
	}
	
	// Mark (now saved) bytes as invalid
	// - Align index
	while(index % 8 && runlength > 0)
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketValidBytes[index/8] &= ~(1 << (index%8));
		index ++;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
		runlength --;
	}
	while( runlength > 7 )
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketValidBytes[index/8] = 0;
		runlength -= 8;
		index += 8;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
	}
	while( runlength > 0)
	{
		Connection->FuturePacketData[index] = 0;
		Connection->FuturePacketData[index/8] &= ~(1 << (index%8));
		index ++;
		if(index > TCP_WINDOW_SIZE)
			index -= TCP_WINDOW_SIZE;
		runlength --;
	}
}

void TCP_int_SendDelayedACK(void *ConnPtr)
{
	TCP_INT_SendACK(ConnPtr, "DACK Timeout");
}

void TCP_INT_SendACK(tTCPConnection *Connection, const char *Reason)
{
	tTCPHeader	hdr;
	// ACK Packet
	hdr.DataOffset = (sizeof(tTCPHeader)/4) << 4;
	hdr.DestPort = htons(Connection->RemotePort);
	hdr.SourcePort = htons(Connection->LocalPort);
	hdr.AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
	hdr.SequenceNumber = htonl(Connection->NextSequenceSend);
	hdr.WindowSize = htons(TCP_WINDOW_SIZE);
	hdr.Flags = TCP_FLAG_ACK;	// TODO: Determine if SYN is wanted too
	hdr.Checksum = 0;	// TODO: Checksum
	hdr.UrgentPointer = 0;
	Log_Debug("TCP", "Sending ACK for 0x%08x (%s)", Connection->NextSequenceRcv, Reason);
	TCP_SendPacket( Connection, &hdr, 0, NULL );
	//Connection->NextSequenceSend ++;
	Connection->LastACKSequence = Connection->NextSequenceRcv;
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
	while( gaTCP_PortBitmap[ret/32] & (1UL << (ret%32)) )
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

tTCPConnection *TCP_int_CreateConnection(tInterface *Interface, enum eTCPConnectionState State)
{
	tTCPConnection	*conn = calloc( sizeof(tTCPConnection) + TCP_WINDOW_SIZE + TCP_WINDOW_SIZE/8, 1 );

	conn->State = State;
	conn->Interface = Interface;
	conn->LocalPort = -1;
	conn->RemotePort = -1;

	conn->Node.Size = -1;
	conn->Node.ReferenceCount = 1;
	conn->Node.ImplPtr = conn;
	conn->Node.NumACLs = 1;
	conn->Node.ACLs = &gVFS_ACL_EveryoneRW;
	conn->Node.Type = &gTCP_ClientNodeType;
	conn->Node.BufferFull = 1;	// Cleared when connection opens

	conn->RecievedBuffer = RingBuffer_Create( TCP_RECIEVE_BUFFER_SIZE );
	#if 0
	conn->SentBuffer = RingBuffer_Create( TCP_SEND_BUFFER_SIZE );
	Semaphore_Init(conn->SentBufferSpace, 0, TCP_SEND_BUFFER_SIZE, "TCP SentBuffer", conn->Name);
	#endif
	
	conn->HighestSequenceRcvd = 0;
	#if CACHE_FUTURE_PACKETS_IN_BYTES
	// Future recieved data (ahead of the expected sequence number)
	conn->FuturePacketData = (Uint8*)conn + sizeof(tTCPConnection);
	conn->FuturePacketValidBytes = conn->FuturePacketData + TCP_WINDOW_SIZE;
	#endif

	conn->DeferredACKTimer = Time_AllocateTimer( TCP_int_SendDelayedACK, conn);
	return conn;
}

void TCP_int_FreeTCB(tTCPConnection *Connection)
{
	ASSERTC(Connection->State, ==, TCP_ST_FINISHED);
	ASSERTC(Connection->Node.ReferenceCount, ==, 0);

	if( Connection->Server )
	{
		tTCPListener	*srv = Connection->Server;
		SHORTLOCK(&srv->lConnections);
		if(Connection->Prev)
			Connection->Prev->Next = Connection->Next;
		else
			srv->Connections = Connection->Next;
		if(Connection->Next)
			Connection->Next->Prev = Connection->Prev;
		else {
			ASSERT(srv->ConnectionsTail == Connection);
			srv->ConnectionsTail = Connection->Prev;
		}
		SHORTREL(&srv->lConnections);
	}
	else
	{
		SHORTLOCK(&glTCP_OutbountCons);
		if(Connection->Prev)
			Connection->Prev->Next = Connection->Next;
		else
			gTCP_OutbountCons = Connection->Next;
		if(Connection->Next)
			Connection->Next->Prev = Connection->Prev;
		else
			;
		SHORTREL(&glTCP_OutbountCons);
	}

	RingBuffer_Free(Connection->RecievedBuffer);
	Time_FreeTimer(Connection->DeferredACKTimer);
	// TODO: Force VFS to close handles? (they should all be closed);
	free(Connection);
}

// --- Server
tVFS_Node *TCP_Server_Init(tInterface *Interface)
{
	tTCPListener	*srv;
	
	srv = calloc( 1, sizeof(tTCPListener) );

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
	srv->Node.Type = &gTCP_ServerNodeType;

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
int TCP_Server_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	tTCPListener	*srv = Node->ImplPtr;
	tTCPConnection	*conn;
	
	ENTER("pNode iPos", Node, Pos);

	Log_Log("TCP", "Thread %i waiting for a connection", Threads_GetTID());
	Semaphore_Wait( &srv->WaitingConnections, 1 );
	
	SHORTLOCK(&srv->lConnections);
	// Increment the new list (the current connection is still on the 
	// normal list)
	conn = srv->NewConnections;
	srv->NewConnections = conn->Next;

	if( srv->NewConnections == NULL )
		VFS_MarkAvaliable( Node, 0 );
	
	SHORTREL( &srv->lConnections );
	
	LOG("conn = %p", conn);
	LOG("srv->Connections = %p", srv->Connections);
	LOG("srv->NewConnections = %p", srv->NewConnections);
	LOG("srv->ConnectionsTail = %p", srv->ConnectionsTail);

	itoa(Dest, conn->Node.ImplInt, 16, 8, '0');
	Log_Log("TCP", "Thread %i got connection '%s'", Threads_GetTID(), Dest);
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Gets a client connection node
 * \param Node	Server node
 * \param Name	Hexadecimal ID of the node
 */
tVFS_Node *TCP_Server_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tTCPConnection	*conn;
	tTCPListener	*srv = Node->ImplPtr;
	char	tmp[9];
	 int	id = atoi(Name);
	
	ENTER("pNode sName", Node, Name);

	// Check for a non-empty name
	if( Name[0] ) 
	{	
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
	}
	// Empty Name - Check for a new connection and if it's there, open it
	else
	{
		SHORTLOCK( &srv->lConnections );
		conn = srv->NewConnections;
		if( conn != NULL )
			srv->NewConnections = conn->Next;
		VFS_MarkAvaliable( Node, srv->NewConnections != NULL );
		SHORTREL( &srv->lConnections );
		if( !conn ) {
			LOG("No new connections");
			LEAVE('n');
			return NULL;
		}
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
	tTCPConnection	*conn = TCP_int_CreateConnection(Interface, TCP_ST_CLOSED);

	SHORTLOCK(&glTCP_OutbountCons);
	conn->Server = NULL;
	conn->Prev = NULL;
	conn->Next = gTCP_OutbountCons;
	if(gTCP_OutbountCons)
		gTCP_OutbountCons->Prev = conn;
	gTCP_OutbountCons = conn;
	SHORTREL(&glTCP_OutbountCons);

	return &conn->Node;
}

/**
 * \brief Wait for a packet and return it
 * \note If \a Length is smaller than the size of the packet, the rest
 *       of the packet's data will be discarded.
 */
size_t TCP_Client_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tTCPConnection	*conn = Node->ImplPtr;
	size_t	len;
	
	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	LOG("conn = %p {State:%i}", conn, conn->State);
	
	// If the connection has been closed (state > ST_OPEN) then clear
	// any stale data in the buffer (until it is empty (until it is empty))
	if( conn->State > TCP_ST_ESTABLISHED )
	{
		LOG("Connection closed");
		Mutex_Acquire( &conn->lRecievedPackets );
		len = RingBuffer_Read( Buffer, conn->RecievedBuffer, Length );
		Mutex_Release( &conn->lRecievedPackets );
		
		if( len == 0 ) {
			VFS_MarkAvaliable(Node, 0);
			errno = 0;
			LEAVE('i', -1);
			return -1;
		}
		
		LEAVE('i', len);
		return len;
	}
	
	// Wait
	{
		tTime	*timeout = NULL;
		tTime	timeout_zero = 0;
		if( Flags & VFS_IOFLAG_NOBLOCK )
			timeout = &timeout_zero;
		if( !VFS_SelectNode(Node, VFS_SELECT_READ|VFS_SELECT_ERROR, timeout, "TCP_Client_Read") ) {
			errno = EWOULDBLOCK;
			LEAVE('i', -1);
			return -1;
		}
	}
	
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
void TCP_INT_SendDataPacket(tTCPConnection *Connection, size_t Length, const void *Data)
{
	char	buf[sizeof(tTCPHeader)+Length];
	tTCPHeader	*packet = (void*)buf;

	// - Stop Delayed ACK timer (as this data packet ACKs)
	Time_RemoveTimer(Connection->DeferredACKTimer);

	// TODO: Don't exceed window size
	
	packet->SourcePort = htons(Connection->LocalPort);
	packet->DestPort = htons(Connection->RemotePort);
	packet->DataOffset = (sizeof(tTCPHeader)/4)*16;
	packet->WindowSize = htons(TCP_WINDOW_SIZE);
	
	packet->AcknowlegementNumber = htonl(Connection->NextSequenceRcv);
	packet->SequenceNumber = htonl(Connection->NextSequenceSend);
	packet->Flags = TCP_FLAG_PSH|TCP_FLAG_ACK;	// Hey, ACK if you can!
	packet->UrgentPointer = 0;
	
	memcpy(packet->Options, Data, Length);
	
	Log_Debug("TCP", "Send sequence 0x%08x", Connection->NextSequenceSend);
#if HEXDUMP_OUTGOING
	Debug_HexDump("TCP_INT_SendDataPacket: Data = ", Data, Length);
#endif
	
	TCP_SendPacket( Connection, packet, Length, Data );
	
	Connection->NextSequenceSend += Length;
}

/**
 * \brief Send some bytes on a connection
 */
size_t TCP_Client_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tTCPConnection	*conn = Node->ImplPtr;
	size_t	rem = Length;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
//	#if DEBUG
//	Debug_HexDump("TCP_Client_Write: Buffer = ",
//		Buffer, Length);
//	#endif
	
	// Don't allow a write to a closed connection
	if( conn->State > TCP_ST_ESTABLISHED ) {
		VFS_MarkError(Node, 1);
		errno = 0;
		LEAVE('i', -1);
		return -1;
	}
	
	// Wait
	{
		tTime	*timeout = NULL;
		tTime	timeout_zero = 0;
		if( Flags & VFS_IOFLAG_NOBLOCK )
			timeout = &timeout_zero;
		if( !VFS_SelectNode(Node, VFS_SELECT_WRITE|VFS_SELECT_ERROR, timeout, "TCP_Client_Write") ) {
			errno = EWOULDBLOCK;
			LEAVE('i', -1);
			return -1;
		}
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
	
	TCP_SendPacket( Conn, &hdr, 0, NULL );
	
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
			tTime	timeout = conn->Interface->TimeoutDelay;
	
			TCP_StartConnection(conn);
			VFS_SelectNode(&conn->Node, VFS_SELECT_WRITE, &timeout, "TCP Connection");
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
	
	ASSERT(Node->ReferenceCount != 0);

	if( Node->ReferenceCount > 1 ) {
		Node->ReferenceCount --;
		LOG("Dereference only");
		LEAVE('-');
		return ;
	}
	Node->ReferenceCount --;
	
	if( conn->State == TCP_ST_CLOSE_WAIT || conn->State == TCP_ST_ESTABLISHED )
	{
		packet.SourcePort = htons(conn->LocalPort);
		packet.DestPort = htons(conn->RemotePort);
		packet.DataOffset = (sizeof(tTCPHeader)/4)*16;
		packet.WindowSize = TCP_WINDOW_SIZE;
		
		packet.AcknowlegementNumber = 0;
		packet.SequenceNumber = htonl(conn->NextSequenceSend);
		packet.Flags = TCP_FLAG_FIN;
		
		TCP_SendPacket( conn, &packet, 0, NULL );
	}
	
	Time_RemoveTimer(conn->DeferredACKTimer);
	
	switch( conn->State )
	{
	case TCP_ST_CLOSED:
		Log_Warning("TCP", "Closing connection that was never opened");
		TCP_int_FreeTCB(conn);
		break;
	case TCP_ST_FORCE_CLOSE:
		conn->State = TCP_ST_FINISHED;
		TCP_int_FreeTCB(conn);
		break;
	case TCP_ST_CLOSE_WAIT:
		conn->State = TCP_ST_LAST_ACK;
		break;
	case TCP_ST_ESTABLISHED:
		conn->State = TCP_ST_FIN_WAIT1;
		while( conn->State == TCP_ST_FIN_WAIT1 )
			Threads_Yield();
		// No free, freed after TIME_WAIT
		break;
	default:
		Log_Warning("TCP", "Unhandled connection state %i in TCP_Client_Close",
			conn->State);
		break;
	}
	
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
Uint32 GetRelative(Uint32 Base, Uint32 Value)
{
	if( Value < Base )
		return Value - Base + 0xFFFFFFFF;
	else
		return Value - Base;
}

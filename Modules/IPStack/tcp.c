/*
 * Acess2 IP Stack
 * - TCP Handling
 */
#include "ipstack.h"
#include "ipv4.h"
#include "tcp.h"

#define TCP_MIN_DYNPORT	0x1000
#define TCP_MAX_HALFOPEN	1024	// Should be enough

// === PROTOTYPES ===
void	TCP_Initialise();
void	TCP_StartConnection(tTCPConnection *Conn);
void	TCP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);
void	TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header);
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
	// SEND PACKET
	// Send a TCP SYN to the target to open the connection
	return ;
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
	
	Log("[TCP  ] sizeof(tTCPHeader) = %i", sizeof(tTCPHeader));
	Log("[TCP  ] DestPort = %i", ntohs(hdr->DestPort));
	Log("[TCP  ] DestPort = %i", ntohs(hdr->DestPort));
	Log("[TCP  ] SequenceNumber = %i", ntohl(hdr->SequenceNumber));
	Log("[TCP  ] AcknowlegementNumber = %i", ntohl(hdr->AcknowlegementNumber));
	Log("[TCP  ] DataOffset = %i", hdr->DataOffset >> 4);
	Log("[TCP  ] Flags = {");
	Log("[TCP  ]   CWR = %B", !!(hdr->Flags & TCP_FLAG_CWR));
	Log("[TCP  ]   ECE = %B", !!(hdr->Flags & TCP_FLAG_ECE));
	Log("[TCP  ]   URG = %B", !!(hdr->Flags & TCP_FLAG_URG));
	Log("[TCP  ]   ACK = %B", !!(hdr->Flags & TCP_FLAG_ACK));
	Log("[TCP  ]   PSH = %B", !!(hdr->Flags & TCP_FLAG_PSH));
	Log("[TCP  ]   RST = %B", !!(hdr->Flags & TCP_FLAG_RST));
	Log("[TCP  ]   SYN = %B", !!(hdr->Flags & TCP_FLAG_SYN));
	Log("[TCP  ]   FIN = %B", !!(hdr->Flags & TCP_FLAG_FIN));
	Log("[TCP  ] }");
	Log("[TCP  ] WindowSize = %i", htons(hdr->WindowSize));
	Log("[TCP  ] Checksum = 0x%x", htons(hdr->Checksum));
	Log("[TCP  ] UrgentPointer = 0x%x", htons(hdr->UrgentPointer));
	
	// Check Servers
	{
		for( srv = gTCP_Listeners; srv; srv = srv->Next )
		{
			// Check the interface
			if(srv->Interface && srv->Interface != Interface)	continue;
			// Check the destination port
			if(srv->Port != hdr->DestPort)	continue;
			
			// Is this in an established connection?
			for( conn = srv->Connections; conn; conn = conn->Next )
			{
				// Check that it is coming in on the same interface
				if(conn->Interface != Interface)	continue;
				
				// Check Source Port
				if(conn->RemotePort != hdr->SourcePort)	continue;
				
				// Check Source IP
				if(conn->Interface->Type == 6 && !IP6_EQU(conn->RemoteIP.v6, *(tIPv6*)Address))
					continue;
				if(conn->Interface->Type == 4 && !IP4_EQU(conn->RemoteIP.v4, *(tIPv4*)Address))
					continue;
				
				// We have a response!
				TCP_INT_HandleConnectionPacket(conn, hdr)
				
				return;
			}
			
			// Open a new connection (well, check that it's a SYN)
			//TODO
			
			break;
		}
	}
	
	
	// Check Open Connections
	{
		for( conn = gTCP_OutbountCons; conn; conn = conn->Next )
		{
			// Check that it is coming in on the same interface
			if(conn->Interface != Interface)	continue;
			
			// Check Source Port
			if(conn->RemotePort != hdr->SourcePort)	continue;
			
			// Check Source IP
			if(conn->Interface->Type == 6 && !IP6_EQU(conn->RemoteIP.v6, *(tIPv6*)Address))
				continue;
			if(conn->Interface->Type == 4 && !IP4_EQU(conn->RemoteIP.v4, *(tIPv4*)Address))
				continue;
			
			TCP_INT_HandleConnectionPacket(conn, hdr)
		}
	}
}

/**
 * \brief Handles a packet sent to a specific connection
 */
void TCP_INT_HandleConnectionPacket(tTCPConnection *Connection, tTCPHeader *Header)
{
	
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
	srv->Connections = NULL;
	srv->Next = NULL;
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

char *TCP_Server_ReadDir(tVFS_Node *Node, int Pos)
{
	return NULL;
}

tVFS_Node *TCP_Server_FindDir(tVFS_Node *Node, char *Name)
{
	return NULL;
}

int TCP_Server_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tTCPListener	*srv = Node->ImplPtr;
	
	switch(ID)
	{
	case 4:	// Get/Set Port
		if(!Data)
			return srv->Port;
		
		if(srv->Port)
			return -1;
		
		if(!CheckMem(Data, sizeof(Uint16)))
			return -1;
		
		if(Threads_GetUID() != 0 && *(Uint16*)Data < 1024)
			return -1;
		
		srv->Port = *(Uint16*)Data;
		if(srv->Port == 0)
			srv->Port = TCP_GetUnusedPort();
		else
			TCP_AllocatePort(srv->Port);
		return srv->Port;
	}
	return 0;
}

void TCP_Server_Close(tVFS_Node *Node)
{
	free(Node->ImplPtr);
}

// --- Client
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

Uint64 TCP_Client_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	return 0;
}

Uint64 TCP_Client_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	return 0;
}

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

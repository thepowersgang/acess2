/*
 * Acess2 IP Stack
 * - TCP Handling
 */
#include "ipstack.h"
#include "tcp.h"

#define TCP_MIN_DYNPORT	0x1000

// === PROTOTYPES ===
void	TCP_Initialise();
void	*TCP_Open(tInterface *Interface, Uint16 LocalPort, void *Address, Uint16 Port);
void	TCP_GetPacket(tInterface *Interface, int Length, void *Buffer);
Uint16	TCP_GetUnusedPort();
 int	TCP_AllocatePort(Uint16 Port);
 int	TCP_DeallocatePort(Uint16 Port);

// === GLOBALS ===
 int	giTCP_NumHalfopen = 0;
tTCPListener	*gTCP_Listeners;
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
	
}

/**
 * \fn void *TCP_Open(tInterface *Interface, Uint16 LocalPort, void *Address, Uint16 Port)
 * \brief Open a connection to another host using TCP
 */
void *TCP_Open(tInterface *Interface, Uint16 LocalPort, void *Address, Uint16 Port)
{
	tTCPConnection	*ret = malloc( sizeof(tTCPConnection) );
	
	ret->State = TCP_ST_CLOSED;
	if(LocalPort == 0)
		ret->LocalPort = TCP_GetUnusedPort();
	else
		ret->LocalPort = LocalPort;
	ret->RemotePort = Port;
	
	ret->LocalInterface = Interface;
	
	if(Interface->Type == 6)
		ret->RemoteIP.v6 = *(tIPv6*)Address;
	else
		ret->RemoteIP.v4 = *(tIPv4*)Address;
	
	// SEND PACKET
	
	return ret;
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
				//TODO
				
				return;
			}
			
			// Open a new connection
			//TODO
			
			break;
		}
	}
	
	
	// Check Open Connections
	{
		for( conn = gTCP_OutbountCons; conn; conn = conn->Next )
		{
			// TODO
		}
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


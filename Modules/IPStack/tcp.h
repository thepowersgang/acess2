/*
 * Acess2 IP Stack
 * - TCP Definitions
 */
#ifndef _TCP_H_
#define _TCP_H_

#include "ipstack.h"

typedef struct sTCPHeader	tTCPHeader;
typedef struct sTCPListener	tTCPListener;
typedef struct sTCPConnection	tTCPConnection;

struct sTCPHeader
{
	Uint16	SourcePort;
	Uint16	DestPort;
	Uint32	SequenceNumber;
	Uint32	AcknowlegementNumber;
	struct {
		unsigned DataOffset: 4;	// Size of the header in 32-bit words
		unsigned Reserved:	4;
	};
	struct {
		unsigned CWR:	1;	// Congestion Window Reduced
		unsigned ECE:	1;	// ECN-Echo
		unsigned URG:	1;	// Urgent pointer is significant
		unsigned ACK:	1;	// Acknowlegement field is significant
		unsigned PSH:	1;	// Push Function
		unsigned RST:	1;	// Reset Connection
		unsigned SYN:	1;	// Synchronise Sequence Numbers
		unsigned FIN:	1;	// Last packet
	} Flags;
	Uint16	WindowSize;
	
	Uint16	Checksum;
	Uint16	UrgentPointer;
	
	Uint8	Options[];
};

struct sTCPListener
{
	struct sTCPListener	*Next;
	Uint16	Port;
	tInterface	*Interface;
	tTCPConnection	*Connections;
};

struct sTCPConnection
{
	struct sTCPConnection	*Next;
	 int	State;
	Uint16	LocalPort;
	Uint16	RemotePort;
	tVFS_Node	Node;
	
	tInterface	*Interface;
	union {
		tIPv4	v4;
		tIPv6	v6;
	} RemoteIP;	// Type is determined by LocalInterface->Type
};

enum eTCPConnectionState
{
	TCP_ST_CLOSED,
	TCP_ST_HALFOPEN,
	TCP_ST_OPEN
};

#endif

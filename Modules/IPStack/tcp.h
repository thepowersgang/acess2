/*
 * Acess2 IP Stack
 * - TCP Definitions
 */
#ifndef _TCP_H_
#define _TCP_H_

#include "ipstack.h"

typedef struct sTCPHeader	tTCPHeader;
typedef struct sTCPListener	tTCPListener;
typedef struct sTCPStoredPacket	tTCPStoredPacket;
typedef struct sTCPConnection	tTCPConnection;

struct sTCPHeader
{
	Uint16	SourcePort;
	Uint16	DestPort;
	Uint32	SequenceNumber;
	Uint32	AcknowlegementNumber;
	#if 0
	struct {	// Lowest to highest
		unsigned Reserved:	4;
		unsigned DataOffset: 4;	// Size of the header in 32-bit words
	} __attribute__ ((packed));
	#else
	Uint8	DataOffset;
	#endif
	#if 0
	struct {	// Lowest to Highest
		unsigned FIN:	1;	// Last packet
		unsigned SYN:	1;	// Synchronise Sequence Numbers
		unsigned RST:	1;	// Reset Connection
		unsigned PSH:	1;	// Push Function
		unsigned ACK:	1;	// Acknowlegement field is significant
		unsigned URG:	1;	// Urgent pointer is significant
		unsigned ECE:	1;	// ECN-Echo
		unsigned CWR:	1;	// Congestion Window Reduced
	} __attribute__ ((packed)) Flags;
	#else
	Uint8	Flags;
	#endif
	Uint16	WindowSize;
	
	Uint16	Checksum;
	Uint16	UrgentPointer;
	
	Uint8	Options[];
} __attribute__ ((packed));

enum eTCPFlags
{
	TCP_FLAG_FIN	= 0x01,
	TCP_FLAG_SYN	= 0x02,
	TCP_FLAG_RST	= 0x04,
	TCP_FLAG_PSH	= 0x08,
	TCP_FLAG_ACK	= 0x10,
	TCP_FLAG_URG	= 0x20,
	TCP_FLAG_ECE	= 0x40,
	TCP_FLAG_CWR	= 0x80
};

struct sTCPListener
{
	struct sTCPListener	*Next;	//!< Next server in the list
	Uint16	Port;		//!< Listening port (0 disables the server)
	tInterface	*Interface;	//!< Listening Interface
	tVFS_Node	Node;	//!< Server Directory node
	 int	NextID;		//!< Name of the next connection
	tSpinlock	lConnections;	//!< Spinlock for connections
	tTCPConnection	*Connections;	//!< Connections (linked list)
	tTCPConnection	*volatile NewConnections;
	tTCPConnection	*ConnectionsTail;
};

struct sTCPStoredPacket
{
	struct sTCPStoredPacket	*Next;
	size_t	Length;
	Uint32	Sequence;
	Uint8	Data[];
};

struct sTCPConnection
{
	struct sTCPConnection	*Next;
	 int	State;	//!< Connection state (see ::eTCPConnectionState)
	Uint16	LocalPort;	//!< Local port
	Uint16	RemotePort;	//!< Remote port
	tInterface	*Interface;	//!< Listening Interface
	tVFS_Node	Node;	//!< Node
	
	Uint32	NextSequenceSend;	//!< Next sequence value for outbound packets
	Uint32	NextSequenceRcv;	//!< Next expected sequence value for inbound
	
	/**
	 * \brief Non-ACKed packets
	 * \note FIFO list
	 * \{
	 */
	tSpinlock	lQueuedPackets;
	tTCPStoredPacket	*QueuedPackets;	//!< Non-ACKed packets
	/**
	 * \}
	 */
	
	/**
	 * \brief Unread Packets
	 * \note Ring buffer
	 * \{
	 */
	tSpinlock	lRecievedPackets;
	tRingBuffer	*RecievedBuffer;
	/**
	 * \}
	 */
	
	/**
	 * \brief Out of sequence packets
	 * \note Sorted list to improve times
	 * \{
	 */
	tSpinlock	lFuturePackets;	//!< Future packets spinlock
	tTCPStoredPacket	*FuturePackets;	//!< Out of sequence packets
	/**
	 * \}
	 */
	
	union {
		tIPv4	v4;
		tIPv6	v6;
	} RemoteIP;	//!< Remote IP Address
	// Type is determined by LocalInterface->Type
};

enum eTCPConnectionState
{
	TCP_ST_CLOSED,
	TCP_ST_SYN_SENT,
	TCP_ST_HALFOPEN,
	TCP_ST_OPEN,
	TCP_ST_FIN_SENT,
	TCP_ST_FINISHED
};

#endif

/*
 * Acess2 RTL8139 Driver
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	0
#define VERSION	((0<<8)|20)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_network.h>
#include <semaphore.h>

// === CONSTANTS ===
#define VENDOR_ID	0x10EC
#define DEVICE_ID	0x8139

enum eRTL8139_Regs
{
	// MAC Address
	MAC0, MAC1, MAC2,
	MAC3, MAC4, MAC5,
	
	// Multicast Registers
	MAR0 = 0x08, MAR1, MAR2, MAR3,
	MAR4, MAR5, MAR6, MAR7,
	
	// Transmit status of descriptors 0 - 3
	TSD0 = 0x10,	TSD1 = 0x14,
	TSD2 = 0x18,	TSD3 = 0x1C,
	// Transmit start addresses
	TSAD0 = 0x20,	TSAD1 = 0x24,
	TSAD2 = 0x28,	TSAD3 = 0x2C,
	
	RBSTART = 0x30,	//!< Recieve Buffer Start (DWord)
	// Early Recieve Byte Count
	ERBCR = 0x34,	// 16-bits
	// Early RX Status Register
	ERSR = 0x36,
	
	// ??, ??, ??, RST, RE, TE, ??, ??
	CMD 	= 0x37,
	
	CAPR	= 0x38,	// Current address of packet read
	CBA 	= 0x3A,	// Current Buffer Address - Total byte count in RX buffer
	
	IMR 	= 0x3C,	// Interrupt mask register
	ISR 	= 0x3E,	// Interrupt status register
	
	TCR 	= 0x40,	// Transmit Configuration Register
	RCR 	= 0x44,	// Recieve Configuration Register
	TCTR	= 0x48,	// 32-bit timer (count)
	MPC 	= 0x4C,	// Missed packet count (due to RX overflow)
	
	CR_9346 = 0x50,
	CONFIG0 = 0x51,
	CONFIG1	= 0x52,
	// 0x53 resvd
	TIMERINT = 0x54,	// Fires a timeout when TCTR equals this value
	
};

#define FLAG_ISR_TOK	0x04
#define FLAG_ISR_ROK	0x01

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;
	
	 int	NumWaitingPackets;
	
	char	*ReceiveBuffer;
	tPAddr	PhysReceiveBuffer;
	 int	ReceiveBufferLength;
	 int	SeenOfs;	//!< End of the most recently seen packet (by IRQ)
	tMutex	ReadMutex;
	tSemaphore	ReadSemaphore;
	
	char	*TransmitBuffers[4];
	tPAddr	PhysTransmitBuffers[4];
	tMutex	TransmitInUse[4];
	tMutex	CurTXProtector;	//!< Protects \a .CurTXDescriptor
	 int	CurTXDescriptor;
	
	char	Name[2];
	tVFS_Node	Node;
	Uint8	MacAddr[6];
}	tCard;

// === PROTOTYPES ===
 int	RTL8139_Install(char **Options);
char	*RTL8139_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*RTL8139_FindDir(tVFS_Node *Node, const char *Filename);
 int	RTL8139_RootIOCtl(tVFS_Node *Node, int ID, void *Arg);
Uint64	RTL8139_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	RTL8139_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, const void *Buffer);
 int	RTL8139_IOCtl(tVFS_Node *Node, int ID, void *Arg);
void	RTL8139_IRQHandler(int Num, void *Ptr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, RTL8139, RTL8139_Install, NULL, NULL);
tVFS_NodeType	gRTL8139_RootNodeType = {
	.ReadDir = RTL8139_ReadDir,
	.FindDir = RTL8139_FindDir,
	.IOCtl = RTL8139_IOCtl
	};
tVFS_NodeType	gRTL8139_DevNodeType = {
	.Write = RTL8139_Write,
	.Read = RTL8139_Read,
	.IOCtl = RTL8139_IOCtl	
	};
tDevFS_Driver	gRTL8139_DriverInfo = {
	NULL, "RTL8139",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Type = &gRTL8139_RootNodeType
	}
};
 int	giRTL8139_CardCount;
tCard	*gaRTL8139_Cards;

// === CODE ===
/**
 * \brief Installs the RTL8139 Driver
 */
int RTL8139_Install(char **Options)
{
	 int	id = -1;
	 int	i = 0;
	Uint16	base;
	tCard	*card;
	
	giRTL8139_CardCount = PCI_CountDevices(VENDOR_ID, DEVICE_ID);
	
	if( giRTL8139_CardCount == 0 )	return MODULE_ERR_NOTNEEDED;

	Log_Debug("RTL8139", "%i cards", giRTL8139_CardCount);	
	gaRTL8139_Cards = calloc( giRTL8139_CardCount, sizeof(tCard) );
	
	for( i = 0 ; (id = PCI_GetDevice(VENDOR_ID, DEVICE_ID, i)) != -1; i ++ )
	{
		card = &gaRTL8139_Cards[i];
		base = PCI_GetBAR( id, 0 );
		if( !(base & 1) ) {
			Log_Warning("RTL8139", "Driver does not support MMIO, skipping card (addr %x)",
				base);
			card->IOBase = 0;
			card->IRQ = 0;
			continue ;
		}
		base &= ~1;
		card->IOBase = base;
		card->IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
		IRQ_AddHandler(card->IRQ, RTL8139_IRQHandler, card);
		
		// Power on
		outb( base + CONFIG1, 0x00 );

		// Reset (0x10 to CMD)
		outb( base + CMD, 0x10 );	
		while( inb(base + CMD) & 0x10 )	;
		
		// Set up recieve buffer
		// - Allocate 3 pages below 4GiB for the recieve buffer (Allows 8k+16+1500)
		card->ReceiveBuffer = (void*)MM_AllocDMA( 3, 32, &card->PhysReceiveBuffer );
		card->ReceiveBufferLength = 8*1024;
		outd(base + RBSTART, (Uint32)card->PhysReceiveBuffer);
		outd(base + CBA, 0);
		outd(base + CAPR, 0);
		// Set IMR to Transmit OK and Receive OK
		outw(base + IMR, 0x5);
		
		// Set up transmit buffers
		// - 2 non-contiguous pages (each page can fit 2 1500 byte packets)
		card->TransmitBuffers[0] = (void*)MM_AllocDMA( 1, 32, &card->PhysTransmitBuffers[0] );
		card->TransmitBuffers[1] = card->TransmitBuffers[0] + 0x800;
		card->PhysTransmitBuffers[1] = card->PhysTransmitBuffers[0] + 0x800;
		
		card->TransmitBuffers[2] = (void*)MM_AllocDMA( 1, 32, &card->PhysTransmitBuffers[2] );
		card->TransmitBuffers[3] = card->TransmitBuffers[2] + 0x800;
		card->PhysTransmitBuffers[3] = card->PhysTransmitBuffers[2] + 0x800;
		
		outd(base + TSAD0, card->PhysTransmitBuffers[0]);
		outd(base + TSAD1, card->PhysTransmitBuffers[1]);
		outd(base + TSAD2, card->PhysTransmitBuffers[2]);
		outd(base + TSAD3, card->PhysTransmitBuffers[3]);
		
		// Set recieve buffer size and recieve mask
		// - Bit 7 being set tells the card to overflow the recieve buffer if needed
		//   (i.e. when the packet starts at the end of the bufffer, it overflows up
		//    to 1500 bytes)
		outd(base + RCR, 0x8F);
	
		// Recive Enable and Transmit Enable	
		outb(base + CMD, 0x0C);
		
		// Get the card's MAC address
		card->MacAddr[0] = inb(base+MAC0);
		card->MacAddr[1] = inb(base+MAC1);
		card->MacAddr[2] = inb(base+MAC2);
		card->MacAddr[3] = inb(base+MAC3);
		card->MacAddr[4] = inb(base+MAC4);
		card->MacAddr[5] = inb(base+MAC5);
		
		// Set VFS Node
		card->Name[0] = '0'+i;
		card->Name[1] = '\0';
		card->Node.ImplPtr = card;
		card->Node.NumACLs = 0;
		card->Node.CTime = now();
		card->Node.Type = &gRTL8139_DevNodeType;
		
		Log_Log("RTL8139", "Card %i 0x%04x, IRQ %i %02x:%02x:%02x:%02x:%02x:%02x",
			i, card->IOBase, card->IRQ,
			card->MacAddr[0], card->MacAddr[1], card->MacAddr[2],
			card->MacAddr[3], card->MacAddr[4], card->MacAddr[5]
			);
	}
	
	gRTL8139_DriverInfo.RootNode.Size = giRTL8139_CardCount;
	DevFS_AddDevice( &gRTL8139_DriverInfo );
	
	return MODULE_ERR_OK;
}

// --- Root Functions ---
char *RTL8139_ReadDir(tVFS_Node *Node, int Pos)
{
	if( Pos < 0 || Pos >= giRTL8139_CardCount )	return NULL;
	
	return strdup( gaRTL8139_Cards[Pos].Name );
}

tVFS_Node *RTL8139_FindDir(tVFS_Node *Node, const char *Filename)
{
	//TODO: It might be an idea to supprt >10 cards
	if(Filename[0] == '\0' || Filename[1] != '\0')	return NULL;
	if(Filename[0] < '0' || Filename[0] > '9')	return NULL;
	return &gaRTL8139_Cards[ Filename[0]-'0' ].Node;
}

const char *csaRTL8139_RootIOCtls[] = {DRV_IOCTLNAMES, NULL};
int RTL8139_RootIOCtl(tVFS_Node *Node, int ID, void *Data)
{
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "RTL8139", VERSION, csaRTL8139_RootIOCtls);
	}
	LEAVE('i', 0);
	return 0;
}

// --- File Functions ---
Uint64 RTL8139_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tCard	*card = Node->ImplPtr;
	Uint16	read_ofs, pkt_length;
	 int	new_read_ofs;

	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);

retry:
	if( Semaphore_Wait( &card->ReadSemaphore, 1 ) != 1 )
	{
		LEAVE_RET('i', 0);
	}
	
	Mutex_Acquire( &card->ReadMutex );
	
	read_ofs = inw( card->IOBase + CAPR );
	LOG("raw read_ofs = %i", read_ofs);
	read_ofs = (read_ofs + 0x10) & 0xFFFF;
	LOG("read_ofs = %i", read_ofs);
	
	pkt_length = *(Uint16*)&card->ReceiveBuffer[read_ofs+2];
	
	// Calculate new read offset
	new_read_ofs = read_ofs + pkt_length + 4;
	new_read_ofs = (new_read_ofs + 3) & ~3;	// Align
	if(new_read_ofs > card->ReceiveBufferLength) {
		LOG("wrapping read_ofs");
		new_read_ofs -= card->ReceiveBufferLength;
	}
	new_read_ofs -= 0x10;	// I dunno
	LOG("new_read_ofs = %i", new_read_ofs);
	
	// Check for errors
	if( *(Uint16*)&card->ReceiveBuffer[read_ofs] & 0x1E ) {
		// Update CAPR
		outw(card->IOBase + CAPR, new_read_ofs);
		Mutex_Release( &card->ReadMutex );
		goto retry;	// I feel evil
	}
	
	// Get packet
	if( Length > pkt_length )	Length = pkt_length;
	memcpy(Buffer, &card->ReceiveBuffer[read_ofs+4], Length);
	
	// Update CAPR
	outw(card->IOBase + CAPR, new_read_ofs);
	
	Mutex_Release( &card->ReadMutex );
	
	LEAVE('i', Length);
	
	return Length;
}

Uint64 RTL8139_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, const void *Buffer)
{
	 int	td;
	Uint32	status;
	tCard	*card = Node->ImplPtr;
	
	if( Length > 1500 )	return 0;	// MTU exceeded
	
	ENTER("pNode XLength pBuffer", Node, Length, Buffer);
	
	// TODO: Implement a semaphore for avaliable transmit buffers

	// Find an avaliable descriptor
	Mutex_Acquire(&card->CurTXProtector);
	td = card->CurTXDescriptor;
	card->CurTXDescriptor ++;
	card->CurTXDescriptor %= 4;
	Mutex_Release(&card->CurTXProtector);
	// - Lock it
	Mutex_Acquire( &card->TransmitInUse[td] );
	
	LOG("td = %i", td);
	
	// Transmit using descriptor `td`
	LOG("card->PhysTransmitBuffers[td] = %P", card->PhysTransmitBuffers[td]);
	outd(card->IOBase + TSAD0 + td*4, card->PhysTransmitBuffers[td]);
	LOG("card->TransmitBuffers[td] = %p", card->TransmitBuffers[td]);
	// Copy to buffer
	memcpy(card->TransmitBuffers[td], Buffer, Length);
	// Start
	status = 0;
	status |= Length & 0x1FFF;	// 0-12: Length
	status |= 0 << 13;	// 13: OWN bit
	status |= (0 & 0x3F) << 16;	// 16-21: Early TX threshold (zero atm, TODO: check)
	LOG("status = 0x%08x", status);
	outd(card->IOBase + TSD0 + td*4, status);
	
	LEAVE('i', (int)Length);
	
	return Length;
}

const char *csaRTL8139_NodeIOCtls[] = {DRV_IOCTLNAMES, NULL};
int RTL8139_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tCard	*card = Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "RTL8139", VERSION, csaRTL8139_NodeIOCtls);
	case NET_IOCTL_GETMAC:
		if( !CheckMem(Data, 6) ) {
			LEAVE('i', -1);
			return -1;
		}
		memcpy( Data, card->MacAddr, 6 );
		LEAVE('i', 1);
		return 1;
	}
	LEAVE('i', 0);
	return 0;
}

void RTL8139_IRQHandler(int Num, void *Ptr)
{
	 int	j;
	tCard	*card = Ptr;
	Uint16	status;

	LOG("Num = %i", Num);
	
	if( Num != card->IRQ )	return;
		
	status = inw(card->IOBase + ISR);
	LOG("status = 0x%02x", status);
		
	// Transmit OK, a transmit descriptor is now free
	if( status & FLAG_ISR_TOK )
	{
		for( j = 0; j < 4; j ++ )
		{
			if( ind(card->IOBase + TSD0 + j*4) & 0x8000 ) {	// TSD TOK
				Mutex_Release( &card->TransmitInUse[j] );
				// TODO: Update semaphore once implemented
			}
		}
		outw(card->IOBase + ISR, FLAG_ISR_TOK);
	}
	
	// Recieve OK, inform read
	if( status & FLAG_ISR_ROK )
	{
		 int	read_ofs, end_ofs;
		 int	packet_count = 0;
		 int	len;
		
		// Scan recieve buffer for packets
		end_ofs = inw(card->IOBase + CBA);
		read_ofs = card->SeenOfs;
		LOG("read_ofs = %i, end_ofs = %i", read_ofs, end_ofs);
		if( read_ofs > end_ofs )
		{
			while( read_ofs < card->ReceiveBufferLength )
			{
				packet_count ++;
				len = *(Uint16*)&card->ReceiveBuffer[read_ofs+2];
				LOG("%i 0x%x Pkt Hdr: 0x%04x, len: 0x%04x",
					packet_count, read_ofs,
					*(Uint16*)&card->ReceiveBuffer[read_ofs],
					len
					);
				if(len > 2000) {
					Log_Warning("RTL8139", "IRQ: Packet in buffer exceeds sanity (%i>2000)", len);
				}
				read_ofs += len + 4;
				read_ofs = (read_ofs + 3) & ~3;	// Align
			}
			read_ofs -= card->ReceiveBufferLength;
			LOG("wrapped read_ofs");
		}
		while( read_ofs < end_ofs )
		{
			packet_count ++;
			LOG("%i 0x%x Pkt Hdr: 0x%04x, len: 0x%04x",
				packet_count, read_ofs,
				*(Uint16*)&card->ReceiveBuffer[read_ofs],
				*(Uint16*)&card->ReceiveBuffer[read_ofs+2]
				);
			read_ofs += *(Uint16*)&card->ReceiveBuffer[read_ofs+2] + 4;
			read_ofs = (read_ofs + 3) & ~3;	// Align
		}
		if( read_ofs != end_ofs ) {
			Log_Warning("RTL8139", "IRQ: read_ofs (%i) != end_ofs(%i)", read_ofs, end_ofs);
			read_ofs = end_ofs;
		}
		card->SeenOfs = read_ofs;
		
		LOG("packet_count = %i, read_ofs = 0x%x", packet_count, read_ofs);
		
		if( packet_count )
		{
			if( Semaphore_Signal( &card->ReadSemaphore, packet_count ) != packet_count ) {
				// Oops?
			}
			VFS_MarkAvaliable( &card->Node, 1 );
		}
		
		outw(card->IOBase + ISR, FLAG_ISR_ROK);
	}	
}

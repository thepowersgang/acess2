/*
 * Acess2 RTL8139 Driver
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	0
#define VERSION	((0<<8)|50)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_network.h>
#include <semaphore.h>

// === CONSTANTS ===
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
	BOOL	TransmitInUse;	// Flags for each transmit descriptor
	
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
Uint64	RTL8139_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	RTL8139_IOCtl(tVFS_Node *Node, int ID, void *Arg);
void	RTL8139_IRQHandler(int Num);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, RTL8139, RTL8139_Install, NULL, NULL);
tDevFS_Driver	gRTL8139_DriverInfo = {
	NULL, "RTL8139",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = RTL8139_ReadDir,
	.FindDir = RTL8139_FindDir,
	.IOCtl = RTL8139_RootIOCtl
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
	
	giRTL8139_CardCount = PCI_CountDevices( 0x10EC, 0x8139, 0 );
	
	gaRTL8139_Cards = calloc( giRTL8139_CardCount, sizeof(tCard) );
	
	
	while( (id = PCI_GetDevice(0x10EC, 0x8139, 0, id)) != -1 )
	{
		base = PCI_AssignPort( id, 0, 0x100 );
		card = &gaRTL8139_Cards[i];
		card->IOBase = base;
		card->IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
		IRQ_AddHandler(card->IRQ, RTL8139_IRQHandler);
		
		// Power on
		outb( base + CONFIG1, 0x00 );

		// Reset (0x10 to CMD)
		outb( base + CMD, 0x10 );	
		while( inb(base + CMD) & 0x10 )	;
		
		// Set up recieve buffer
		// - Allocate 3 pages below 4GiB for the recieve buffer (Allows 8k+16+1500)
		card->ReceiveBuffer = (void*)MM_AllocDMA( 3, 32, &card->PhysReceiveBuffer );
		outd(base + RBSTART, (Uint32)card->PhysReceiveBuffer);
		// Set IMR to Transmit OK and Receive OK
		outw(base + IMR, 0x5);
		
		// Set up transmit buffers
		// - 2 non-contiguous pages (each page can fit 2 1500 byte packets)
		card->TransmitBuffers[0] = (void*)MM_AllocDMA( 1, 32, &card->PhysTransmitBuffers[0] );
		card->TransmitBuffers[1] = card->TransmitBuffers[0] + 0x800;
		card->PhysTransmitBuffers[1] = card->PhysTransmitBuffers[0] + 0x800;
		
		card->TransmitBuffers[2] = (void*)MM_AllocDMA( 1, 32, &card->PhysTransmitBuffers[1] );
		card->TransmitBuffers[3] = card->TransmitBuffers[2] + 0x800;
		card->PhysTransmitBuffers[3] = card->PhysTransmitBuffers[2] + 0x800;
		
		outd(base + TSAD0, card->PhysTransmitBuffers[0]);
		outd(base + TSAD1, card->PhysTransmitBuffers[1]);
		outd(base + TSAD2, card->PhysTransmitBuffers[2]);
		outd(base + TSAD3, card->PhysTransmitBuffers[3]);
		
		// Set recieve buffer size and recieve mask
		// - Bit 7 being unset tells the card to overflow the recieve buffer if needed
		//   (i.e. when the packet starts at the end of the bufffer, it overflows up
		//    to 1500 bytes)
		outd(base + RCR, 0x0F);
	
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
		card->Node.Write = RTL8139_Write;
		card->Node.Read = RTL8139_Read;
		card->Node.IOCtl = RTL8139_IOCtl;
		
		Log_Log("RTL8139", "Card %i 0x%04x %02x:%02x:%02x:%02x:%02x:%02x",
			i, base,
			card->MacAddr[0], card->MacAddr[1], card->MacAddr[2],
			card->MacAddr[3], card->MacAddr[4], card->MacAddr[5]
			);
		
		i ++;
	}
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
	if(Filename[0] == '\0' || Filename[0] != '\0')	return NULL;
	if(Filename[0] < '0' || Filename[0] > '9')	return NULL;
	return &gaRTL8139_Cards[ Filename[0]-'0' ].Node;
}

const char *csaRTL8139_RootIOCtls[] = {DRV_IOCTLNAMES, NULL};
int RTL8139_RootIOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "RTL8139", VERSION, csaRTL8139_RootIOCtls);
	}
	return 0;
}

// --- File Functions ---
Uint64 RTL8139_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tCard	*card = Node->ImplPtr;
	 int	read_ofs, pkt_length;

retry:
	Semaphore_Wait( &card->ReadSemaphore, 1 );
	
	Mutex_Acquire( &card->ReadMutex );
	
	read_ofs = ind( card->IOBase + CAPR ) - card->PhysReceiveBuffer;
	
	// Check for errors
	if( *(Uint16*)&card->ReceiveBuffer[read_ofs] & 0x1E ) {
		Mutex_Release( &card->ReadMutex );
		goto retry;	// I feel evil
	}
	
	pkt_length = *(Uint16*)&card->ReceiveBuffer[read_ofs+2];
	
	// Get packet
	if( Length > pkt_length )	Length = pkt_length;
	memcpy(Buffer, &card->ReceiveBuffer[read_ofs+4], Length);
	
	// Update read offset
	read_ofs += pkt_length + 4;
	read_ofs = (read_ofs + 3) & ~3;	// Align
	if(read_ofs > card->ReceiveBufferLength)	read_ofs = 0;
	outd(card->IOBase + CAPR, read_ofs + card->PhysReceiveBuffer);
	
	Mutex_Release( &card->ReadMutex );
	
	return Length;
}

Uint64 RTL8139_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	td;
	Uint32	status;
	tCard	*card = Node->ImplPtr;
	
	if( Length > 1500 )	return 0;	// MTU exceeded
	
	// TODO: Implement a semaphore for avaliable tranmit buffers
	
	// Find an avaliable descriptor
	do {
		for( td = 0; td < 4; td ++ ) {
			if( !(card->TransmitInUse & (1 << td)) )
				break;
		}
	} while(td == 4 && (Threads_Yield(),1));
	
	// Transmit using descriptor `td`
	card->TransmitInUse |= (1 << td);
	// Copy to buffer
	memcpy(card->TransmitBuffers[td], Buffer, Length);
	// Start
	status = 0;
	status |= Length & 0x1FFF;	// 0-12: Length
	status |= 0 << 13;	// 13: OWN bit
	status |= (0 & 0x3F) << 16;	// 16-21: Early TX threshold (zero atm, TODO: check)
	outd(card->IOBase + TSD0 + td, status);
	
	return 0;
}

const char *csaRTL8139_NodeIOCtls[] = {DRV_IOCTLNAMES, NULL};
int RTL8139_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "RTL8139", VERSION, csaRTL8139_NodeIOCtls);
	}
	return 0;
}

void RTL8139_IRQHandler(int Num)
{
	 int	i, j;
	tCard	*card;
	Uint8	status;
	for( i = 0; i < giRTL8139_CardCount; i ++ )
	{
		card = &gaRTL8139_Cards[i];
		if( Num != card->IRQ )	break;
		
		status = inb(card->IOBase + ISR);
		
		// Transmit OK, a transmit descriptor is now free
		if( status & FLAG_ISR_TOK )
		{
			outb(card->IOBase + ISR, FLAG_ISR_TOK);
			for( j = 0; j < 4; j ++ )
			{
				if( ind(card->IOBase + TSD0 + j) & 0x8000 ) {	// TSD TOK
					card->TransmitInUse &= ~(1 << j);
					// TODO: Update semaphore once implemented
				}
			}
		}
		
		// Recieve OK, inform read
		if( status & FLAG_ISR_ROK )
		{
			 int	read_ofs, end_ofs;
			 int	packet_count = 0;
			
			// Scan recieve buffer for packets
			end_ofs = ind(card->IOBase + CBA) - card->PhysReceiveBuffer;
			read_ofs = card->SeenOfs;
			if( read_ofs > end_ofs )
			{
				while( read_ofs < card->ReceiveBufferLength )
				{
					packet_count ++;
					read_ofs += *(Uint16*)&card->ReceiveBuffer[read_ofs+1] + 2;
				}
				read_ofs = 0;
			}
			while( read_ofs < end_ofs )
			{
				packet_count ++;
				read_ofs += *(Uint16*)&card->ReceiveBuffer[read_ofs+1] + 2;
			}
			card->SeenOfs = read_ofs;
			
			Semaphore_Signal( &card->ReadSemaphore, packet_count );
			if( packet_count )
				VFS_MarkAvaliable( &card->Node, 1 );
			
			outb(card->IOBase + ISR, FLAG_ISR_ROK);
		}
	}
}

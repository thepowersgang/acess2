/* Acess2
 * NE2000 Driver
 * 
 * See: ~/Sources/bochs/bochs.../iodev/ne2k.cc
 */
#define	DEBUG	0
#define VERSION	((0<<8)|50)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_network.h>

// === CONSTANTS ===
#define	MEM_START	0x40
#define	MEM_END		0xC0
#define RX_FIRST	(MEM_START)
#define RX_LAST		(MEM_START+RX_BUF_SIZE-1)
#define	RX_BUF_SIZE	0x40
#define TX_FIRST	(MEM_START+RX_BUF_SIZE)
#define TX_LAST		(MEM_END)
#define	TX_BUF_SIZE	0x40
#define	MAX_PACKET_QUEUE	10

static const struct {
	Uint16	Vendor;
	Uint16	Device;
} csaCOMPAT_DEVICES[] = {
	{0x10EC, 0x8029},	// Realtek 8029
	{0x10EC, 0x8129}	// Realtek 8129
};
#define NUM_COMPAT_DEVICES	(sizeof(csaCOMPAT_DEVICES)/sizeof(csaCOMPAT_DEVICES[0]))

enum eNe2k_Page0Read {
	CMD = 0,	//!< the master command register
	CLDA0,		//!< Current Local DMA Address 0
	CLDA1,		//!< Current Local DMA Address 1
	BNRY,		//!< Boundary Pointer (for ringbuffer)
	TSR,		//!< Transmit Status Register
	NCR,		//!< collisions counter
	FIFO,		//!< (for what purpose ??)
	ISR,		//!< Interrupt Status Register
	CRDA0,		//!< Current Remote DMA Address 0
	CRDA1,		//!< Current Remote DMA Address 1
	RSR = 0xC	//!< Receive Status Register
};

enum eNe2k_Page0Write {
	PSTART = 1,	//!< page start (init only)
	PSTOP,		//!< page stop  (init only)
	TPSR = 4,	//!< transmit page start address
	TBCR0,		//!< transmit byte count (low)
	TBCR1,		//!< transmit byte count (high)
	RSAR0 = 8,	//!< remote start address (lo)
	RSAR1,	//!< remote start address (hi)
	RBCR0,	//!< remote byte count (lo)
	RBCR1,	//!< remote byte count (hi)
	RCR,	//!< receive config register
	TCR,	//!< transmit config register
	DCR,	//!< data config register    (init)
	IMR		//!< interrupt mask register (init)
};

enum eNe2k_Page1Read {
	CURR = 7	//!< current page
};

// === TYPES ===
typedef struct sNe2k_Card {
	Uint16	IOBase;	//!< IO Port Address from PCI
	Uint8	IRQ;	//!< IRQ Assigned from PCI
	
	 int	NumWaitingPackets;
	 int	NextRXPage;
	
	 int	NextMemPage;	//!< Next Card Memory page to use
	
	Uint8	Buffer[RX_BUF_SIZE*256];
	
	char	Name[2];	// "0"
	tVFS_Node	Node;
	Uint8	MacAddr[6];
} tCard;

// === PROTOTYPES ===
 int	Ne2k_Install(char **Arguments);
char	*Ne2k_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Ne2k_FindDir(tVFS_Node *Node, char *Name);
 int	Ne2k_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Ne2k_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint8	Ne2k_int_GetWritePage(tCard *Card, Uint16 Length);
void	Ne2k_IRQHandler(int IntNum);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Ne2k, Ne2k_Install, NULL, NULL);
tDevFS_Driver	gNe2k_DriverInfo = {
	NULL, "ne2k",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = Ne2k_ReadDir,
	.FindDir = Ne2k_FindDir,
	.IOCtl = Ne2k_IOCtl
	}
};
Uint16	gNe2k_BaseAddress;
 int	giNe2k_CardCount = 0;
tCard	*gpNe2k_Cards = NULL;

// === CODE ===
/**
 * \fn int Ne2k_Install(char **Options)
 * \brief Installs the NE2000 Driver
 */
int Ne2k_Install(char **Options)
{
	 int	i, j, k;
	 int	count, id, base;
	
	// --- Scan PCI Bus ---
	// Count Cards
	giNe2k_CardCount = 0;
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		giNe2k_CardCount += PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
	}
	
	// Enumerate Cards
	k = 0;
	gpNe2k_Cards = calloc( giNe2k_CardCount, sizeof(tCard) );
	
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		count = PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
		for( j = 0; j < count; j ++,k ++ )
		{
			id = PCI_GetDevice( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0, j );
			// Create Structure
			base = PCI_AssignPort( id, 0, 0x20 );
			gpNe2k_Cards[ k ].IOBase = base;
			gpNe2k_Cards[ k ].IRQ = PCI_GetIRQ( id );
			gpNe2k_Cards[ k ].NextMemPage = 64;
			gpNe2k_Cards[ k ].NextRXPage = RX_FIRST;
			
			// Install IRQ Handler
			IRQ_AddHandler(gpNe2k_Cards[ k ].IRQ, Ne2k_IRQHandler);
			
			// Reset Card
			outb( base + 0x1F, inb(base + 0x1F) );
			while( (inb( base+ISR ) & 0x80) == 0 );
			outb( base + ISR, 0x80 );
			
			// Initialise Card
			outb( base + CMD, 0x40|0x21 );	// Page 1, No DMA, Stop
			outb( base + CURR, RX_FIRST );	// Current RX page
			outb( base + CMD, 0x21 );	// No DMA and Stop
			outb( base + DCR, 0x49 );	// Set WORD mode
			outb( base + IMR, 0x00 );
			outb( base + ISR, 0xFF );
			outb( base + RCR, 0x20 );	// Reciever to Monitor
			outb( base + TCR, 0x02 );	// Transmitter OFF (TCR.LB = 1, Internal Loopback)
			outb( base + RBCR0, 6*4 );	// Remote Byte Count
			outb( base + RBCR1, 0 );
			outb( base + RSAR0, 0 );	// Clear Source Address
			outb( base + RSAR1, 0 );
			outb( base + CMD, 0x0A );	// Remote Read, Start
			
			// Read MAC Address
			gpNe2k_Cards[ k ].MacAddr[0] = inb(base+0x10);//	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[1] = inb(base+0x10);//	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[2] = inb(base+0x10);//	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[3] = inb(base+0x10);//	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[4] = inb(base+0x10);//	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[5] = inb(base+0x10);//	inb(base+0x10);
			
			outb( base+PSTART, RX_FIRST);	// Set Receive Start
			outb( base+BNRY, RX_LAST-1);	// Set Boundary Page
			outb( base+PSTOP, RX_LAST);	// Set Stop Page
			outb( base+ISR, 0xFF );	// Clear all ints
			outb( base+CMD, 0x22 );	// No DMA, Start
			outb( base+IMR, 0x3F );	// Set Interupt Mask
			outb( base+RCR, 0x0F );	// Set WRAP and allow all packet matches
			outb( base+TCR, 0x00 );	// Set Normal Transmitter mode
			outb( base+TPSR, 0x40);	// Set Transmit Start
			// Set MAC Address
			/*
			Ne2k_WriteReg(base, MAC0, gpNe2k_Cards[ k ].MacAddr[0]);
			Ne2k_WriteReg(base, MAC1, gpNe2k_Cards[ k ].MacAddr[1]);
			Ne2k_WriteReg(base, MAC2, gpNe2k_Cards[ k ].MacAddr[2]);
			Ne2k_WriteReg(base, MAC3, gpNe2k_Cards[ k ].MacAddr[3]);
			Ne2k_WriteReg(base, MAC4, gpNe2k_Cards[ k ].MacAddr[4]);
			Ne2k_WriteReg(base, MAC5, gpNe2k_Cards[ k ].MacAddr[5]);
			*/
			
			Log("[NE2K]: Card #%i: IRQ=%i, IOBase=0x%x",
				k, gpNe2k_Cards[ k ].IRQ, gpNe2k_Cards[ k ].IOBase);
			Log("MAC Address %x:%x:%x:%x:%x:%x",
				gpNe2k_Cards[ k ].MacAddr[0], gpNe2k_Cards[ k ].MacAddr[1],
				gpNe2k_Cards[ k ].MacAddr[2], gpNe2k_Cards[ k ].MacAddr[3],
				gpNe2k_Cards[ k ].MacAddr[4], gpNe2k_Cards[ k ].MacAddr[5]
				);
			
			// Set VFS Node
			gpNe2k_Cards[ k ].Name[0] = '0'+k;
			gpNe2k_Cards[ k ].Name[1] = '\0';
			gpNe2k_Cards[ k ].Node.ImplPtr = &gpNe2k_Cards[ k ];
			gpNe2k_Cards[ k ].Node.NumACLs = 0;	// Root Only
			gpNe2k_Cards[ k ].Node.CTime = now();
			gpNe2k_Cards[ k ].Node.Write = Ne2k_Write;
			gpNe2k_Cards[ k ].Node.Read = Ne2k_Read;
			gpNe2k_Cards[ k ].Node.IOCtl = Ne2k_IOCtl;
		}
	}
	
	gNe2k_DriverInfo.RootNode.Size = giNe2k_CardCount;
	DevFS_AddDevice( &gNe2k_DriverInfo );
	return 1;
}

/**
 * \fn char *Ne2k_ReadDir(tVFS_Node *Node, int Pos)
 */
char *Ne2k_ReadDir(tVFS_Node *Node, int Pos)
{
	char	ret[2];
	if(Pos < 0 || Pos >= giNe2k_CardCount)	return NULL;
	ret[0] = '0'+Pos;
	ret[1] = '\0';
	return strdup(ret);
}

/**
 * \fn tVFS_Node *Ne2k_FindDir(tVFS_Node *Node, char *Name)
 */
tVFS_Node *Ne2k_FindDir(tVFS_Node *Node, char *Name)
{
	if(Name[0] == '\0' || Name[1] != '\0')	return NULL;
	
	return &gpNe2k_Cards[ Name[0]-'0' ].Node;
}

static const char *casIOCtls[] = { DRV_IOCTLNAMES, DRV_NETWORK_IOCTLNAMES, NULL };
/**
 * \fn int Ne2k_IOCtl(tVFS_Node *Node, int ID, void *Data)
 * \brief IOCtl calls for a network device
 */
int Ne2k_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	 int	tmp;
	ENTER("pNode iID pData", Node, ID, Data);
	switch( ID )
	{
	case DRV_IOCTL_TYPE:
		LEAVE('i', DRV_TYPE_NETWORK);
		return DRV_TYPE_NETWORK;
	
	case DRV_IOCTL_IDENT:
		tmp = ModUtil_SetIdent(Data, "Ne2k");
		LEAVE('i', tmp);
		return tmp;
	
	case DRV_IOCTL_VERSION:
		LEAVE('x', VERSION);
		return VERSION;
	
	case DRV_IOCTL_LOOKUP:
		tmp = ModUtil_LookupString( (char**)casIOCtls, Data );
		LEAVE('i', tmp);
		return tmp;
	}
	
	// If this is the root, return
	if( Node == &gNe2k_DriverInfo.RootNode ) {
		LEAVE('i', 0);
		return 0;
	}
	
	// Device specific settings
	switch( ID )
	{
	case NET_IOCTL_GETMAC:
		if( !CheckMem(Data, 6) ) {
			LEAVE('i', -1);
			return -1;
		}
		memcpy( Data, ((tCard*)Node->ImplPtr)->MacAddr, 6 );
		LEAVE('i', 1);
		return 1;
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn Uint64 Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Send a packet from the network card
 */
Uint64 Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tCard	*Card = (tCard*)Node->ImplPtr;
	Uint16	*buf = Buffer;
	 int	rem = Length;
	 int	page;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	// Sanity Check Length
	if(Length > TX_BUF_SIZE*256) {
		Warning(
			"Ne2k_Write - Attempting to send over TX_BUF_SIZE(%i) bytes (%i)",
			TX_BUF_SIZE*256, Length
			);
		LEAVE('i', 0);
		return 0;
	}
	
	// Make sure that the card is in page 0
	outb(Card->IOBase + CMD, 0|0x22);	// Page 0, Start, NoDMA
	
	// Clear Remote DMA Flag
	outb(Card->IOBase + ISR, 0x40);	// Bit 6
	
	// Send Size - Remote Byte Count Register
	outb(Card->IOBase + TBCR0, Length & 0xFF);
	outb(Card->IOBase + TBCR1, Length >> 8);
	
	// Send Size - Remote Byte Count Register
	outb(Card->IOBase + RBCR0, Length & 0xFF);
	outb(Card->IOBase + RBCR1, Length >> 8);
	
	// Set up transfer
	outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
	page = Ne2k_int_GetWritePage(Card, Length);
	outb(Card->IOBase + RSAR1, page);	// Page Offset
	// Start
	//outb(Card->IOBase + CMD, 0|0x18|0x4|0x2);	// Page 0, Transmit Packet, TXP, Start
	outb(Card->IOBase + CMD, 0|0x10|0x2);	// Page 0, Remote Write, Start
	
	// Send Data
	for(rem = Length; rem; rem -= 2)
		outw(Card->IOBase + 0x10, *buf++);
	
	while( inb(Card->IOBase + ISR) == 0 )	// Wait for Remote DMA Complete
		;	//Proc_Yield();
	
	outb( Card->IOBase + ISR, 0x40 );	// ACK Interrupt
	
	// Send Packet
	outb(Card->IOBase + TPSR, page);
	outb(Card->IOBase + CMD, 0|0x10|0x4|0x2);
	
	// Complete DMA
	//outb(Card->IOBase + CMD, 0|0x20);
	
	LEAVE('i', Length);
	return Length;
}

/**
 * \fn Uint64 Ne2k_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Wait for and read a packet from the network card
 */
Uint64 Ne2k_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tCard	*Card = (tCard*)Node->ImplPtr;
	Uint8	page;
	Uint8	data[256];
	 int	i;
	struct {
		Uint8	Status;
		Uint8	NextPacketPage;
		Uint16	Length;	// Little Endian
	}	*pktHdr;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	while(Card->NumWaitingPackets == 0)	Threads_Yield();
	
	// Make sure that the card is in page 0
	outb(Card->IOBase + CMD, 0|0x22);	// Page 0, Start, NoDMA
	
	// Get BOUNDARY
	page = Card->NextRXPage;
	
	// Set up transfer
	outb(Card->IOBase + RBCR0, 0);
	outb(Card->IOBase + RBCR1, 1);	// 256-bytes
	outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
	outb(Card->IOBase + RSAR1, page);	// Page Number
	
	outb(Card->IOBase + CMD, 0|0x08|0x2);	// Page 0, Remote Read, Start
	
	// Clear Remote DMA Flag
	outb(Card->IOBase + ISR, 0x40);	// Bit 6
	
	// Read data
	for(i = 0; i < 128; i ++)
		((Uint16*)data)[i] = inw(Card->IOBase + 0x10);
	
	pktHdr = (void*)data;
	//Log("Ne2k_Read: Recieved packet (%i bytes)", pktHdr->Length);
	
	// Have we read all the required bytes yet?
	if(pktHdr->Length < 256 - 4)
	{
		if(Length > pktHdr->Length)
			Length = pktHdr->Length;
		memcpy(Buffer, &data[4], Length);
		page ++;
		if(page == RX_LAST+1)	page = RX_FIRST;
	}
	// No? oh damn, now we need to allocate a buffer
	else {
		 int	j = 256/2;
		char	*buf = malloc( (pktHdr->Length + 4 + 255) & ~255 );
		
		if(!buf) {
			LEAVE('i', -1);
			return -1;
		}
		
		memcpy(buf, data, 256);
		
		page ++;
		while(page != pktHdr->NextPacketPage)
		{
			if(page == RX_LAST+1)	page = RX_FIRST;
			
			outb(Card->IOBase + RBCR0, 0);
			outb(Card->IOBase + RBCR1, 1);	// 256-bytes
			outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
			outb(Card->IOBase + RSAR1, page);	// Page Number
			outb(Card->IOBase + CMD, 0|0x08|0x2);	// Page 0, Remote Read, Start
			
			for(i = 0; i < 128; i ++)
				((Uint16*)buf)[j+i] = inw(Card->IOBase + 0x10);
			j += 128;
			page ++;
		}
		
		if(Length > pktHdr->Length)
			Length = pktHdr->Length;
		memcpy(Buffer, &buf[4], Length);
	}
	
	// Write BNRY
	if(page == RX_FIRST)
		outb( Card->IOBase + BNRY, RX_LAST );
	else
		outb( Card->IOBase + BNRY, page-1 );
	// Set next RX Page and decrement the waiting list
	Card->NextRXPage = page;
	Card->NumWaitingPackets --;
	
	LEAVE('i', Length);
	return Length;
}

/**
 * \fn Uint8 Ne2k_int_GetWritePage(tCard *Card, Uint16 Length)
 */
Uint8 Ne2k_int_GetWritePage(tCard *Card, Uint16 Length)
{
	Uint8	ret = Card->NextMemPage;
	
	Card->NextMemPage += (Length + 0xFF) >> 8;
	if(Card->NextMemPage >= TX_LAST) {
		Card->NextMemPage -= TX_BUF_SIZE;
	}
	
	return ret;
}

/**
 * \fn void Ne2k_IRQHandler(int IntNum)
 */
void Ne2k_IRQHandler(int IntNum)
{
	 int	i;
	Uint8	byte;
	for( i = 0; i < giNe2k_CardCount; i++ )
	{
		if(gpNe2k_Cards[i].IRQ == IntNum)
		{
			byte = inb( gpNe2k_Cards[i].IOBase + ISR );
			
			// 0: Packet recieved (no error)
			if( byte & 1 )
			{
				gpNe2k_Cards[i].NumWaitingPackets ++;
				if( gpNe2k_Cards[i].NumWaitingPackets > MAX_PACKET_QUEUE )
					gpNe2k_Cards[i].NumWaitingPackets = MAX_PACKET_QUEUE;
			}
			// 1: Packet sent (no error)
			// 2: Recieved with error
			// 3: Transmission Halted (Excessive Collisions)
			// 4: Recieve Buffer Exhausted
			// 5: 
			// 6: Remote DMA Complete
			// 7: Reset
			//LOG("Clearing interrupts on card %i (was 0x%x)\n", i, byte);
			outb( gpNe2k_Cards[i].IOBase + ISR, 0xFF );	// Reset All
			return ;
		}
	}
	Warning("[NE2K ] Recieved Unknown IRQ %i", IntNum);
}

/* Acess2
 * NE2000 Driver
 * 
 * See: ~/Sources/bochs/bochs.../iodev/ne2k.cc
 */
#define	DEBUG	1
#define VERSION	((0<<8)|50)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_network.h>
#include <semaphore.h>

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
#define NUM_COMPAT_DEVICES	((int)(sizeof(csaCOMPAT_DEVICES)/sizeof(csaCOMPAT_DEVICES[0])))

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
	
	tSemaphore	Semaphore;	//!< Semaphore for incoming packets
	 int	NextRXPage; 	//!< Next expected RX page
	
	 int	NextMemPage;	//!< Next Card Memory page to use
		
	char	Name[2];	// "0"
	tVFS_Node	Node;	//!< VFS Node
	Uint8	MacAddr[6];	//!< Cached MAC address
} tCard;

// === PROTOTYPES ===
 int	Ne2k_Install(char **Arguments);
char	*Ne2k_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Ne2k_FindDir(tVFS_Node *Node, const char *Name);
 int	Ne2k_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Ne2k_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);

 int	Ne2k_int_ReadDMA(tCard *Card, int FirstPage, int NumPages, void *Buffer);
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
	
	if( giNe2k_CardCount == 0 ) {
		Log_Warning("Ne2k", "No cards detected");
		return MODULE_ERR_NOTNEEDED;
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
			outb( base + IMR, 0x00 );	// Interrupt Mask Register
			outb( base + ISR, 0xFF );
			outb( base + RCR, 0x20 );	// Reciever to Monitor
			outb( base + TCR, 0x02 );	// Transmitter OFF (TCR.LB = 1, Internal Loopback)
			
			// Read MAC Address
			outb( base + RBCR0, 6*4 );	// Remote Byte Count
			outb( base + RBCR1, 0 );
			outb( base + RSAR0, 0 );	// Clear Source Address
			outb( base + RSAR1, 0 );
			outb( base + CMD, 0x0A );	// Remote Read, Start
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
			
			Log_Log("Ne2k", "Card %i 0x%04x IRQ%i %02x:%02x:%02x:%02x:%02x:%02x",
				k, base, gpNe2k_Cards[ k ].IRQ,
				gpNe2k_Cards[k].MacAddr[0], gpNe2k_Cards[k].MacAddr[1],
				gpNe2k_Cards[k].MacAddr[2], gpNe2k_Cards[k].MacAddr[3],
				gpNe2k_Cards[k].MacAddr[4], gpNe2k_Cards[k].MacAddr[5]
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
			
			// Initialise packet semaphore
			// - Start at zero, no max
			Semaphore_Init( &gpNe2k_Cards[k].Semaphore, 0, 0, "NE2000", gpNe2k_Cards[ k ].Name );
		}
	}
	
	gNe2k_DriverInfo.RootNode.Size = giNe2k_CardCount;
	DevFS_AddDevice( &gNe2k_DriverInfo );
	return MODULE_ERR_OK;
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
 * \fn tVFS_Node *Ne2k_FindDir(tVFS_Node *Node, const char *Name)
 */
tVFS_Node *Ne2k_FindDir(tVFS_Node *Node, const char *Name)
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
	ENTER("pNode iID pData", Node, ID, Data);
	switch( ID )
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "NE2000", VERSION, casIOCtls);
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
		Log_Warning(
			"Ne2k",
			"Ne2k_Write - Attempting to send over TX_BUF_SIZE*256 (%i) bytes (%i)",
			TX_BUF_SIZE*256, Length
			);
		LEAVE('i', 0);
		return 0;
	}
	
	// Make sure that the card is in page 0
	outb(Card->IOBase + CMD, 0|0x22);	// Page 0, Start, NoDMA
	
	// Clear Remote DMA Flag
	outb(Card->IOBase + ISR, 0x40);	// Bit 6
	
	// Send Size - Transfer Byte Count Register
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
	outb(Card->IOBase + CMD, 0|0x10|0x2);	// Page 0, Remote Write, Start
	
	// Send Data
	for(rem = Length; rem > 0; rem -= 2) {
		outw(Card->IOBase + 0x10, *buf++);
	}
	
	while( !(inb(Card->IOBase + ISR) & 0x40) )	// Wait for Remote DMA Complete
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
	struct {
		Uint8	Status;
		Uint8	NextPacketPage;
		Uint16	Length;	// Little Endian
	}	*pktHdr;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	// Wait for packets
	Semaphore_Wait( &Card->Semaphore, 1 );
	
	outb(Card->IOBase, 0x22 | (1 << 6));	// Page 6
	LOG("CURR : 0x%02x", inb(Card->IOBase + CURR));
	
	// Get current read page
	page = Card->NextRXPage;
	LOG("page = %i", page);
	
	Ne2k_int_ReadDMA(Card, page, 1, data);
	
	pktHdr = (void*)data;
	
	LOG("pktHdr->Status = 0x%x", pktHdr->Status);
	LOG("pktHdr->NextPacketPage = %i", pktHdr->NextPacketPage);
	LOG("pktHdr->Length = 0x%03x", pktHdr->Length);
	
	// Have we read all the required bytes yet?
	if(pktHdr->Length < 256 - 4)
	{
		if(Length > pktHdr->Length)
			Length = pktHdr->Length;
		memcpy(Buffer, &data[4], Length);
	}
	// No? oh damn, now we need to allocate a buffer
	else
	{
		 int	pages = pktHdr->NextPacketPage - page;
		char	*buf = malloc( pages*256 );
		
		LOG("pktHdr->Length (%i) > 256 - 4, allocated buffer %p", pktHdr->Length, buf);
		
		if(!buf) {
			LEAVE('i', -1);
			return -1;
		}
		
		// Copy the already read data
		memcpy(buf, data, 256);
		
		// Read all the needed pages
		page ++;
		if(page == RX_LAST+1)	page = RX_FIRST;
		
		if( page + pages > RX_LAST )
		{
			 int	first_count = RX_LAST+1 - page;
			 int	tmp = 0;
			tmp += Ne2k_int_ReadDMA(Card, page, first_count, buf+256);
			tmp += Ne2k_int_ReadDMA(Card, RX_FIRST, pages-1-first_count, buf+(first_count+1)*256);
			LOG("composite return count = %i", tmp);
		}
		else
			Ne2k_int_ReadDMA(Card, page, pages-1, buf+256);
		
		// Wrap length to the packet length
		if(Length > pktHdr->Length)
			Length = pktHdr->Length;
		else if( Length < pktHdr->Length ) {
			Log_Warning("NE2000", "Packet truncated! (%i bytes truncated to %i)",
				pktHdr->Length, Length);
		}
		memcpy(Buffer, &buf[4], Length);
		
		free(buf);
	}
	
	// Write BNRY (maximum page for incoming packets)
	if(pktHdr->NextPacketPage == RX_FIRST)
		outb( Card->IOBase + BNRY, RX_LAST-1 );
	else
		outb( Card->IOBase + BNRY, pktHdr->NextPacketPage-1 );
	// Set next RX Page and decrement the waiting list
	Card->NextRXPage = pktHdr->NextPacketPage;
	
	LEAVE('i', Length);
	return Length;
}

int Ne2k_int_ReadDMA(tCard *Card, int FirstPage, int NumPages, void *Buffer)
{
	 int	i;
	
	// Sanity check
	if( !(0 <= FirstPage && FirstPage < 256) ) {
		Log_Warning("NE2000", "Ne2k_int_ReadDMA: BUG - FirstPage(%i) not 8-bit", FirstPage);
		return -1;
	}
	if( !(0 <= NumPages && NumPages < 256) ) {
		Log_Warning("NE2000", "Ne2k_int_ReadDMA: BUG - NumPages(%i) not 8-bit", NumPages);
		return -1;
	}
	
	ENTER("pCard iFirstPage iNumPages pBuffer", Card, FirstPage, NumPages, Buffer);
	
	// Make sure that the card is in bank 0
	outb(Card->IOBase + CMD, 0|0x22);	// Bank 0, Start, NoDMA
	outb(Card->IOBase + ISR, 0x40);	// Clear Remote DMA Flag
	
	// Set up transfer
	outb(Card->IOBase + RBCR0, 0);
	outb(Card->IOBase + RBCR1, NumPages);	// page count
	outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
	outb(Card->IOBase + RSAR1, FirstPage);	// Page Number
	outb(Card->IOBase + CMD, 0|0x08|0x2);	// Bank 0, Remote Read, Start
	
	// TODO: Less expensive
	//while( !(inb(Card->IOBase + ISR) & 0x40) ) {
	//	HALT();
	//	LOG("inb(ISR) = 0x%02x", inb(Card->IOBase + ISR));
	//}
	HALT();	// Small delay?
	
	// Read data
	for(i = 0; i < 128*NumPages; i ++)
		((Uint16*)Buffer)[i] = inw(Card->IOBase + 0x10);
		
	
	outb(Card->IOBase + ISR, 0x40);	// Clear Remote DMA Flag
	
	LEAVE('i', NumPages);
	return NumPages;
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
			
			LOG("byte = 0x%02x", byte);
			
			
			// Reset All (save for RDMA), that's polled
			outb( gpNe2k_Cards[i].IOBase + ISR, 0xFF&(~0x40) );
			
			// 0: Packet recieved (no error)
			if( byte & 1 )
			{
				//if( gpNe2k_Cards[i].NumWaitingPackets > MAX_PACKET_QUEUE )
				//	gpNe2k_Cards[i].NumWaitingPackets = MAX_PACKET_QUEUE;
				Semaphore_Signal( &gpNe2k_Cards[i].Semaphore, 1 );
			}
			// 1: Packet sent (no error)
			// 2: Recieved with error
			// 3: Transmission Halted (Excessive Collisions)
			// 4: Recieve Buffer Exhausted
			// 5: 
			// 6: Remote DMA Complete
			// 7: Reset
			
			return ;
		}
	}
	Log_Warning("Ne2k", "Recieved Unknown IRQ %i", IntNum);
}

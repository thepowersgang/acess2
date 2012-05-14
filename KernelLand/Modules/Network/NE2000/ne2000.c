/*
 * Acess2 NE2000 Driver
 * - By John Hodge (thePowersGang)
 * 
 * See: ~/Sources/bochs/bochs.../iodev/ne2k.cc
 */
#define	DEBUG	1
#define VERSION	VER2(0,6)
#include <acess.h>
#include <modules.h>
#include <drv_pci.h>
#include <semaphore.h>
#include <IPStack/include/adapters_api.h>

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
		
	void	*IPStackHandle;
	Uint8	MacAddr[6];	//!< Cached MAC address
} tCard;

// === PROTOTYPES ===
 int	Ne2k_Install(char **Arguments);

 int	Ne2k_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
tIPStackBuffer	*Ne2k_WaitForPacket(void *Ptr);

 int	Ne2k_int_ReadDMA(tCard *Card, int FirstPage, int NumPages, void *Buffer);
Uint8	Ne2k_int_GetWritePage(tCard *Card, Uint16 Length);
void	Ne2k_IRQHandler(int IntNum, void *Ptr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Ne2k, Ne2k_Install, NULL, NULL);
tIPStack_AdapterType	gNe2k_AdapterType = {
	.Name = "Ne2k",
	.Type = 0,	// TODO: Differentiate differnet wire protos and speeds
	.Flags = 0,	// TODO: IP checksum offloading, MAC checksum offloading etc
	.SendPacket = Ne2k_SendPacket,
	.WaitForPacket = Ne2k_WaitForPacket
};
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
	 int	count, base;
	tPCIDev	id;
	
	// --- Scan PCI Bus ---
	// Count Cards
	giNe2k_CardCount = 0;
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		giNe2k_CardCount += PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device );
	}
	if( giNe2k_CardCount == 0 )	return MODULE_ERR_NOTNEEDED;

	LOG("%i NE2K cards found", giNe2k_CardCount);
	
	// Enumerate Cards
	k = 0;
	gpNe2k_Cards = calloc( giNe2k_CardCount, sizeof(tCard) );
	
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		count = PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device );
		for( j = 0; j < count; j ++, k ++ )
		{
			tCard	*card = &gpNe2k_Cards[k];
			
			id = PCI_GetDevice( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, j );
			// Create Structure
			base = PCI_GetBAR( id, 0 );
			LOG("%i: %i/%i id = %i, base = 0x%x", k, i, j, id, base);
			if( !(base & 1) ) {
				Log_Warning("Ne2k", "PCI %i's BARs are incorrect (BAR0 is not IO)", id);
				continue ;
			}
			base &= ~1;
			card->IOBase = base;
			card->IRQ = PCI_GetIRQ( id );
			card->NextMemPage = 64;
			card->NextRXPage = RX_FIRST;
			
			// Install IRQ Handler
			IRQ_AddHandler(card->IRQ, Ne2k_IRQHandler, &gpNe2k_Cards[k]);
			LOG("%i: IRQ %i mapped, IOBase = 0x%x", k, card->IRQ, card->IOBase);
			
			// Reset Card
			outb( base + 0x1F, inb(base + 0x1F) );
			while( (inb( base + ISR ) & 0x80) == 0 );
			outb( base + ISR, 0x80 );
		
			LOG("Reset done");
	
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
			card->MacAddr[0] = inb(base+0x10);//	inb(base+0x10);
			card->MacAddr[1] = inb(base+0x10);//	inb(base+0x10);
			card->MacAddr[2] = inb(base+0x10);//	inb(base+0x10);
			card->MacAddr[3] = inb(base+0x10);//	inb(base+0x10);
			card->MacAddr[4] = inb(base+0x10);//	inb(base+0x10);
			card->MacAddr[5] = inb(base+0x10);//	inb(base+0x10);
			
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
				k, base, card->IRQ,
				card->MacAddr[0], card->MacAddr[1],
				card->MacAddr[2], card->MacAddr[3],
				card->MacAddr[4], card->MacAddr[5]
				);
			
			card->IPStackHandle = IPStack_Adapter_Add(&gNe2k_AdapterType, card, card->MacAddr);
			
			// Initialise packet semaphore
			// - Start at zero, no max
			char	name[10];
			sprintf(name, "%i", k-1);
			Semaphore_Init( &card->Semaphore, 0, 0, "NE2000", name );
		}
	}
	
	return MODULE_ERR_OK;
}

/**
 * \brief Send a packet from the network card
 */
int Ne2k_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tCard	*Card = Ptr;
	 int	length;
	 int	page;
	
	ENTER("pPtr pBuffer", Ptr, Buffer);
	
	length = IPStack_Buffer_GetLength(Buffer);

	// TODO: Lock
	
	// Sanity Check Length
	if(length > TX_BUF_SIZE*256) {
		Log_Warning(
			"Ne2k",
			"Ne2k_Write - Attempting to send over TX_BUF_SIZE*256 (%i) bytes (%i)",
			TX_BUF_SIZE*256, length
			);
		LEAVE('i', -1);
		return -1;
	}
	
	// Make sure that the card is in page 0
	outb(Card->IOBase + CMD, 0|0x22);	// Page 0, Start, NoDMA
	
	// Clear Remote DMA Flag
	outb(Card->IOBase + ISR, 0x40);	// Bit 6
	
	// Send Size - Transfer Byte Count Register
	outb(Card->IOBase + TBCR0, length & 0xFF);
	outb(Card->IOBase + TBCR1, length >> 8);
	
	// Send Size - Remote Byte Count Register
	outb(Card->IOBase + RBCR0, length & 0xFF);
	outb(Card->IOBase + RBCR1, length >> 8);
	
	// Set up transfer
	outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
	page = Ne2k_int_GetWritePage(Card, length);
	outb(Card->IOBase + RSAR1, page);	// Page Offset
	// Start
	outb(Card->IOBase + CMD, 0|0x10|0x2);	// Page 0, Remote Write, Start
	
	// Send Data
	size_t	buflen;
	const void	*bufptr;
	for(int id = -1; (id = IPStack_Buffer_GetBuffer(Buffer, -1, &buflen, &bufptr)) != -1; )
	{
		const Uint16	*buf = bufptr;
		if( buflen & 1 )
			Log_Notice("NE2000", "Alignment issue in TX buffer");
		buflen = (buflen + 1) / 2;
		while(buflen --)
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
	
	LEAVE('i', 0);
	return 0;
}

void _FreeHeapSubBuf(void *Arg, size_t Pre, size_t Post, const void *DataBuf)
{
	free(Arg);
}

/**
 * \brief Wait for and read a packet from the network card
 */
tIPStackBuffer *Ne2k_WaitForPacket(void *Ptr)
{
	tCard	*Card = Ptr;
	Uint8	page;
	Uint8	data[256];
	void	*buf;
	tIPStackBuffer	*ret;
	struct {
		Uint8	Status;
		Uint8	NextPacketPage;
		Uint16	Length;	// Little Endian
	}	*pktHdr;
	
	ENTER("pPtr", Ptr);
	
	// Wait for packets
	if( Semaphore_Wait( &Card->Semaphore, 1 ) != 1 )
	{
		// Error or interrupted
		LEAVE('n');
		return NULL;
	}
	
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

	ret = IPStack_Buffer_CreateBuffer(1);
	if(!ret)	LEAVE_RET('n', NULL);
	buf = malloc( pktHdr->Length );
	if(!buf)	LEAVE_RET('n', NULL);
	IPStack_Buffer_AppendSubBuffer(ret, pktHdr->Length, 0, buf, _FreeHeapSubBuf, buf);

	// Have we read all the required bytes yet?
	if(pktHdr->Length < 256 - 4)
	{
		memcpy(buf, &data[4], pktHdr->Length);
	}
	// No? oh damn, now we need to allocate a buffer
	else
	{
		 int	pages = pktHdr->NextPacketPage - page;
		Uint8	*writepos = buf;
		 int	rem_bytes = pktHdr->Length;
		
		LOG("pktHdr->Length (%i) > 256 - 4, allocated buffer %p", pktHdr->Length, buf);
		
		
		// Copy the already read data
		memcpy(writepos, data+4, 256-4);
		writepos += 256-4;
		rem_bytes -= 256-4;
		
		// Read all the needed pages
		page ++;
		if(page == RX_LAST+1)	page = RX_FIRST;

		// - Body Pages
		if( pages > 2 )
		{
			if( page + pages - 2 > RX_LAST )
			{
				 int	first_count = RX_LAST - page + 1;
				Ne2k_int_ReadDMA(Card, page, first_count, writepos);
				Ne2k_int_ReadDMA(Card, RX_FIRST, pages-2-first_count, writepos + first_count*256);
				writepos += (pages-2-first_count) * 256;
			}
			else
				Ne2k_int_ReadDMA(Card, page, pages - 2, writepos);
			page += pages - 2;
			if(page > RX_LAST)	page -= (RX_LAST-RX_FIRST)+1;
			writepos += (pages-2) * 256;
			rem_bytes -= (pages-2) * 256;
		}

		ASSERT(rem_bytes > 0 && rem_bytes <= 0x100);

		// Final page
		Ne2k_int_ReadDMA(Card, page, 1, data);
		memcpy(writepos, data, rem_bytes);
	}
	
	// Write BNRY (maximum page for incoming packets)
	if(pktHdr->NextPacketPage == RX_FIRST)
		outb( Card->IOBase + BNRY, RX_LAST-1 );
	else
		outb( Card->IOBase + BNRY, pktHdr->NextPacketPage-1 );
	// Set next RX Page and decrement the waiting list
	Card->NextRXPage = pktHdr->NextPacketPage;
	
	LEAVE('p', ret);
	return ret;
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
void Ne2k_IRQHandler(int IntNum, void *Ptr)
{
	Uint8	byte;
	tCard	*card = Ptr;

	if(card->IRQ != IntNum)	return;
	
	byte = inb( card->IOBase + ISR );
	
	LOG("byte = 0x%02x", byte);
			
			
	// Reset All (save for RDMA), that's polled
	outb( card->IOBase + ISR, 0xFF&(~0x40) );
			
	// 0: Packet recieved (no error)
	if( byte & 1 )
	{
		//if( card->NumWaitingPackets > MAX_PACKET_QUEUE )
		//	card->NumWaitingPackets = MAX_PACKET_QUEUE;
		if( Semaphore_Signal( &card->Semaphore, 1 ) != 1 ) {
			// Oops?
		}
	}
	// 1: Packet sent (no error)
	// 2: Recieved with error
	// 3: Transmission Halted (Excessive Collisions)
	// 4: Recieve Buffer Exhausted
	// 5: 
	// 6: Remote DMA Complete
	// 7: Reset
}

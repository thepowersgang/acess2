/*
 * Acess2 PCnet-FAST III Driver
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	1
#define VERSION	((1<<8)|0)
#include <acess.h>
#include <modules.h>
#include <drv_pci.h>
#include <semaphore.h>
#include <IPStack/include/adapters_api.h>
#include "hw.h"

// === CONSTANTS ===
#define VENDOR_ID	0x1022
#define DEVICE_ID	0x2000
#define TLEN_LOG2	6	// 64
#define RLEN_LOG2	7
#define	TLEN	(1 << TLEN_LOG2)
#define	RLEN	(1 << RLEN_LOG2)
#define	RXBUFLEN	128	// 128*128 = 16K total RX buffer
#define RXBUF_PER_PAGE	(PAGE_SIZE/RXBUFLEN)
#define NUM_RXBUF_PAGES	((RLEN*RXBUFLEN)/PAGE_SIZE)

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;

	tPAddr	TxQueuePhys;
	tTxDesc_3	*TxQueue;
	void	*TxQueueBuffers[TLEN];	// Pointer to the tIPStackBuffer (STP only)

	tMutex	lTxPos;
	tSemaphore	TxDescSem;
	 int	FirstUsedTxD;
	 int	FirstFreeTx;
	
	tSemaphore	ReadSemaphore;	
	tMutex	lRxPos;
	 int	RxPos;
	tPAddr	RxQueuePhys;
	tRxDesc_3	*RxQueue;
	void	*RxBuffers[NUM_RXBUF_PAGES];	// Pages
	
	Uint8	MacAddr[6];
	void	*IPStackHandle;
}	tCard;

// === PROTOTYPES ===
 int	PCnet3_Install(char **Options);
 int	PCnet3_Cleanup(void);

tIPStackBuffer	*PCnet3_WaitForPacket(void *Ptr);
 int	PCnet3_SendPacket(void *Ptr, tIPStackBuffer *Buffer);

 int	PCnet3_int_InitCard(tCard *Card);
void	PCnet3_IRQHandler(int Num, void *Ptr);
void	PCnet3_ReleaseRxD(void *Arg, size_t HeadLen, size_t FootLen, const void *Data);

static Uint16	_ReadCSR(tCard *Card, Uint8 Reg);
static void	_WriteCSR(tCard *Card, Uint8 Reg, Uint16 Value);
static Uint16	_ReadBCR(tCard *Card, Uint8 Reg);
static void	_WriteBCR(tCard *Card, Uint8 Reg, Uint16 Value);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Network_PCnetFAST3, PCnet3_Install, PCnet3_Cleanup, "IPStack", NULL);
tIPStack_AdapterType	gPCnet3_AdapterType = {
	.Name = "PCnet-FAST III",
	.Type = ADAPTERTYPE_ETHERNET_100M,
	//.Flags = ADAPTERFLAG_OFFLOAD_MAC,
	.Flags = 0,
	.SendPacket = PCnet3_SendPacket,
	.WaitForPacket = PCnet3_WaitForPacket
};
 int	giPCnet3_CardCount;
tCard	*gaPCnet3_Cards;
// - Init
tInitBlock32	gPCnet3_StaticInitBlock;
tInitBlock32	*gpPCnet3_InitBlock;

// === CODE ===
/**
 * \brief Installs the PCnet3 Driver
 */
int PCnet3_Install(char **Options)
{
	 int	id = -1;
	 int	i = 0;
	Uint16	base;
	tCard	*card;
	
	giPCnet3_CardCount = PCI_CountDevices(VENDOR_ID, DEVICE_ID);
	if( giPCnet3_CardCount == 0 )	return MODULE_ERR_NOTNEEDED;

	gpPCnet3_InitBlock = &gPCnet3_StaticInitBlock;
	// TODO: Edge case bug here with the block on the end of a page
	if( MM_GetPhysAddr(gpPCnet3_InitBlock) + sizeof(tInitBlock32) != MM_GetPhysAddr(gpPCnet3_InitBlock+1)
	#if PHYS_BITS > 32
		||  MM_GetPhysAddr(gpPCnet3_InitBlock) > (1ULL<<32)
	#endif
		)
	{
		// allocate
		Log_Error("PCnet3", "TODO: Support 64-bit init / spanning init");
		return MODULE_ERR_MISC;
	}
	
	gaPCnet3_Cards = calloc( giPCnet3_CardCount, sizeof(tCard) );
	
	while( (id = PCI_GetDevice(VENDOR_ID, DEVICE_ID, i)) != -1 )
	{
		// Set up card addresses
		// - BAR0: IO base address
		// - BAR1: MMIO base address
		card = &gaPCnet3_Cards[i];
		base = PCI_GetBAR( id, 0 );
		if( !(base & 1) ) {
			Log_Warning("PCnet3", "Driver does not support MMIO, skipping card");
			card->IOBase = 0;
			card->IRQ = 0;
			continue ;
		}
		base &= ~1;
		card->IOBase = base;
		card->IRQ = PCI_GetIRQ( id );

		// Switch the card into DWord mode
		// - TODO: Should the value of RAP matter here?
		outd(card->IOBase + REG_RDP, 0);

		// Get MAC address
		Uint32	macword;
		macword = ind(card->IOBase + REG_APROM0);
		card->MacAddr[0] = macword & 0xFF;
		card->MacAddr[1] = macword >> 8;
		card->MacAddr[2] = macword >> 16;
		card->MacAddr[3] = macword >> 24;
		macword = ind(card->IOBase + REG_APROM4);
		card->MacAddr[4] = macword & 0xFF;
		card->MacAddr[5] = macword >> 8;

		// Install IRQ Handler
		IRQ_AddHandler(card->IRQ, PCnet3_IRQHandler, card);
		
		// Initialise the card state
		PCnet3_int_InitCard(card);

		// Register
		card->IPStackHandle = IPStack_Adapter_Add(&gPCnet3_AdapterType, card, card->MacAddr);
		
		i ++;
	}

	if( gpPCnet3_InitBlock != &gPCnet3_StaticInitBlock )
	{
		MM_UnmapHWPages( gpPCnet3_InitBlock, 1 );
	}
	
	return MODULE_ERR_OK;
}

int PCnet3_Cleanup(void)
{
	// TODO: Kill IPStack adapters and clean up
	return -1;
}

// --- Root Functions ---
tIPStackBuffer *PCnet3_WaitForPacket(void *Ptr)
{
	tCard	*card = Ptr;

	ENTER("pPtr", Ptr);

	if( Semaphore_Wait( &card->ReadSemaphore, 1 ) != 1 )
	{
		LEAVE_RET('n', NULL);
	}

	// Get descriptor range for packet
	// TODO: Replace asserts with something a little more permissive	
	Mutex_Acquire( &card->lRxPos );
	 int	first_td = card->RxPos;
	 int	nextp_td = first_td;
	assert( card->RxQueue[first_td].Flags & RXDESC_FLG_STP );
	while( !(card->RxQueue[nextp_td].Flags & RXDESC_FLG_ENP) )
	{
		tRxDesc_3 *rd = &card->RxQueue[nextp_td];
		assert( !(rd->Flags & RXDESC_FLG_OWN) );
		// TODO: Check error bits properly
		if( rd->Flags & 0x7C000000 ) {
			Log_Notice("PCnet3", "Error bits set: 0x%x", (rd->Flags>>24) & 0x7C);
		}
		nextp_td = (nextp_td+1) % RLEN;
		assert(nextp_td != first_td);
	}
	nextp_td = (nextp_td+1) % RLEN;
	card->RxPos = nextp_td;
	Mutex_Release( &card->lRxPos );
	
	 int	nDesc = (nextp_td - first_td + RLEN) % RLEN;

	// Create buffer structure	
	// TODO: Could be more efficient by checking for buffers in the same page / fully contig allocations
	// - Meh
	tIPStackBuffer *ret = IPStack_Buffer_CreateBuffer(nDesc);
	for( int idx = first_td; idx != nextp_td; idx = (idx+1) % RLEN )
	{
		tRxDesc_3 *rd = &card->RxQueue[idx];
		void	*ptr = card->RxBuffers[idx/RXBUF_PER_PAGE] + (idx%RXBUF_PER_PAGE)*RXBUFLEN;
		IPStack_Buffer_AppendSubBuffer(ret, (rd->Count & 0xFFF), 0, ptr, PCnet3_ReleaseRxD, rd);
	}

	LEAVE('p', ret);
	return ret;
}

int PCnet3_int_FillTD(tTxDesc_3 *td, Uint32 BufAddr, Uint32 Len, int bBounced)
{
	td->Flags0 = 0;
	td->Flags1 = 0xF000 | (4096 - (Len & 0xFFF));
	td->Buffer = BufAddr;
	td->_avail = bBounced;
	return 0;
}

int PCnet3_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tCard	*card = Ptr;
	
	if( IPStack_Buffer_GetLength(Buffer) > 1500 ) {
		// MTU exceeded
		return EINVAL;
	}
	
	ENTER("pPtr pBuffer", Ptr, Buffer);
	// Need a sequence of `n` transmit descriptors
	// - Can assume that descriptors are consumed FIFO from the current descriptor point
	 int	idx = 0;
	 int	nDesc = 0;
	const void *sbuf_ptr;
	size_t	sbuf_len;
	while( (idx = IPStack_Buffer_GetBuffer(Buffer, idx, &sbuf_len, &sbuf_ptr)) != -1 )
	{
		nDesc ++;
		#if PHYS_BITS > 32
		if( MM_GetPhysAddr(sbuf_ptr) > (1ULL<<32) )
			;	// will be bounce-buffered
		else
		#endif
		if( MM_GetPhysAddr(sbuf_ptr)+sbuf_len-1 != MM_GetPhysAddr(sbuf_ptr+sbuf_len-1) )
		{
			// Split
			nDesc ++;
		}
	}

	// - Obtain enough descriptors
	int rv = Semaphore_Wait(&card->TxDescSem, nDesc);
	if( rv != nDesc ) {
		Log_Notice("PCnet3", "Semaphore wait interrupted, restoring %i descriptors");
		Semaphore_Signal(&card->TxDescSem, rv);
		LEAVE_RET('i', EINTR);
	}
	Mutex_Acquire(&card->lTxPos);
	int first_desc = card->FirstFreeTx;
	card->FirstFreeTx = (card->FirstFreeTx + nDesc) % TLEN;
	Mutex_Release(&card->lTxPos);
	
	// - Set up descriptors
	 int	td_idx = first_desc;
	while( (idx = IPStack_Buffer_GetBuffer(Buffer, idx, &sbuf_len, &sbuf_ptr)) != -1 )
	{
		tTxDesc_3 *td = &card->TxQueue[td_idx];
		assert( !(td->Flags1 & TXDESC_FLG1_OWN) );
		td_idx = (td_idx + 1) % TLEN;
		
		tPAddr	start_phys = MM_GetPhysAddr(sbuf_ptr);
		size_t	page1_maxsize = PAGE_SIZE - (start_phys % PAGE_SIZE);
		tPAddr	end_phys = MM_GetPhysAddr(sbuf_ptr + sbuf_len-1);

		#if PHYS_BITS > 32
		if( start_phys > (1ULL<<32) || end_phys > (1ULL<<32) )
		{
			// TODO: Have a global set of bounce buffers
			tPAddr bounce_phys;
			void *bounce_virt = MM_AllocDMA(1, 32, &bounce_phys);
			memcpy(bounce_virt, sbuf_ptr, sbuf_len);
			// Copy to bounce buffer
			PCnet3_int_FillTD(td, bounce_phys, sbuf_len, 1);
			LOG("%i: Bounce buffer %P+%i (orig %P,%P) - %p",
				idx, bounce_phys, sbuf_len, start_phys, end_phys, td);
		}
		else
		#endif
		if( start_phys+sbuf_len-1 != end_phys )
		{
			// Split buffer into two descriptors
			tTxDesc_3 *td2 = &card->TxQueue[td_idx];
			assert( !(td2->Flags1 & TXDESC_FLG1_OWN) );
			td_idx = (td_idx + 1) % TLEN;
			
			PCnet3_int_FillTD(td, start_phys, page1_maxsize, 0);
			
			size_t	page2_size = sbuf_len - page1_maxsize;
			PCnet3_int_FillTD(td2, end_phys - (page2_size-1), page2_size, 0);
			// - Explicitly set OWN on td2 because it's never the first, and `td` gets set below
			td2->Flags1 |= TXDESC_FLG1_OWN;
			
			LOG("%i: Split (%P,%P)+%i - %p,%p",
				idx, td->Buffer, td2->Buffer, sbuf_len, td, td2);
		}
		else
		{
			PCnet3_int_FillTD(td, start_phys, sbuf_len, 0);
			LOG("%i: Straight %P+%i - %p",
				idx, td->Buffer, sbuf_len, td);
		}
		// On every descriptor except the first, set OWN
		// - OWN set later once all are filled
		if( td != &card->TxQueue[first_desc] )
			td->Flags1 |= TXDESC_FLG1_OWN;
	}

	// - Lock buffer before allowing the card to continue
	IPStack_Buffer_LockBuffer(Buffer);
	
	// - Set STP/ENP
	card->TxQueue[first_desc].Flags1 |= TXDESC_FLG1_STP;
	card->TxQueue[(td_idx+TLEN-1)%TLEN].Flags1 |= TXDESC_FLG1_ENP|TXDESC_FLG1_ADDFCS;
	// - Set OWN on the first descriptor
	card->TxQueue[first_desc].Flags1 |= TXDESC_FLG1_OWN;
	card->TxQueueBuffers[first_desc] = Buffer;

	LOG("CSR0=0x%x", _ReadCSR(card, 0));
	LOG("Transmit started, waiting for completion");
	
	// Block here until packet is sent
	// TODO: Should be able to return, but just in case
	IPStack_Buffer_LockBuffer(Buffer);
	IPStack_Buffer_UnlockBuffer(Buffer);
	
	LEAVE('i', 0);
	return 0;
}

int PCnet3_int_InitCard(tCard *Card)
{
	// Allocate ring buffers
	Card->TxQueue = (void*)MM_AllocDMA(1, 32, &Card->TxQueuePhys);
	if( !Card->TxQueue ) {
		return MODULE_ERR_MALLOC;
	}
	memset(Card->TxQueue, 0, TLEN*sizeof(*Card->TxQueue));
	#if TLEN + RLEN <= PAGE_SIZE / (4*4)
	Card->RxQueue = (void*)MM_AllocDMA(1, 32, &Card->RxQueuePhys);
	if( !Card->RxQueue ) {
		return MODULE_ERR_MALLOC;
	}
	#else
	Card->RxQueue = Card->TxQueue + TLEN;
	Card->RxQueuePhys = Card->RxQueuePhys + TLEN*sizeof(*Card->TxQueue);
	#endif

	// Allocate Rx buffers
	for( int i = 0; i < NUM_RXBUF_PAGES; i ++ )
	{
		tPAddr	physaddr;
		Card->RxBuffers[i] = (void*)MM_AllocDMA(1, 32, &physaddr);
		if( !Card->RxBuffers[i] ) {
			return MODULE_ERR_MALLOC;
		}
		for( int j = 0; j < RXBUF_PER_PAGE; j ++ )
		{
			Card->RxQueue[i*RXBUF_PER_PAGE+j].Buffer = physaddr;
			physaddr += RXBUFLEN;
		}
	}
	
	// Initialise semaphores
	Semaphore_Init(&Card->TxDescSem, TLEN, TLEN, "PCnet3", "Tx Descriptors");
	Semaphore_Init(&Card->ReadSemaphore, 0, RLEN, "PCnet3", "Rx Descriptors");
	
	// Fill Init Block for this card
	gpPCnet3_InitBlock->Mode = (TLEN_LOG2 << 28) | (RLEN_LOG2 << 20);
	gpPCnet3_InitBlock->PhysAddr1 = 0;
	memcpy(&gpPCnet3_InitBlock->PhysAddr0, Card->MacAddr, 6);
	gpPCnet3_InitBlock->LAdrF0 = -1;	// TODO: Allow these to be set by the IPStack
	gpPCnet3_InitBlock->LAdrF1 = -1;
	gpPCnet3_InitBlock->RDRA = Card->RxQueuePhys;
	gpPCnet3_InitBlock->TDRA = Card->TxQueuePhys;

	// Reset card
	inw(Card->IOBase + REG_RESET);
	_WriteBCR(Card, BCR_SWSTYLE, (1<<8)|3);	// Set SSIZE32
	LOG("BCR_SWSTYLE reads as 0x%x", _ReadBCR(Card, BCR_SWSTYLE));
	LOG("CSR4 reads as 0x%x", _ReadCSR(Card, 4));

	// Initialise
	tPAddr	paddr = MM_GetPhysAddr(gpPCnet3_InitBlock);
	_WriteCSR(Card, CSR_IBA0, paddr & 0xFFFF);
	_WriteCSR(Card, CSR_IBA1, paddr >> 16);
	_WriteCSR(Card, CSR_STATUS, CSR_STATUS_INIT|CSR_STATUS_IENA|CSR_STATUS_STRT);

	return 0;
}

void PCnet3_IRQHandler(int Num, void *Ptr)
{
	tCard	*card = Ptr;
	Uint16	status = _ReadCSR(card, CSR_STATUS);

	LOG("status = 0x%02x", status);
	status &= ~CSR_STATUS_INTR;	// Read-only bit
	
	// Rx Interrupt
	// META - Check LAPPEN bit in CSR3
	if( status & CSR_STATUS_RINT )
	{
		// TODO: Avoid issues when two packets arrive in one interrupt time
		Semaphore_Signal(&card->ReadSemaphore, 1);
	}
	
	// Tx Interrupt
	if( status & CSR_STATUS_TINT )
	{
		 int	idx;
		for( idx = card->FirstUsedTxD; idx != card->FirstFreeTx; idx = (idx+1)%TLEN )
		{
			tTxDesc_3 *td = &card->TxQueue[idx];
			// Stop on the first chip-owned TxD
			LOG("idx=%i, Flags1=0x%08x", idx, td->Flags1);
			if( td->Flags1 & TXDESC_FLG1_OWN )
				break;
			if( td->Flags1 & (1<<30) )
			{
				LOG(" Flags0=0x%08x %s%s%s%s%s%s%i",
					td->Flags0,
					(td->Flags0 & (1<<31)) ? "BUFF " : "",
					(td->Flags0 & (1<<30)) ? "UFLO " : "",
					(td->Flags0 & (1<<29)) ? "EXDEF " : "",
					(td->Flags0 & (1<<28)) ? "LCOL " : "",
					(td->Flags0 & (1<<27)) ? "LCAR " : "",
					(td->Flags0 & (1<<26)) ? "RTRY " : "",
					td->Flags0 & 15
					);
			}
			if( td->Flags1 & TXDESC_FLG1_STP )
				IPStack_Buffer_UnlockBuffer( card->TxQueueBuffers[idx] );
			Semaphore_Signal(&card->TxDescSem, 1);
		}
		card->FirstUsedTxD = idx;
	}

	if( status & CSR_STATUS_IDON )
	{
		Log_Debug("PCnet3", "Card %p initialisation done", card);
		LOG("CSR15 reads as 0x%x", _ReadCSR(card, 15));
	}

	// ERR set?
	if( status & 0xBC00 )
	{
		Log_Notice("PCnet3", "Error on %p: %s%s%s%s",
			card,
			(status & (1<<15)) ? "ERR " : "",
			(status & (1<<13)) ? "CERR " : "",
			(status & (1<<12)) ? "MISS " : "",
			(status & (1<<11)) ? "MERR " : ""
			);
	}

	_WriteCSR(card, CSR_STATUS, status);
}

void PCnet3_ReleaseRxD(void *Arg, size_t HeadLen, size_t FootLen, const void *Data)
{
	tRxDesc_3	*rd = Arg;
	rd->Flags &= 0xFFFF;
	rd->Flags |= RXDESC_FLG_OWN;
}

static Uint16 _ReadCSR(tCard *Card, Uint8 Reg)
{
	outd(Card->IOBase + REG_RAP, Reg);
	return ind(Card->IOBase + REG_RDP);
}
static void _WriteCSR(tCard *Card, Uint8 Reg, Uint16 Value)
{
	outd(Card->IOBase + REG_RAP, Reg);
	outd(Card->IOBase + REG_RDP, Value);
}
static Uint16 _ReadBCR(tCard *Card, Uint8 Reg)
{
	outd(Card->IOBase + REG_RAP, Reg);
	return ind(Card->IOBase + REG_BDP);
}
void _WriteBCR(tCard *Card, Uint8 Reg, Uint16 Value)
{
	outd(Card->IOBase + REG_RAP, Reg);
	outd(Card->IOBase + REG_BDP, Value);
}


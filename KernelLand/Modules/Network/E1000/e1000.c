/*
 * Acess2 E1000 Network Driver
 * - By John Hodge (thePowersGang)
 *
 * e1000.c
 * - Intel 8254x Network Card Driver (core)
 */
#define DEBUG	0
#define	VERSION	VER2(0,1)
#include <acess.h>
#include "e1000.h"
#include <modules.h>
#include <drv_pci.h>
#include <IPStack/include/adapters_api.h>
#include <timers.h>	// Time_Delay

const struct sSupportedCard {
	Uint16	Vendor, Device;
} caSupportedCards[] = {
	{0x8086, 0x100E},	// 82540EM-A Desktop
	{0x8086, 0x1010},	// 82546EB-A1 Copper Dual Port
	{0x8086, 0x1012},	// 82546EB-A1 Fiber
	{0x8086, 0x1019},	// 82547[EG]I Copper
	{0x8086, 0x101A},	// 82547EI Mobile
	{0x8086, 0x101D},	// 82546EB-A1 Copper Quad Port
};
const int ciNumSupportedCards = sizeof(caSupportedCards)/sizeof(caSupportedCards[0]);

// === PROTOTYPES ===
 int	E1000_Install(char **Arguments);
 int	E1000_Cleanup(void);
tIPStackBuffer	*E1000_WaitForPacket(void *Ptr);
 int	E1000_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	E1000_IRQHandler(int Num, void *Ptr);
 int	E1000_int_InitialiseCard(tCard *Card);
Uint16	E1000_int_ReadEEPROM(tCard *Card, Uint8 WordIdx);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, E1000, E1000_Install, E1000_Cleanup, NULL);
tIPStack_AdapterType	gE1000_AdapterType = {
	.Name = "E1000",
	.Type = ADAPTERTYPE_ETHERNET_1G,	// TODO: Differentiate differnet wire protos and speeds
	.Flags = ADAPTERFLAG_OFFLOAD_MAC,	// TODO: IP/TCP/UDP checksum offloading
	.SendPacket = E1000_SendPacket,
	.WaitForPacket = E1000_WaitForPacket
	};
tCard	*gaE1000_Cards;

// === CODE ===
int E1000_Install(char **Arguments)
{
	 int	card_count = 0;
	for( int modelidx = 0; modelidx < ciNumSupportedCards; modelidx ++ )
	{
		const struct sSupportedCard	*cardtype = &caSupportedCards[modelidx];
		card_count += PCI_CountDevices(cardtype->Vendor, cardtype->Device);
	}
	LOG("card_count = %i", card_count);
	if( card_count == 0 ) {
		LOG("Zero cards located");
		return MODULE_ERR_NOTNEEDED;
	}

	// Allocate card array
	gaE1000_Cards = calloc(sizeof(tCard), card_count);
	if( !gaE1000_Cards ) {
		Log_Warning("E1000", "Allocation of %i card structures failed", card_count);
		return MODULE_ERR_MALLOC;
	}	

	// Initialise cards
	int card_idx = 0;
	for( int modelidx = 0; modelidx < ciNumSupportedCards; modelidx ++ )
	{
		const struct sSupportedCard	*cardtype = &caSupportedCards[modelidx];
		for( int id = -1, i = 0; (id = PCI_GetDevice(cardtype->Vendor, cardtype->Device, i)) != -1; i ++ )
		{
			tCard	*card = &gaE1000_Cards[card_idx++];
			card->MMIOBasePhys = PCI_GetValidBAR(id, 0, PCI_BARTYPE_MEMNP);
			if( !card->MMIOBasePhys ) {
				Log_Warning("E1000", "Dev %i: BAR0 should be non-prefetchable memory", id);
				continue;
			}

			card->IRQ = PCI_GetIRQ(id);
			IRQ_AddHandler(card->IRQ, E1000_IRQHandler, card);
			PCI_SetCommand(id, PCI_CMD_MEMENABLE|PCI_CMD_BUSMASTER, 0);
		
			Log_Debug("E1000", "Card %i: %P IRQ %i", card_idx, card->MMIOBasePhys, card->IRQ);

			if( E1000_int_InitialiseCard(card) ) {
				Log_Warning("E1000", "Initialisation of card #%i failed", card_idx);
				return MODULE_ERR_MALLOC;
			}
			
			card->IPStackHandle = IPStack_Adapter_Add(&gE1000_AdapterType, card, card->MacAddr);
		}
	}
	return MODULE_ERR_OK;
}

int E1000_Cleanup(void)
{
	return 0;
}

void E1000_int_ReleaseRXD(void *Arg, size_t HeadLen, size_t FootLen, const void *Data)
{
	tCard	**cardptr = Arg;
	tCard	*Card = *cardptr;
	 int	rxd = (Arg - (void*)Card->RXBackHandles) / sizeof(void*);

	LOG("RXD %p %i being released", Card, rxd);
	ASSERT(rxd >= 0 && rxd < NUM_RX_DESC);

	Card->RXDescs[rxd].Status = 0;
	Mutex_Acquire(&Card->lRXDescs);
	if( rxd == REG32(Card, REG_RDT) ) {
		while( rxd != Card->FirstUnseenRXD && !(Card->RXDescs[rxd].Status & RXD_STS_DD) ) {
			rxd ++;
			if( rxd == NUM_RX_DESC )
				rxd = 0;
		}
		REG32(Card, REG_RDT) = rxd;
		LOG("Updated RDT=%i", rxd);
	}
	Mutex_Release(&Card->lRXDescs);
}

tIPStackBuffer *E1000_WaitForPacket(void *Ptr)
{
	tCard	*Card = Ptr;
	
	if( Semaphore_Wait(&Card->AvailPackets, 1) != 1 )
		return NULL;
	
	ENTER("pPtr", Ptr);

	Mutex_Acquire(&Card->lRXDescs);
	 int	first_rxd = Card->FirstUnseenRXD;
	 int	last_rxd = first_rxd;
	 int	nDesc = 1;
	while( last_rxd != Card->LastUnseenRXD  ) {
		if( !(Card->RXDescs[last_rxd].Status & RXD_STS_DD) )
			break;	// Oops, should ahve found an EOP first
		if( Card->RXDescs[last_rxd].Status & RXD_STS_EOP )
			break;
		nDesc ++;
		last_rxd = (last_rxd + 1) % NUM_RX_DESC;
	}
	Card->FirstUnseenRXD = (last_rxd + 1) % NUM_RX_DESC;
	Mutex_Release(&Card->lRXDescs);

	LOG("nDesc = %i, first_rxd = %i", nDesc, first_rxd);
	tIPStackBuffer *ret = IPStack_Buffer_CreateBuffer(nDesc);
	 int	rxd = first_rxd;
	for( int i = 0; i < nDesc; i ++ )
	{
		IPStack_Buffer_AppendSubBuffer(ret, 0, Card->RXDescs[rxd].Length, Card->RXBuffers[rxd],
			E1000_int_ReleaseRXD, &Card->RXBackHandles[rxd]);
	}

	LEAVE('p', ret);
	return ret;
}

int E1000_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tCard	*Card = Ptr;

	ENTER("pPtr pBuffer", Ptr, Buffer);

	 int	nDesc = 0;
	size_t	len;
	const void	*ptr;
	// Count sub-buffers (including splitting cross-page entries)
	 int	idx = -1;
	while( (idx = IPStack_Buffer_GetBuffer(Buffer, idx, &len, &ptr)) != -1 )
	{
		if( len > PAGE_SIZE ) {
			LOG("len=%i > PAGE_SIZE", len);
			LEAVE('i', EINVAL);
			return EINVAL;
		}
		if( MM_GetPhysAddr(ptr) + len-1 != MM_GetPhysAddr((char*)ptr + len-1) ) {
			LOG("Buffer %p+%i spans non-contig physical pages", ptr, len);
			nDesc ++;
		}
		nDesc ++;
	}
	
	// Request set of TX descriptors
	int rv = Semaphore_Wait(&Card->FreeTxDescs, nDesc);
	if(rv != nDesc) {
		LEAVE('i', EINTR);
		return EINTR;
	}
	Mutex_Acquire(&Card->lTXDescs);
	 int	first_txd = Card->FirstFreeTXD;
	Card->FirstFreeTXD = (first_txd + nDesc) % NUM_TX_DESC;
	 int	last_txd = (first_txd + nDesc-1) % NUM_TX_DESC;

	LOG("first_txd = %i, last_txd = %i", first_txd, last_txd);

	// Populate buffers
	idx = -1;
	 int txd = first_txd;
	while( (idx = IPStack_Buffer_GetBuffer(Buffer, idx, &len, &ptr)) != -1 )
	{
		//Debug_HexDump("E100 SendPacket", ptr, len);
		if( MM_GetPhysAddr(ptr) + len-1 != MM_GetPhysAddr((char*)ptr + len-1) )
		{
			size_t	remlen = PAGE_SIZE - ((tVAddr)ptr & (PAGE_SIZE-1));
			// Split in two
			// - First Page
			Card->TXDescs[txd].Buffer = MM_GetPhysAddr(ptr);
			Card->TXDescs[txd].Length = remlen;
			Card->TXDescs[txd].CMD = TXD_CMD_RS;
			txd = (txd + 1) % NUM_TX_DESC;
			// - Second page
			Card->TXDescs[txd].Buffer = MM_GetPhysAddr((char*)ptr + remlen);
			Card->TXDescs[txd].Length = len - remlen;
			Card->TXDescs[txd].CMD = TXD_CMD_RS;
		}
		else
		{
			// Single
			volatile tTXDesc *txdp = &Card->TXDescs[txd];
			txdp->Buffer = MM_GetPhysAddr(ptr);
			txdp->Length = len;
			txdp->CMD = TXD_CMD_RS;
			LOG("%P: %llx %x %x", MM_GetPhysAddr((void*)txdp), txdp->Buffer, txdp->Length, txdp->CMD);
		}
		txd = (txd + 1) % NUM_TX_DESC;
	}
	Card->TXDescs[last_txd].CMD |= TXD_CMD_EOP|TXD_CMD_IDE|TXD_CMD_IFCS;
	Card->TXSrcBuffers[last_txd] = Buffer;

	__sync_synchronize();
	#if DEBUG
	{
		volatile tTXDesc *txdp = Card->TXDescs + last_txd;
		LOG("%p %P: %llx %x %x", txdp, MM_GetPhysAddr((void*)txdp), txdp->Buffer, txdp->Length, txdp->CMD);
		volatile tTXDesc *txdp_base = MM_MapTemp(MM_GetPhysAddr((void*)Card->TXDescs));
		txdp = txdp_base + last_txd;
		LOG("%p %P: %llx %x %x", txdp, MM_GetPhysAddr((void*)txdp), txdp->Buffer, txdp->Length, txdp->CMD);
		MM_FreeTemp( (void*)txdp_base);
	}
	#endif
	// Trigger TX
	IPStack_Buffer_LockBuffer(Buffer);
	LOG("Triggering TX - Buffers[%i]=%p", last_txd, Buffer);
	REG32(Card, REG_TDT) = Card->FirstFreeTXD;
	Mutex_Release(&Card->lTXDescs);
	LOG("Waiting for TX to complete");
	
	// Wait for completion (lock will block, then release straight away)
	IPStack_Buffer_LockBuffer(Buffer);
	IPStack_Buffer_UnlockBuffer(Buffer);

	// TODO: Check status bits

	LEAVE('i', 0);
	return 0;
}

void E1000_IRQHandler(int Num, void *Ptr)
{
	tCard	*Card = Ptr;
	
	Uint32	icr = REG32(Card, REG_ICR);
	if( icr == 0 )
		return ;
	LOG("icr = %x", icr);

	// Transmit descriptor written
	if( (icr & ICR_TXDW) || (icr & ICR_TXQE) )
	{
		 int	nReleased = 0;
		 int	txd = Card->LastFreeTXD;
		 int	nReleasedAtLastDD = 0;
		 int	idxOfLastDD = txd;
		// Walk descriptors looking for the first non-complete descriptor
		LOG("TX %i:%i", Card->LastFreeTXD, Card->FirstFreeTXD);
		while( txd != Card->FirstFreeTXD )
		{
			nReleased ++;
			if(Card->TXDescs[txd].Status & TXD_STS_DD) {
				nReleasedAtLastDD = nReleased;
				idxOfLastDD = txd;
			}
			txd ++;
			if(txd == NUM_TX_DESC)
				txd = 0;
		}
		if( nReleasedAtLastDD )
		{
			// Unlock buffers
			txd = Card->LastFreeTXD;
			LOG("TX unlocking range %i-%i", txd, idxOfLastDD);
			while( txd != (idxOfLastDD+1)%NUM_TX_DESC )
			{
				if( Card->TXSrcBuffers[txd] ) {
					LOG("- Unlocking %i:%p", txd, Card->TXSrcBuffers[txd]);
					IPStack_Buffer_UnlockBuffer( Card->TXSrcBuffers[txd] );
					Card->TXSrcBuffers[txd] = NULL;
				}
				txd ++;
				if(txd == NUM_TX_DESC)
					txd = 0;
			}
			// Update last free
			Card->LastFreeTXD = txd;
			Semaphore_Signal(&Card->FreeTxDescs, nReleasedAtLastDD);
			LOG("nReleased = %i", nReleasedAtLastDD);
		}
		else
		{
			LOG("No completed TXDs");
		}
	}	

	if( icr & ICR_LSC )
	{
		// Link status change
		LOG("LSC");
		// TODO: Detect link drop/raise and poke IPStack
	}

	if( icr & ICR_RXO )
	{
		LOG("RX Overrun");
	}
	
	// Pending packet (s)
	if( icr & ICR_RXT0 )
	{
		 int	nPackets = 0;
		LOG("RX %i:%i", Card->LastUnseenRXD, Card->FirstUnseenRXD);
		while( (Card->RXDescs[Card->LastUnseenRXD].Status & RXD_STS_DD) )
		{
			if( Card->RXDescs[Card->LastUnseenRXD].Status & RXD_STS_EOP )
				nPackets ++;
			Card->LastUnseenRXD ++;
			if( Card->LastUnseenRXD == NUM_RX_DESC )
				Card->LastUnseenRXD = 0;
			
			if( Card->LastUnseenRXD == Card->FirstUnseenRXD )
				break;
		}
		Semaphore_Signal(&Card->AvailPackets, nPackets);
		LOG("nPackets = %i", nPackets);
	}
	
	// Transmit Descriptor Low Threshold hit
	if( icr & ICR_TXD_LOW )
	{
		
	}

	// Receive Descriptor Minimum Threshold Reached
	// - We're reading too slow
	if( icr & ICR_RXDMT0 )
	{
		LOG("RX descs running out");
	}
	
	icr &= ~(ICR_RXT0|ICR_LSC|ICR_TXQE|ICR_TXDW|ICR_TXD_LOW|ICR_RXDMT0);
	if( icr )
		Log_Warning("E1000", "Unhandled ICR bits 0x%x", icr);
}

// TODO: Move this function into Kernel/drvutil.c
/**
 * \brief Allocate a set of buffers in hardware mapped space
 * 
 * Allocates \a NumBufs buffers using shared pages (if \a BufSize is less than a page) or
 * as a set of contiugious allocations.
 */
int DrvUtil_AllocBuffers(void **Buffers, int NumBufs, int PhysBits, size_t BufSize)
{
	if( BufSize >= PAGE_SIZE )
	{
		const int	pages_per_buf = BufSize / PAGE_SIZE;
		ASSERT(pages_per_buf * PAGE_SIZE == BufSize);
		for( int i = 0; i < NumBufs; i ++ ) {
			Buffers[i] = (void*)MM_AllocDMA(pages_per_buf, PhysBits, NULL);
			if( !Buffers[i] )	return 1;
		}
	}
	else
	{
		size_t	ofs = 0;
		const int 	bufs_per_page = PAGE_SIZE / BufSize;
		ASSERT(bufs_per_page * BufSize == PAGE_SIZE);
		void	*page = NULL;
		for( int i = 0; i < NumBufs; i ++ )
		{
			if( ofs == 0 ) {
				page = (void*)MM_AllocDMA(1, PhysBits, NULL);
				if( !page )	return 1;
			}
			Buffers[i] = (char*)page + ofs;
			ofs += BufSize;
			if( ofs >= PAGE_SIZE )
				ofs = 0;
		}
	}
	return 0;
}

int E1000_int_InitialiseCard(tCard *Card)
{
	ENTER("pCard", Card);
	
	// Map required structures
	Card->MMIOBase = (void*)MM_MapHWPages( Card->MMIOBasePhys, 7 );
	if( !Card->MMIOBase ) {
		Log_Error("E1000", "%p: Failed to map MMIO Space (7 pages)", Card);
		LEAVE('i', 1);
		return 1;
	}

	// --- Read MAC address from EEPROM ---
	{
		Uint16	macword;
		macword = E1000_int_ReadEEPROM(Card, 0);
		Card->MacAddr[0] = macword & 0xFF;
		Card->MacAddr[1] = macword >> 8;
		macword = E1000_int_ReadEEPROM(Card, 1);
		Card->MacAddr[2] = macword & 0xFF;
		Card->MacAddr[3] = macword >> 8;
		macword = E1000_int_ReadEEPROM(Card, 2);
		Card->MacAddr[4] = macword & 0xFF;
		Card->MacAddr[5] = macword >> 8;
	}
	Log_Log("E1000", "%p: MAC Address %02x:%02x:%02x:%02x:%02x:%02x",
		Card,
		Card->MacAddr[0], Card->MacAddr[1],
		Card->MacAddr[2], Card->MacAddr[3],
		Card->MacAddr[4], Card->MacAddr[5]);
	
	// --- Prepare for RX ---
	LOG("RX Preparation");
	Card->RXDescs = (void*)MM_AllocDMA(1, 64, NULL);
	if( !Card->RXDescs ) {
		LEAVE('i', 2);
		return 2;
	}
	if( DrvUtil_AllocBuffers(Card->RXBuffers, NUM_RX_DESC, 64, RX_DESC_BSIZE) ) {
		LEAVE('i', 3);
		return 3;
	}
	for( int i = 0; i < NUM_RX_DESC; i ++ )
	{
		Card->RXDescs[i].Buffer = MM_GetPhysAddr(Card->RXBuffers[i]);
		Card->RXDescs[i].Status = 0;	// Clear RXD_STS_DD, gives it to the card
		Card->RXBackHandles[i] = Card;
	}
	
	REG64(Card, REG_RDBAL) = MM_GetPhysAddr((void*)Card->RXDescs);
	REG32(Card, REG_RDLEN) = NUM_RX_DESC * 16;
	REG32(Card, REG_RDH) = 0;
	REG32(Card, REG_RDT) = NUM_RX_DESC;
	// Hardware size, Multicast promisc, Accept broadcast, Interrupt at 1/4 Rx descs free
	REG32(Card, REG_RCTL) = RX_DESC_BSIZEHW | RCTL_MPE | RCTL_BAM | RCTL_RDMTS_1_4;
	Card->FirstUnseenRXD = 0;
	Card->LastUnseenRXD = 0;

	// --- Prepare for TX ---
	LOG("TX Preparation");
	Card->TXDescs = (void*)MM_AllocDMA(1, 64, NULL);
	if( !Card->RXDescs ) {
		LEAVE('i', 4);
		return 4;
	}
	LOG("Card->RXDescs = %p [%P]", Card->TXDescs, MM_GetPhysAddr((void*)Card->TXDescs));
	for( int i = 0; i < NUM_TX_DESC; i ++ )
	{
		Card->TXDescs[i].Buffer = 0;
		Card->TXDescs[i].CMD = 0;
	}
	REG64(Card, REG_TDBAL) = MM_GetPhysAddr((void*)Card->TXDescs);
	REG32(Card, REG_TDLEN) = NUM_TX_DESC * 16;
	REG32(Card, REG_TDH) = 0;
	REG32(Card, REG_TDT) = 0;
	// Enable, pad short packets
	REG32(Card, REG_TCTL) = TCTL_EN | TCTL_PSP | (0x0F << TCTL_CT_ofs) | (0x40 << TCTL_COLD_ofs);
	Card->FirstFreeTXD = 0;

	// -- Prepare Semaphores
	Semaphore_Init(&Card->FreeTxDescs, NUM_TX_DESC, NUM_TX_DESC, "E1000", "TXDescs");
	Semaphore_Init(&Card->AvailPackets, 0, NUM_RX_DESC, "E1000", "RXDescs");

	// --- Prepare for full operation ---
	LOG("Starting card");
	REG32(Card, REG_CTRL) = CTRL_SLU|CTRL_ASDE;	// Link up, auto speed detection
	REG32(Card, REG_IMS) = 0x1F6DC;	// Interrupt mask
	(void)REG32(Card, REG_ICR);	// Discard pending interrupts
	REG32(Card, REG_RCTL) |= RCTL_EN;
	LEAVE('i', 0);
	return 0;
}

Uint16 E1000_int_ReadEEPROM(tCard *Card, Uint8 WordIdx)
{
	REG32(Card, REG_EERD) = ((Uint32)WordIdx << 8) | 1;
	Uint32	tmp;
	while( !((tmp = REG32(Card, REG_EERD)) & (1 << 4)) ) {
		// TODO: use something like Time_MicroDelay instead
		Time_Delay(1);
	}
	
	return tmp >> 16;
}

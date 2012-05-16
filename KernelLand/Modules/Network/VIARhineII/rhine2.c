/*
 * Acess2 VIA Rhine II Driver (VT6102)
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	1
#define VERSION	((0<<8)|10)
#include <acess.h>
#include <modules.h>
#include <drv_pci.h>
#include <semaphore.h>
#include "rhine2_hw.h"
#include <IPStack/include/adapters_api.h>

#define DESC_SIZE	16
#define N_RX_DESCS	16
#define RX_BUF_SIZE	1024
#define N_RX_PAGES	((N_RX_DESCS*RX_BUF_SIZE)/PAGE_SIZE)
#define N_TX_DESCS	((PAGE_SIZE/DESC_SIZE)-N_RX_DESCS)

#define CR0_BASEVAL	(CR0_STRT|CR0_TXON|CR0_RXON)

// === CONSTANTS ===
#define VENDOR_ID	0x1106
#define DEVICE_ID	0x3065

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;
	
	tSemaphore	ReadSemaphore;
	tSemaphore	SendSemaphore;

	struct {
		Uint32	Phys;
		void	*Virt;
	} RXBuffers[N_RX_PAGES];

	Uint32	DescTablePhys;
	void	*DescTable;

	struct sTXDesc	*TXDescs;
	 int	NextTX;
	 int	nFreeTX;
	
	struct sRXDesc	*FirstRX;	// Most recent unread packet
	struct sRXDesc	*LastRX;	// End of RX descriptor queue

	void	*IPHandle;	
	Uint8	MacAddr[6];
}	tCard;

// === PROTOTYPES ===
 int	Rhine2_Install(char **Options);
void	Rhine2_int_InitialiseCard(tCard *Card);
tIPStackBuffer	*Rhine2_WaitPacket(void *Ptr);
 int	Rhine2_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	Rhine2_IRQHandler(int Num, void *Pt);
// --- Helpers ---
struct sRXDesc	*Rhine2_int_GetDescFromPhys(tCard *Card, Uint32 Addr);
void	*Rhine2_int_GetBufferFromPhys(tCard *Card, Uint32 Addr);
void	Rhine2_int_FreeRXDesc(void *Desc, size_t, size_t, const void*);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VIARhineII, Rhine2_Install, NULL, NULL);
tIPStack_AdapterType	gRhine2_AdapterType = {
	.Name = "VIA Rhine II",
	.SendPacket = Rhine2_SendPacket,
	.WaitForPacket = Rhine2_WaitPacket,
};
 int	giRhine2_CardCount;
tCard	*gaRhine2_Cards;

// === CODE ===
/**
 * \brief Initialises the driver
 */
int Rhine2_Install(char **Options)
{
	 int	id = -1;
	 int	i = 0;
//	Uint16	base;
	tCard	*card;
	
	giRhine2_CardCount = PCI_CountDevices(VENDOR_ID, DEVICE_ID);
	if( giRhine2_CardCount == 0 )	return MODULE_ERR_NOTNEEDED;
	
	gaRhine2_Cards = calloc( giRhine2_CardCount, sizeof(tCard) );
	
	while( (id = PCI_GetDevice(VENDOR_ID, DEVICE_ID, i)) != -1 )
	{
		card = &gaRhine2_Cards[i];
		
		LOG("BAR0 = 0x%08x", PCI_GetBAR(id, 0));
		LOG("BAR1 = 0x%08x", PCI_GetBAR(id, 1));
		LOG("BAR2 = 0x%08x", PCI_GetBAR(id, 2));
		LOG("BAR3 = 0x%08x", PCI_GetBAR(id, 3));
		LOG("BAR4 = 0x%08x", PCI_GetBAR(id, 4));
		LOG("BAR5 = 0x%08x", PCI_GetBAR(id, 5));
		
		card->IOBase = PCI_GetBAR(id, 0);
		if( !(card->IOBase & 1) ) {
			// Oops?
			Log_Warning("Rhine2", "BAR0 is not in IO space");
			continue ;
		}
		card->IOBase &= ~1;
		card->IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
		IRQ_AddHandler(card->IRQ, Rhine2_IRQHandler, card);
		
		Rhine2_int_InitialiseCard(card);
	
		card->IPHandle = IPStack_Adapter_Add(&gRhine2_AdapterType, card, card->MacAddr);
	
		Log_Log("Rhine2", "Card %i 0x%04x, IRQ %i %02x:%02x:%02x:%02x:%02x:%02x",
			i, card->IOBase, card->IRQ,
			card->MacAddr[0], card->MacAddr[1], card->MacAddr[2],
			card->MacAddr[3], card->MacAddr[4], card->MacAddr[5]
			);
		
		i ++;
	}
	
	return MODULE_ERR_OK;
}

void Rhine2_int_InitialiseCard(tCard *Card)
{
	tPAddr	phys;
	
	Card->MacAddr[0] = inb(Card->IOBase + REG_PAR0);
	Card->MacAddr[1] = inb(Card->IOBase + REG_PAR1);
	Card->MacAddr[2] = inb(Card->IOBase + REG_PAR2);
	Card->MacAddr[3] = inb(Card->IOBase + REG_PAR3);
	Card->MacAddr[4] = inb(Card->IOBase + REG_PAR4);
	Card->MacAddr[5] = inb(Card->IOBase + REG_PAR5);

	LOG("Resetting card");	
	outb(Card->IOBase + REG_CR1, CR1_SFRST);
	// TODO: Timeout
	while( inb(Card->IOBase + REG_CR1) & CR1_SFRST ) ;
	
	LOG("Allocaating RX buffers");
	// Allocate memory for things
	for( int i = 0; i < N_RX_PAGES; i ++ )
	{
		Card->RXBuffers[i].Virt = (void*)MM_AllocDMA(1, 32, &phys);
		Card->RXBuffers[i].Phys = phys;
	}
	
	LOG("Allocating and filling RX/TX Descriptors");
	Card->DescTable = (void*)MM_AllocDMA(1, 32, &phys);
	Card->DescTablePhys = phys;

	// Initialise RX Descriptors
	struct sRXDesc	*rxdescs = Card->DescTable;
	for( int i = 0; i < N_RX_DESCS; i ++ )
	{
		rxdescs[i].RSR = 0;
		rxdescs[i].BufferSize = RX_BUF_SIZE;
		rxdescs[i].RXBufferStart = Card->RXBuffers[i/(PAGE_SIZE/RX_BUF_SIZE)].Phys
			+ (i % (PAGE_SIZE/RX_BUF_SIZE)) * RX_BUF_SIZE;
		rxdescs[i].RDBranchAddress = Card->DescTablePhys + (i+1) * DESC_SIZE;
		rxdescs[i].Length = (1 << 15);	// set OWN
	}
	rxdescs[ N_RX_DESCS - 1 ].RDBranchAddress = Card->DescTablePhys;
	Card->FirstRX = &rxdescs[0];
	Card->LastRX = &rxdescs[N_RX_DESCS - 1];

	Card->TXDescs = (void*)(rxdescs + N_RX_DESCS);
	memset(Card->TXDescs, 0, sizeof(struct sTXDesc)*N_TX_DESCS);
	for( int i = 0; i < N_TX_DESCS; i ++ )
	{
		Card->TXDescs[i].TDBranchAddress = Card->DescTablePhys + (N_RX_DESCS + i + 1) * DESC_SIZE;
	}
	Card->TXDescs[N_TX_DESCS-1].TDBranchAddress = Card->DescTablePhys + N_RX_DESCS * DESC_SIZE;
	Card->nFreeTX = N_TX_DESCS;
	Card->NextTX = 0;
	
	// - Initialise card state
	LOG("Initialising card state");
	outb(Card->IOBase + REG_IMR0, 0xFF);
	outb(Card->IOBase + REG_IMR1, 0xFF);
	outd(Card->IOBase + REG_CUR_RX_DESC, Card->DescTablePhys);
	outd(Card->IOBase + REG_CUR_TX_DESC, Card->DescTablePhys + N_RX_DESCS * DESC_SIZE);

	outb(Card->IOBase + REG_TCR, TCR_TRSF(4));
	
	LOG("RX started");
	outb(Card->IOBase + REG_CR0, CR0_BASEVAL);
	outb(Card->IOBase + REG_CR1, CR1_DPOLL);	// Disabled TX polling only?
	
	LOG("ISR state: %02x %02x", inb(Card->IOBase + REG_ISR0), inb(Card->IOBase + REG_ISR1));
}

// --- File Functions ---
tIPStackBuffer *Rhine2_WaitPacket(void *Ptr)
{
	tCard	*card = Ptr;
	tIPStackBuffer	*ret;
	struct sRXDesc	*desc;
	 int	nDesc;
	
	ENTER("pPtr", Ptr);

	LOG("CR0 state: %02x", inb(card->IOBase + REG_CR0));
	if( Semaphore_Wait( &card->ReadSemaphore, 1 ) != 1 )
	{
		LEAVE('n');
		return NULL;
	}
	
	nDesc = 0;
	desc = card->FirstRX;
	while( !(desc->Length & (1 << 15)) )
	{
//		LOG("desc(%p) = {RSR:%04x,Length:%04x,BufferSize:%04x,RXBufferStart:%08x,RDBranchAddress:%08x}",
//			desc, desc->RSR, desc->Length, desc->BufferSize, desc->RXBufferStart, desc->RDBranchAddress);
		// TODO: This kinda expensive call can be changed for an increment, but cbf
		desc = Rhine2_int_GetDescFromPhys(card, desc->RDBranchAddress);
		nDesc ++;
	}

	LOG("%i descriptors in packet", nDesc);

	ret = IPStack_Buffer_CreateBuffer(nDesc);
	desc = card->FirstRX;
	while( !(desc->Length & (1 << 15)) )
	{
		void	*data = Rhine2_int_GetBufferFromPhys(card, desc->RXBufferStart);
		IPStack_Buffer_AppendSubBuffer(ret,
			desc->Length, 0, data,
			Rhine2_int_FreeRXDesc, desc
			);
		desc = Rhine2_int_GetDescFromPhys(card, desc->RDBranchAddress);
	}	
	card->FirstRX = desc;

	LEAVE('p', ret);
	return ret;
}

int Rhine2_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tCard	*card = Ptr;
	size_t	len;
	const void	*data;
	 int	nDescs, first_desc_id;
	struct sTXDesc	*desc = NULL;
	struct sTXDesc	*first_desc = NULL;
	struct sTXDesc	*last_desc = NULL;

	ENTER("pPtr pBuffer", Ptr, Buffer);	

	#if 0
	// Iterate buffers
	nDescs = 0;
	for( int id = -1; -1 != (id = IPStack_Buffer_GetBuffer(Buffer, id, &len, &data)); )
	{
		if( ((tVAddr)data & (PAGE_SIZE-1)) + len > PAGE_SIZE )
			nDescs ++;
		nDescs ++;
	}
	if( nDescs == 0 ) {
		LEAVE('i', -1);
		return -1;
	}

	LOG("%i descriptors needed", nDescs);
	
	if( card->nFreeTX < nDescs ) {
		// Oops... wait?
		// TODO: Semaphore instead?
		LEAVE('i', 1);
		return 1;
	}

	first_desc_id = card->NextTX;
	card->NextTX = (card->NextTX + nDescs) % N_TX_DESCS;

	desc = card->TXDescs + first_desc_id;
	
	nDescs = 0;
	for( int id = -1; -1 != (id = IPStack_Buffer_GetBuffer(Buffer, id, &len, &data)); )
	{
		tPAddr	pdata = MM_GetPhysAddr( (tVAddr)data );
		#if PHYS_BITS > 32
		if( pdata >> 32 ) {
			// TODO: re-map
			Log_Warning("Rhine2", "TODO: Bounce-buffer >32 pbit buffers");
		} 
		#endif
	
		if( (pdata & (PAGE_SIZE-1)) + len > PAGE_SIZE )
		{
			// Need to split into to descriptors
			Log_Warning("Rhine2", "TODO: Split cross-page buffers");
		}
		
		LOG("Buffer %i (%p+0x%x) placed in %p", id-1, data, len, desc);

		// TODO: Rhine I requires 4-byte alignment (fsck that, I don't have one)
	
		desc->TXBufferStart = pdata;
		desc->BufferSize = len | (1 << 15);
		// TODO: TCR
		desc->TCR = 0;
		if( nDescs == 0 )
			desc->TSR = 0;	// OWN will be set below
		else
			desc->TSR = TD_TSR_OWN;

		nDescs ++;
		if(first_desc_id + nDescs == N_TX_DESCS)
			desc = card->TXDescs;
		else
			desc ++;
	}
	#else
	data = IPStack_Buffer_CompactBuffer(Buffer, &len);
	
	nDescs = 1;
	first_desc_id = card->NextTX;
	card->NextTX = (card->NextTX + nDescs) % N_TX_DESCS;
	desc = card->TXDescs + first_desc_id;
	
	desc->TXBufferStart = MM_GetPhysAddr( (tVAddr)data );
	desc->BufferSize = len | (1 << 15);
	desc->TSR = 0;
	desc->TCR = 0;
	#endif
	
	first_desc = card->TXDescs + first_desc_id;
	last_desc = desc;
	
	first_desc->TCR |= TD_TCR_STP;
	last_desc->TCR |= TD_TCR_EDP|TD_TCR_IC;
//	last_desc->BufferSize &= ~(1 << 15);

	first_desc->TSR |= TD_TSR_OWN;

	LOG("%i descriptors allocated, first = %p, last = %p", nDescs, first_desc, last_desc);


	LOG("Waiting for TX to complete");
	outb(card->IOBase + REG_CR0, CR0_TDMD|CR0_BASEVAL);
	Semaphore_Wait(&card->SendSemaphore, 1);

	#if 1
	free((void*)data);
	#endif

	LEAVE('i', 0);
	return 0;
}

void Rhine2_IRQHandler(int Num, void *Ptr)
{
	tCard	*card = Ptr;
	Uint8	isr0 = inb(card->IOBase + REG_ISR0);
	Uint8	isr1 = inb(card->IOBase + REG_ISR1);

	if( isr0 == 0 )	return ;	

	LOG("ISR0 = 0x%02x, ISR1 = 0x%02x", isr0, isr1);

	if( isr0 & ISR0_PRX )
	{
		LOG("PRX");
		Semaphore_Signal(&card->ReadSemaphore, 1);
	}
	
	if( isr0 & ISR0_PTX )
	{
		LOG("PTX");
		Semaphore_Signal(&card->SendSemaphore, 1);
	}
	
	if( isr0 & ISR0_TXE )
	{
		LOG("TX Error... oops");
		Semaphore_Signal(&card->SendSemaphore, 1);
	}
	
	if( isr0 & ISR0_TU )
	{
		LOG("Transmit buffer underflow");
	}

	LOG("Acking interrupts");
	outb(card->IOBase + REG_ISR0, isr0);
	outb(card->IOBase + REG_ISR1, isr1);
}

// --- Helpers ---
struct sRXDesc *Rhine2_int_GetDescFromPhys(tCard *Card, Uint32 Addr)
{
	if( Card->DescTablePhys > Addr )	return NULL;
	if( Card->DescTablePhys + PAGE_SIZE <= Addr )	return NULL;
	if( Addr & 15 )	return NULL;
	return (struct sRXDesc*)Card->DescTable + ((Addr & (PAGE_SIZE-1)) / 16);
}

void *Rhine2_int_GetBufferFromPhys(tCard *Card, Uint32 Addr)
{
	for( int i = 0; i < N_RX_PAGES; i ++ )
	{
		if( Card->RXBuffers[i].Phys > Addr )	continue;
		if( Card->RXBuffers[i].Phys + PAGE_SIZE <= Addr )	continue;
		return Card->RXBuffers[i].Virt + (Addr & (PAGE_SIZE-1));
	}
	return NULL;
}

void Rhine2_int_FreeRXDesc(void *Ptr, size_t u1, size_t u2, const void *u3)
{
	struct sRXDesc	*desc = Ptr;
	
	desc->RSR = 0;
	desc->Length = (1 << 15);	// Reset OWN
}


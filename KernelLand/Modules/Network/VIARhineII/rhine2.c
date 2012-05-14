/*
 * Acess2 VIA Rhine II Driver (VT6102)
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	0
#define VERSION	((0<<8)|10)
#include <acess.h>
#include <modules.h>
#include <drv_pci.h>
#include <semaphore.h>
#include "rhine2_hw.h"
#include <IPStack/include/adapters_api.h>

// === CONSTANTS ===
#define VENDOR_ID	0x1106
#define DEVICE_ID	0x3065

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;
	
	tSemaphore	ReadSemaphore;

	Uint32	RXBuffersPhys;
	void	*RXBuffers;

	Uint32	DescTablePhys;
	void	*DescTable;
	
	struct sTXDesc	*FirstTX;
	struct sTXDesc	*LastTX;	// Most recent unsent packet
	
	struct sRXDesc	*FirstRX;	// Most recent unread packet
	struct sRXDesc	*LastRX;	// End of RX descriptor queue

	void	*IPHandle;	
	Uint8	MacAddr[6];
}	tCard;

// === PROTOTYPES ===
 int	Rhine2_Install(char **Options);
tIPStackBuffer	*Rhine2_WaitPacket(void *Ptr);
 int	Rhine2_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	Rhine2_IRQHandler(int Num);
// --- Helpers ---
struct sRXDesc	*Rhine2_int_GetDescFromPhys(tCard *Card, Uint32 Addr);
void	*Rhine2_int_GetBufferFromPhys(tCard *Card, Uint32 Addr);
void	Rhine2_int_FreeRXDesc(void *Desc, size_t, size_t, const void*);
struct sTXDesc	*Rhine2_int_AllocTXDesc(tCard *Card);
// --- IO ---
void	_WriteDWord(tCard *Card, int Offset, Uint32 Value);

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
	Log_Debug("Rhine2", "%i cards", giRhine2_CardCount);
	
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
		
//		card->IOBase = base;
		card->IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
//		IRQ_AddHandler(card->IRQ, Rhine2_IRQHandler);
		
		
		
//		Log_Log("PCnet3", "Card %i 0x%04x, IRQ %i %02x:%02x:%02x:%02x:%02x:%02x",
//			i, card->IOBase, card->IRQ,
//			card->MacAddr[0], card->MacAddr[1], card->MacAddr[2],
//			card->MacAddr[3], card->MacAddr[4], card->MacAddr[5]
//			);
		
		i ++;
	}
	
	return MODULE_ERR_OK;
}

// --- File Functions ---
tIPStackBuffer *Rhine2_WaitPacket(void *Ptr)
{
	tCard	*card = Ptr;
	tIPStackBuffer	*ret;
	struct sRXDesc	*desc;
	 int	nDesc;
	
	ENTER("pPtr", Ptr);

	if( Semaphore_Wait( &card->ReadSemaphore, 1 ) != 1 )
	{
		LEAVE('n');
		return NULL;
	}
	
	nDesc = 0;
	desc = card->FirstRX;
	while( desc->BufferSize & (1 << 15) )
	{
		desc = Rhine2_int_GetDescFromPhys(card, desc->RDBranchAddress);
		nDesc ++;
	}

	LOG("%i descriptors in packet", nDesc);

	ret = IPStack_Buffer_CreateBuffer(nDesc);
	desc = card->FirstRX;
	while( desc->BufferSize & (1 << 15) )
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
	struct sTXDesc	*first_desc = NULL;
	struct sTXDesc	*last_desc = NULL;

	ENTER("pPtr pBuffer", Ptr, Buffer);	

	// Iterate buffers
	for( int id = -1; -1 != (id = IPStack_Buffer_GetBuffer(Buffer, id, &len, &data)); )
	{
		tPAddr	pdata = MM_GetPhysAddr( (tVAddr)data );
		struct sTXDesc	*desc;
		#if PHYS_BITS > 32
		if( pdata >> 32 ) {
			// TODO: re-map
		} 
		#endif
		
		desc = Rhine2_int_AllocTXDesc(card);
		if(!last_desc)
			first_desc = desc;
		else
			last_desc->TDBranchAddress = MM_GetPhysAddr( (tVAddr)desc );

		desc->TXBufferStart = pdata;
		desc->BufferSize = len;
		// TODO: TCR
		desc->TCR = 0;
		desc->TDBranchAddress = 0;

		last_desc = desc;
	}
	
	if( !first_desc ) {
		LEAVE('i', -1);
		return -1;
	}

	first_desc->TCR |= TD_TCR_STP;
	last_desc->TCR |= TD_TCR_EDP;

	if( card->LastTX )
		card->LastTX->TDBranchAddress = MM_GetPhysAddr( (tVAddr)first_desc );
	else {
		card->FirstTX = first_desc;
		card->LastTX = first_desc;
		_WriteDWord(card, REG_CUR_TX_DESC, MM_GetPhysAddr( (tVAddr)first_desc ));
	}
	
	// TODO: Wait until the packet has sent, then clean up

	return 0;
}

void Rhine2_IRQHandler(int Num)
{
	
}

// --- Helpers ---
struct sRXDesc *Rhine2_int_GetDescFromPhys(tCard *Card, Uint32 Addr)
{
	return NULL;
}

void *Rhine2_int_GetBufferFromPhys(tCard *Card, Uint32 Addr)
{
	return NULL;
}

void Rhine2_int_FreeRXDesc(void *Desc, size_t u1, size_t u2, const void *u3)
{
	
}

struct sTXDesc *Rhine2_int_AllocTXDesc(tCard *Card)
{
	return NULL;
}

// --- IO ---
void _WriteDWord(tCard *Card, int Offset, Uint32 Value)
{
	
}

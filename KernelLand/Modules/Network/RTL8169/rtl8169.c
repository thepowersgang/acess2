/*
 * Acess2 RealTek 8169 Gigabit Network Controller Driver
 * - By John Hodge (thePowersGang)
 *
 * rtl8169.c
 * - Driver source
 */
#define DEBUG	0
#define VERSION	0x001
#include <acess.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_network.h>
#include "rtl8169.h"

#define N_RX_DESCS	128
#define N_TX_DESCS	(256-N_RX_DESCS)	// 256 * 16 = 4096 (1 page)
#define RX_BUF_SIZE	1024	// Most packets will fit in one buffer

#define INT_SYSERR	(1 << 15)	// System Error
#define INT_TIMEOUT	(1 << 14)	// Timer fire (TCTR reg)
#define INT_LINKCHANGE	(1 << 5)
#define	INT_TXERR	(1 << 3)
#define	INT_TXOK	(1 << 2)
#define	INT_RXERR	(1 << 1)
#define	INT_RXOK	(1 << 0)

enum eRegisters
{
	REG_INTMASK  = 0x3C,
	REG_INSSTATE = 0x3E,
	REG_TXC = 0x40,
	REG_RXC = 0x44,
}

// === PROTOTYPES ===
 int	RTL8169_Initialise(char **Arguments);
void	RTL8169_Cleanup(void);
void	RTL8169_InterruptHandler(int IRQ, void *CardPtr);

// === GLOBALS ===
MODULE_DEFINE(0, RTL8169, RTL8169_Initialise, RTL8169_Cleanup, NULL);
 int	giRTL8169_CardCount;
tCard	*gaRTL8169_Cards;

// === CODE ===
int RTL8169_Initialise(char **Arguments)
{
	const Uint16	vendor_id = 0x10EC;
	const Uint16	device_id = 0x8129;	// 8169 = 0x8129 PCI?
	giRTL8169_CardCount = PCI_CountDevices(vendor_id, device_id);
	if( !giRTL8169_CardCount )	return MODULE_ERR_NOTNEEDED;
	
	// Allocate card structures
	gaRTL8169_Cards = malloc( sizeof(tCard) * giRTL8169_CardCount );
	
	// Initialise devices
	for( int id = -1, int i = 0 ; (id = PCI_GetDevice(vendor_id, device_id, id)) != -1; i ++ )
	{
		tCard	*card = &gaRTL8169_Cards[i];
		card->PCIId = id;
		
		card->IOAddr = PCI_GetBAR(id, 0);
		if( !(card->IOAddr & 1) ) {
			// Invalid IO address
			Log_Warning("RTL8169", "Card %i doesn't have BAR0 as an IO space", i);
			continue;
		}
		
		if(card->IOAddr == 1 )
		{
			// Try MMIO
			card->PMemAddr = PCI_GetBAR(id, 1);
			if( card->PMemAddr == 0 ) {
				Log_Warning("RTL8169", "TODO: Allocate MMIO space");
				continue ;
			}
			card->MemAddr = MM_MapHWPages(card->PMemAddr, 1);
		}
	
		card->IRQNum = PCI_GetIRQ(card);
		IRQ_AddHandler( card->IRQNum, RTL8169_InterruptHandler, card );
	
		RTL8169_int_SetupCard(card);
	}
}

void RTL8169_int_SetupCard(tCard *Card)
{
	// Initialise RX/TX Descs
	// TODO: Handle non-64-bit PCI
	Card->RXDescs = MM_AllocDMA(1, -1, &Card->RXDescsPhys);
	Card->TXDescs = Card->RXDescs + N_RX_DESCS;

	// - RX Descs - Max of 4096 bytes per packet (1 page)
	tPAddr	paddr;
	for( int i = 0; i < N_RX_DESCS; i += 2 )
	{
		t8169_Desc	*desc;

		desc = &Card->RXDescs[i];
		if( i % (PAGE_SIZE/RX_BUF_SIZE) == 0 )
			Card->RXBufs[i] = MM_AllocDMA(1, 64, &paddr);
		else
			Card->RXBufs[i] = Card->RXBufs[i-1] + RX_BUF_SIZE;
		desc->FrameLength = RX_BUF_SIZE;
		desc->Flags = TXD_FLAG_OWN;
		desc->VLANTag = 0;
		desc->VLANFlags = 0;
		desc->AddressLow = paddr & 0xFFFFFFFF;
		desc->AddressHigh = paddr >> 32;
		paddr += RX_BUF_SIZE;
	}
	Card->RXDescs[N_RX_DESCS-1].Flags |= TXD_FLAG_EOR;
	// - TX Descs - Just clear
	memset(Card->TXDescs, 0, sizeof(t8169_Desc)*N_TX_DESCS);
	Card->TXDescs[N_TX_DESCS-1].Flags |= TXD_FLAG_EOR;
	
	// Reset
	_WriteB(Card, 0x37, 0x10);
	while( _ReadB(Card, 0x37) & 0x10 )
		;	// TODO: Timeout
	
	// Read MAC address
	for( int i = 0; i < 6; i ++ )
		Card->MacAddr[i] = _ReadB(Card, i);
	
	// Initialise
//	_WriteB(Card, 0x50, 0xC0);
	_WriteD(Card, REG_TXC, 0x03000700);	// TX Config
	_WriteD(Card, REG_RXC, 0x0000E70F);	// RX Config
	_WriteW(Card, 0xDA, RX_BUF_SIZE-1);	// Max RX Size
	_WriteW(Card, 0xEC, 2048/32);	// Max TX size (in units of 32/128 bytes)
	
	_WriteQ(Card, 0x20, MM_GetPhysAddr( (tVAddr)Card->TXDescs ));
//	_WriteQ(Card, 0x28, MM_GetPhysAddr( (tVAddr)Card->TXHighDescs ));
	_WriteQ(Card, 0xE4, MM_GetPhysAddr( (tVAddr)Card->RXDescs ));

	_WriteW(Card, REG_INTMASK, INT_LINKCHANGE|INT_TXOK|INT_RXOK);	// Set interrupt mask

	_WriteB(card, 0x37, 0x0C);	// Enable
}

void RTL8169_InterruptHandler(int IRQ, void *CardPtr)
{
	
}

// --- IO ---
void _WriteW(tCard *Card, int Offset, Uint16 Value)
{
	if( Card->MemAddr )
		((Uint16*)Card->MemAddr)[Offset/2] = Value;
	else
		outw(Card->IOAddr & ~1, Offset, Value);
}

void _WriteD(tCard *Card, int Offset, Uint32 Value)
{
	if( Card->MemAddr )
		((Uint32*)Card->MemAddr)[Offset/4] = Value;
	else
		outl(Card->IOAddr & ~1, Offset, Value);
}

void _WriteQ(tCard *Card, int Offset, Uint64 Value)
{
	if( Card->MemAddr ) {
		((Uint32*)Card->MemAddr)[Offset/4+0] = Value;
		((Uint32*)Card->MemAddr)[Offset/4+1] = Value>>32;
	}
	else {
		outd(Card->IOAddr & ~1, Offset, Value);
		outd(Card->IOAddr & ~1, Offset+4, Value>>32);
	}
}

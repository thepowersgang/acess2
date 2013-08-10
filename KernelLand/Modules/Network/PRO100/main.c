/*
 * Acess2 PRO/100 Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver core
 *
 * Built with reference to the linux e100 driver (drivers/net/ethernet/intel/e100.c)
 * 82559-fast-ethernet-multifunciton-pci-datasheet.pdf
 */
#define DEBUG	1
#include <acess.h>
#include <IPStack/include/adapters_api.h>
#include <modules.h>
#include <drv_pci.h>
#include <timers.h>
#include "pro100.h"

// === CONSTANTS ===
#define NUM_STATIC_CARDS	2
static const Uint16	caSupportedCards[][2] = {
	{0x8086, 0x103D},	// prelude's card
	{0x8086, 0x1209},	// qemu's i82559 emulation
	};
static const int	ciNumSupportedCards = sizeof(caSupportedCards)/sizeof(caSupportedCards[0]);

// === PROTOTYPES ===
 int	PRO100_Install(char **Arguments);
 int	PRO100_InitCard(tCard *Card);
 int	PRO100_Cleanup(void);
tIPStackBuffer	*PRO100_WaitForPacket(void *Ptr);
 int	PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	PRO100_IRQHandler(int Num, void *Ptr);

Uint16	PRO100_int_ReadEEPROM(tCard *Card, Uint8 *AddrLen, size_t Ofs);

static void	_Write8(tCard *Card, int Ofs, Uint8 Val);
static void	_Write16(tCard *Card, int Ofs, Uint16 Val);
static void	_Write32(tCard *Card, int Ofs, Uint32 Val);
static Uint8	_Read8(tCard *Card, int Ofs);
static Uint16	_Read16(tCard *Card, int Ofs);
//static Uint32	_Read32(tCard *Card, int Ofs);
static void	_FlushWait(tCard *Card, int Delay);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, PRO100, PRO100_Install, PRO100_Cleanup, "IPStack", NULL);
tIPStack_AdapterType	gPRO100_AdapterType = {
	.Name = "PRO/100",
	.Type = ADAPTERTYPE_ETHERNET_100M,
	.Flags = 0,	
	.SendPacket = PRO100_SendPacket,
	.WaitForPacket = PRO100_WaitForPacket
};
tCard	gaPRO100_StaticCards[NUM_STATIC_CARDS];

// === CODE ===
int PRO100_Install(char **Arguments)
{
	 int	cardidx = 0;
	for( int i = 0; i < ciNumSupportedCards; i ++ )
	{
		const Uint16	*ven_dev = caSupportedCards[i];
		LOG("Checking %04x:%04x: %i reported", ven_dev[0], ven_dev[1],
			PCI_CountDevices(ven_dev[0], ven_dev[1]));
		for( int idx = 0, pciid = -1; -1 != (pciid = PCI_GetDevice(ven_dev[0], ven_dev[1], idx)); idx++ )
		{
			Uint8	irq = PCI_GetIRQ(pciid);
			Uint32	mmiobase = PCI_GetValidBAR(pciid, 0, PCI_BARTYPE_MEM32);
			Uint16	iobase = PCI_GetValidBAR(pciid, 1, PCI_BARTYPE_IO);
			LOG("IO Base = %x, MMIOBase = %x", iobase, mmiobase);

			PCI_SetCommand(pciid, PCI_CMD_IOENABLE|PCI_CMD_BUSMASTER, 0);
			
			tCard	*card;
			if( cardidx < NUM_STATIC_CARDS ) {
				card = &gaPRO100_StaticCards[cardidx++];
			}
			else {
				card = malloc(sizeof(tCard));
			}

			card->IOBase = iobase;
			//card->MMIO = MM_MapHWPages(mmiobase, 1);
			IRQ_AddHandler(irq, PRO100_IRQHandler, card); 
			
			// TODO: Error check
			PRO100_InitCard(card);
			
			IPStack_Adapter_Add(&gPRO100_AdapterType, card, card->MAC.Bytes);
		}
	}
	return MODULE_ERR_OK;
}

int PRO100_InitCard(tCard *Card)
{
	// Initialise structures
	Semaphore_Init(&Card->TXCommandSem, NUM_TX, NUM_TX, "PRO100", "Command Buffers");
	Semaphore_Init(&Card->RXSemaphore, 0, NUM_RX, "PRO100", "Receive");

	// Card reset
	_Write32(Card, REG_Port, PORT_SELECTIVERESET);
	_FlushWait(Card, 20);	// - Write Flush, wait 20us
	_Write32(Card, REG_Port, PORT_SOFTWARERESET);
	_FlushWait(Card, 20);	// - Write Flush, wait 20us

	// Read MAC address
	Uint8	addr_len = 8;	// default to 8, updated on first read
	Card->MAC.Words[0] = PRO100_int_ReadEEPROM(Card, &addr_len, 0);
	Card->MAC.Words[1] = PRO100_int_ReadEEPROM(Card, &addr_len, 1);
	Card->MAC.Words[2] = PRO100_int_ReadEEPROM(Card, &addr_len, 2);

	// Set interrupt mask
	_Write8(Card, REG_IntMask, 0);

	// Prepare Command Unit
	Card->TXCommands = MM_AllocDMA(1, 32, NULL);
	Uint32	txbase = MM_GetPhysAddr(Card->TXCommands);
	ASSERT(Card->TXCommands);
	for( int i = 0; i < NUM_TX; i ++ )
	{
		tCommandUnit	*cu = &Card->TXCommands[i].Desc.CU;
		cu->Status = 0;
		cu->Command = CMD_Nop|CMD_Suspend;
		cu->Link = MM_GetPhysAddr(&Card->TXCommands[(i+1)%NUM_TX]) - txbase;
	}
	
	_Write32(Card, REG_GenPtr, txbase);
	_Write16(Card, REG_Command, CU_CMD_BASE);
	// Ensure CU is in suspend before we attempt sending
	Card->LastTXIndex = 1;
	Card->CurTXIndex = 1;
	_Write32(Card, REG_GenPtr, 0);
	_Write16(Card, REG_Command, CU_CMD_START);

	// Create RX Descriptors
	for( int i = 0; i < NUM_RX; i += 2 )
	{
		char	*base = MM_AllocDMA(1, 32, NULL);
		ASSERT(base);
		Card->RXBufs[i+0] = (void*)base;
		Card->RXBufs[i+1] = (void*)(base + 0x800);
		for( int j = 0; j < 2; j ++ )
		{
			tRXBuffer	*rx = Card->RXBufs[i+j];
			rx->CU.Status = 0;
			rx->CU.Command = 0;
			// Link is populated later
			rx->Size = RX_BUF_SIZE;
			rx->RXBufAddr = 0;	// unused?
		}
	}

	// NOTE: All `Link` values are relative to the RX base address	
	Uint32	rx_desc_phys = MM_GetPhysAddr(Card->RXBufs[0]);
	for( int i = 0; i < NUM_RX-1; i ++ )
	{
		tRXBuffer	*rx = Card->RXBufs[i];
		rx->CU.Link = MM_GetPhysAddr(Card->RXBufs[i+1]) - rx_desc_phys;
	}
	Card->RXBufs[NUM_RX-1]->CU.Command = CMD_Suspend;
	Card->RXBufs[NUM_RX-1]->CU.Link = 0;	// link = 0, loop back
	
	// Set RX Buffer base
	_Write32(Card, REG_GenPtr, rx_desc_phys);
	_Write16(Card, REG_Command, RX_CMD_ADDR_LOAD);
	
	_Write32(Card, REG_GenPtr, 0);
	_Write16(Card, REG_Command, RX_CMD_START);

	return 0;
}

int PRO100_Cleanup(void)
{
	return 0;
}

void PRO100_ReleaseRXBuf(void *Arg, size_t HeadLen, size_t FootLen, const void *Data)
{
	tCard	*Card = Arg;
	tRXBuffer	*buf = (tRXBuffer*)Data - 1;

	 int	idx;
	for( idx = 0; idx < NUM_RX && Card->RXBufs[idx] != buf; idx ++ )
		;
	ASSERT(idx != NUM_RX);

	tRXBuffer	*prev = Card->RXBufs[ (idx-1+NUM_RX)%NUM_RX ];
	buf->CU.Status = 0;
	buf->CU.Command = 0;
	prev->CU.Command &= ~CMD_Suspend;
	
	// Resume
	_Write16(Card, REG_Command, RX_CMD_RESUME);
}


tIPStackBuffer *PRO100_WaitForPacket(void *Ptr)
{
	tCard	*Card = Ptr;
	// Wait for a packet
	do {
		Semaphore_Wait(&Card->RXSemaphore, 1);
	} while( Card->RXBufs[Card->CurRXIndex]->CU.Status == 0 );
	// Mark previous buffer as suspend (stops the RX unit running into old packets
	Card->RXBufs[ (Card->CurRXIndex-1+NUM_RX)%NUM_RX ]->CU.Command |= CMD_Suspend;
	tRXBuffer *buf = Card->RXBufs[Card->CurRXIndex++];
	
	// Return packet (freed in PRO100_ReleaseRXBuf);
	tIPStackBuffer	*ret = IPStack_Buffer_CreateBuffer(1);
	// - actual data is just after the descriptor
	IPStack_Buffer_AppendSubBuffer(ret, buf->Count, 0, buf+1, PRO100_ReleaseRXBuf, Card);
	
	return ret;
}

void PRO100_int_SetBuf(tTXCommand *TXC, int *IndexPtr, Uint32 Addr, Uint16 Len)
{
	ASSERTC(*IndexPtr, <, NUM_LOCAL_TXBUFS);
	
	tTXBufDesc	*txb = &TXC->LocalBufs[*IndexPtr];
	txb->Addr = Addr;
	txb->Len = Len;
	(*IndexPtr) ++;
}

int PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	tCard	*Card = Ptr;

	Semaphore_Wait(&Card->TXCommandSem, 1);

	// Acquire a command buffer
	Mutex_Acquire(&Card->lTXCommands);
	int txc_idx = Card->CurTXIndex;
	Card->CurTXIndex = (Card->CurTXIndex + 1) % NUM_TX;
	Mutex_Release(&Card->lTXCommands);
	tTXCommand	*txc = &Card->TXCommands[txc_idx];

	// Populate
	 int	txb_idx = 0;
	const void	*ptr;
	size_t	len;
	size_t	total_size = 0;
	 int	buf_idx = -1;
	while( (buf_idx = IPStack_Buffer_GetBuffer(Buffer, buf_idx, &len, &ptr)) != -1 ) 
	{
		#if PHYS_BITS > 32
		if( MM_GetPhysAddr(ptr) >> 32 ) {
			// Need to bounce
			TODO();
			continue ;
		}
		#endif
		
		ASSERTC(len, <, PAGE_SIZE);
		
		// Check if buffer split is required
		if( MM_GetPhysAddr((char*)ptr + len-1) != MM_GetPhysAddr(ptr)+len-1 )
		{
			// Need to split buffer
			size_t	space = PAGE_SIZE - ((tVAddr)ptr % PAGE_SIZE);
			PRO100_int_SetBuf(txc, &txb_idx, MM_GetPhysAddr(ptr), space);
			PRO100_int_SetBuf(txc, &txb_idx, MM_GetPhysAddr((char*)ptr+space), len-space);
		}
		else
		{
			// Single buffer
			PRO100_int_SetBuf(txc, &txb_idx, MM_GetPhysAddr(ptr), len);
		}
		
		total_size += len;
	}

	// Set buffer pointer
	Card->TXBuffers[txc_idx] = Buffer;
	// Mark as usable
	txc->Desc.TBDArrayAddr = 0xFFFFFFFF;
	txc->Desc.TCBBytes = total_size;
	txc->Desc.TXThreshold = 0;	// TODO: What does this do on RHW?
	txc->Desc.TBDCount = 0;
	txc->Desc.CU.Command = CMD_Suspend|CMD_Tx;
	// - Mark previous as not suspended
	Card->TXCommands[ (txc_idx-1+NUM_TX) % NUM_TX ].Desc.CU.Command &= ~CMD_Suspend;

	IPStack_Buffer_LockBuffer(Buffer);

	// And dispatch
	// - If currently running or idle, this should not matter
	// NOTE: Qemu describes this behavior as 'broken'
	_Write16(Card, REG_Command, CU_CMD_RESUME);

	IPStack_Buffer_LockBuffer(Buffer);
	IPStack_Buffer_UnlockBuffer(Buffer);

	return 0;
}

void PRO100_IRQHandler(int Num, void *Ptr)
{
	tCard	*Card = Ptr;
	Uint8	status = _Read8(Card, REG_Ack);
	
	if( !status )
		return ;
	
	_Write8(Card, REG_Ack, status);
	LOG("status = %x", status);
	if( status & ISR_FR ) {
		LOG("FR");
		Semaphore_Signal(&Card->RXSemaphore, 1);
	}
	
	// CU Idle
	if( status & ISR_CNA )
	{
		// Chase the next command buffer
		while( Card->LastTXIndex != Card->CurTXIndex )
		{
			 int	idx = Card->LastTXIndex++;
			// Once we hit an incomplete command, stop
			if( !(Card->TXCommands[idx].Desc.CU.Status & CU_Status_Complete) )
				break ;
			IPStack_Buffer_UnlockBuffer( Card->TXBuffers[idx] );
			Semaphore_Signal(&Card->TXCommandSem, 1);
		}
		LOG("CU Idle (%i / %i)", Card->LastTXIndex, Card->CurTXIndex);
	}
}

Uint16 PRO100_int_ReadEEPROM(tCard *Card, Uint8 *addr_len, size_t Ofs)
{
	ASSERTC( *addr_len, <=, 12 );
	
	Uint32	addr_data = ((EEPROM_OP_READ << *addr_len) | Ofs) << 16;

	// Deslect chip (god knows what state it was left in)
	_Write16( Card, REG_EEPROMCtrl, 0 );
	_FlushWait(Card, 4);	// Flush + 4us
	// Raise CS
	_Write16( Card, REG_EEPROMCtrl, EEPROM_CTRL_CS | EEPROM_CTRL_SK );
	_FlushWait(Card, 4);	// Flush + 4us

	Uint32	data = 0;	

	// 2 preamble (0,1) + 2 command (read=1,0) + n address + 16 data
	for( int i = (2+2+*addr_len+16); i --; )
	{
		Uint16	ctrl = EEPROM_CTRL_CS | ((addr_data & (1 << i)) ? EEPROM_CTRL_DI : 0);
		_Write16( Card, REG_EEPROMCtrl, ctrl );
		_FlushWait(Card, 4);	// Flush + 4us
		_Write16( Card, REG_EEPROMCtrl, ctrl | EEPROM_CTRL_SK );
		_FlushWait(Card, 4);	// Flush + 4us
		
		ctrl = _Read16( Card, REG_EEPROMCtrl );
		// Once the address is fully recieved, the card emits a zero bit
		if( !(ctrl & EEPROM_CTRL_DO) && i > 16 )
		{
			*addr_len = *addr_len - (i - 16);
			LOG("addr_len = %i", *addr_len);
			
			i = 16;
		}
		
		data = (data << 1) | (ctrl & EEPROM_CTRL_DO ? 1 : 0);
	}

	// Deslect chip
	_Write16( Card, REG_EEPROMCtrl, 0 );
	_FlushWait(Card, 4);	// Flush + 4us

	LOG("Read %x from EEPROM ofs %i", data&0xFFFF, Ofs);
	return (data & 0xFFFF);
}

static void _Write8(tCard *Card, int Ofs, Uint8 Val) {
	//LOG("%p +%i := %02x", Card, Ofs, Val);
	outb(Card->IOBase + Ofs, Val);
}
static void _Write16(tCard *Card, int Ofs, Uint16 Val) {
	//LOG("%p +%i := %04x", Card, Ofs, Val);
	ASSERT( !(Ofs & 1) );
	outw(Card->IOBase + Ofs, Val);
}
static void _Write32(tCard *Card, int Ofs, Uint32 Val) {
	//LOG("%p +%i := %08x", Card, Ofs, Val);
	ASSERT( !(Ofs & 3) );
	outd(Card->IOBase + Ofs, Val);
}
static Uint8 _Read8(tCard *Card, int Ofs) {
	Uint8 rv = inb(Card->IOBase + Ofs);
	//LOG("%p +%i == %02x", Card, Ofs, rv);
	return rv;
}
static Uint16 _Read16(tCard *Card, int Ofs) {
	ASSERT( !(Ofs & 1) );
	Uint16 rv = inw(Card->IOBase + Ofs);
	//LOG("%p +%i == %04x", Card, Ofs, rv);
	return rv;
}
//static Uint32 _Read32(tCard *Card, int Ofs) { return ind(Card->IOBase + Ofs); }

static void _FlushWait(tCard *Card, int Delay)
{
	_Read16( Card, REG_Status );
	if(Delay > 0)
	{
		Time_MicroSleep(Delay);
	}
}


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
#include <acess.h>
#include <IPStack/include/adapters_api.h>
#include <modules.h>
#include <drv_pci.h>
#include <timers.h>
#include "pro100.h"

// === CONSTANTS ===
#define NUM_STATIC_CARDS	2
const Uint16	caSupportedCards[][2] = {
	{0x8086, 0x103D},
	};
const int	ciNumSupportedCards = sizeof(caSupportedCards)/sizeof(caSupportedCards[0]);

// === PROTOTYPES ===
 int	PRO100_Install(char **Arguments);
 int	PRO100_InitCard(tCard *Card);
 int	PRO100_Cleanup(void);
tIPStackBuffer	*PRO100_WaitForPacket(void *Ptr);
 int	PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	PRO100_IRQHandler(int Num, void *Ptr);

Uint16	PRO100_int_ReadEEPROM(tCard *Card, size_t Ofs);

static void	_Write16(tCard *Card, int Ofs, Uint16 Val);
static void	_Write32(tCard *Card, int Ofs, Uint32 Val);
static Uint16	_Read16(tCard *Card, int Ofs);
static Uint32	_Read32(tCard *Card, int Ofs);
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
		int pciid = -1;
		while( -1 != (pciid = PCI_GetDevice(ven_dev[0], ven_dev[1], pciid)) )
		{
			Uint32	base = PCI_GetValidBAR(pciid, 0, PCI_BARTYPE_MEM32);
			tCard	*card;
			if( cardidx < NUM_STATIC_CARDS ) {
				card = &gaPRO100_StaticCards[cardidx++];
			}
			else {
				card = malloc(sizeof(tCard));
			}
			
			card->MMIO = MM_MapHWPages(base, 1);
			
			// TODO: Error check
			PRO100_InitCard(card);
			
			IPStack_Adapter_Add(&gPRO100_AdapterType, card, card->MAC.Bytes);
		}
	}
	return MODULE_ERR_OK;
}

int PRO100_InitCard(tCard *Card)
{
	// Card reset
	_Write32(Card, REG_Port, PORT_SELECTIVERESET);
	_FlushWait(Card, 20);	// - Write Flush, wait 20us
	_Write32(Card, REG_Port, PORT_SOFTWARERESET);
	_FlushWait(Card, 20);	// - Write Flush, wait 20us

	// Read MAC address
	Card->MAC.Words[0] = PRO100_int_ReadEEPROM(Card, 0);
	Card->MAC.Words[1] = PRO100_int_ReadEEPROM(Card, 1);
	Card->MAC.Words[2] = PRO100_int_ReadEEPROM(Card, 2);

	// Create RX Descriptors
	
	// Set RX Buffer base
	_Write32(Card, REG_GenPtr, rx_desc_phys);
	_Write32(Card, REG_Command, RX_CMD_ADDR_LOAD);
	
	_Write32(Card, REG_GenPtr, 0);
	_Write32(Card, REG_Command, RX_CMD_START);
	
	return 0;
}

int PRO100_Cleanup(void)
{
	return 0;
}

tIPStackBuffer *PRO100_WaitForPacket(void *Ptr)
{
	return NULL;
}

int PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	return -1;
}

void PRO100_IRQHandler(int Num, void *Ptr)
{
	
}

Uint16 PRO100_int_ReadEEPROM(tCard *Card, size_t Ofs)
{
	Uint8	addr_len = 8;
	Uint32	addr_data = ((EEPROM_OP_READ << addr_len) | Ofs) << 16;
	
	_Write16( Card, REG_EEPROMCtrl, EEPROM_CTRL_CS | EEPROM_CTRL_SK );
	_FlushWait(Card, 4);	// Flush + 4us

	Uint32	data = 0;	

	for( int i = 32; i --; )
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
			addr_len = addr_len - (i - 16);
			i = 17;
		}
		
		data = (data << 1) | (ctrl & EEPROM_CTRL_DO ? 1 : 0);
	}

	// Deslect chip
	_Write16( Card, REG_EEPROMCtrl, 0 );
	_FlushWait(Card, 4);	// Flush + 4us

	return (data & 0xFFFF);
}

static void _Write16(tCard *Card, int Ofs, Uint16 Val) { outw(Card->IOBase + Ofs, Val); }
static void _Write32(tCard *Card, int Ofs, Uint32 Val) { outd(Card->IOBase + Ofs, Val); }
static Uint16 _Read16(tCard *Card, int Ofs) { return inw(Card->IOBase + Ofs); }
static Uint32 _Read32(tCard *Card, int Ofs) { return ind(Card->IOBase + Ofs); }

static void _FlushWait(tCard *Card, int Delay)
{
	_Read16( Card, REG_Status );
	if(Delay > 0)
	{
		Time_MicroSleep(Delay);
	}
}


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
#include <IPStack/include/adapter_api.h>
#include <modules.h>

// === CONSTANTS ===
const Uint16	caSupportedCards[][2] = {
	{0x8086, 0x103D},
	};
const int	ciNumSupportedCards = sizeof(caSupportedCards)/sizeof(caSupportedCards[0]);

// === PROTOTYPES ===
 int	PRO100_Install(char **Arguments);
 int	PRO100_InitCard(tCard *Card);
void	PRO100_Cleanup(void);
tIPStackBuffer	*PRO100_WaitForPacket(void *Ptr);
 int	PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	PRO100_IRQHandler(int Num, void *Ptr);

size_t	PRO100_int_ReadEEPROM(tCard *Card, size_t Ofs, size_t Len, void *Buffer);
// === GLOBALS ===
MODULE_DEFINE(0, PRO100, PRO100_Install, PRO100_Cleanup, "IPStack", NULL);
tIPStack_AdapterType	gPRO100_AdapterType = {
	.Name = "PRO/100",
	.Type = ADAPTERTYPE_ETHERNET_100M,
	.Flags = 0,	
	.SendPacket = PRO100_SendPacket,
	.WaitForPacket = PRO100_WaitForPacket
	
};

// === CODE ===
int PRO100_Install(char **Arguments)
{
	return MODULE_ERR_OK;
}

int PRO100_InitCard(tCard *Card)
{
	// Card reset
	Card->MMIO->Port = PORT_SELECTIVERESET;
	// - Write Flush, wait 20us
	Card->MMIO->Port = PORT_SOFTWARERESET;
	// - Write Flush, wait 20us

	// Read MAC address
	Card->MAC.Words[0] = PRO100_int_ReadEEPROM(Card, 0);
	Card->MAC.Words[1] = PRO100_int_ReadEEPROM(Card, 1);
	Card->MAC.Words[2] = PRO100_int_ReadEEPROM(Card, 2);
	
	return 0;
}

void PRO100_Cleanup(void)
{
	return 0;
}

tIPStackBuffer	*PRO100_WaitForPacket(void *Ptr)
{
}

int PRO100_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	
}

void PRO100_IRQHandler(int Num, void *Ptr)
{
	
}

Uint16 PRO100_int_ReadEEPROM(tCard *Card, size_t Ofs)
{
	Uint8	addr_len = 8;
	Uint32	addr_data = ((EEPROM_OP_READ << addr_len) | Ofs) << 16;
	
	Card->MMIO->EEPROMCtrl = EEPROM_CTRL_CS | EEPROM_CTRL_SK;
	// Flush + 4us
	
	for( int i = 32; i --; )
	{
		Uint8	ctrl = EEPROM_CTRL_CS | ((addr_data & (1 << i)) ? EEPROM_CTRL_DI : 0);
		Card->MMIO->EEPROMCtrl = ctrl;
		// Flush + 4us
		Card->MMIO->EEPROMCtrl = ctrl | EEPROM_CTRL_SK;
		// Flush + 4us
		
		ctrl = Card->MMIO->EEPROMCtrl;
		// Once the address is fully recieved, the card emits a zero bit
		if( !(ctrl & EEPROM_CTRL_DO) && i > 16 )
		{
			addr_len = addr_len - (i - 16);
			i = 17;
		}
		
		data = (data << 1) | (ctrl & EEPROM_CTRL_DO ? 1 : 0)
	}

	// Deslect chip
	Card->MMIO->EEPROMCtrl = 0;	
	// Flush + 4us

	return (data & 0xFFFF);
}

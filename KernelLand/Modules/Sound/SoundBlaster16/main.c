/*
 * Acess2 SoundBlaster16 Driver
 */
#define DEBUG	0
#include <acess.h>
#include <errno.h>
#include <modules.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_sound.h>

#define INT

// === TYPES ===
typedef struct sSB16
{
	Uint16	Base;
}	tSB16;

// === CONSTANTS ===
enum {
	SB16_PORT_RESET	= 0x6,
	SB16_PORT_READ	= 0xA,
	SB16_PORT_WRITE	= 0xC,
	SB16_PORT_AVAIL	= 0xE
};
#define SB16_BASE_PORT	0x200
enum {
	SB16_CMD_RAW8    = 0x10,	// 8-bit DAC value follows
	SB16_CMD_DMAFREQ = 0x40,	// followed by TIME_CONSTANT = 256 - 1000000 / frequency
	SB16_CMD_DMASTOP = 0xD0,
	SB16_CMD_SPKRON  = 0xD1,
	SB16_CMD_SPKROFF = 0xD3,
	SB16_CMD_DMACONT = 0xD4,
	
	// DMA Types (uses channel 1)
	// - Followed by 16-bit length (well, length - 1)
	SB16_CMD_DMA_8BIT = 0x14,
};


// === PROTOTYPES ===
// Driver
 int	SB16_Install(char **Arguments);
void	SB16_Uninstall();
// Filesystem
Uint64	SB16_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	SB16_IOCtl(tVFS_Node *Node, int ID, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, SoundBlaster16, SB16_Install, SB16_Uninstall, "PCI", NULL);
tDevFS_Driver	gBGA_DriverStruct = {
	NULL, "SoundBlaster16",
	{
	.Write = SB16_Write,
	.IOCtl = SB16_IOCtl
	}
};

// === CODE ===
int SB16_Install(char **Arguments)
{
	 int	jumper_port_setting = 1;	// 1-6 incl
	
	// Reset
	outb(card->Base+SB16_PORT_RESET, 1);
	// - Wait 3us
	outb(card->Base+SB16_PORT_RESET, 0);
	
	SB16_ReadDSP(card);
	
	return MODULE_ERR_OK;
}

void SB16_Uninstall()
{
}

/**
 * \fn Uint64 SB16_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write to the framebuffer
 */
Uint64 SB16_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{	
	return 0;
}

/**
 * \fn int SB16_IOCtl(tVFS_Node *Node, int ID, void *Data)
 * \brief Handle messages to the device
 */
int SB16_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	return -1;
}

//
int SB16_WriteDSP(tSB16 *Card, Uint8 Value)
{
	// Wait for the card to be ready
	while( inb(Card->Base+SB16_PORT_WRITE) & 0x80 )
		;
	
	outb(Card->Base+SB16_PORT_WRITE, Value);
	
	return 0;
}

Uint8 SB16_ReadDSP(tSB16 *Card)
{
	// Wait for bit 7 of AVAIL
	while( !(inb(card->Base+SB16_PORT_AVAIL) & 0x80) )
		;
	return inb(card->Base+SB16_PORT_READ);
}

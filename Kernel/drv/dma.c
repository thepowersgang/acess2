/*
 * AcessOS 1.0
 * DMA Driver
 */
#include <acess.h>
#include <modules.h>

#define DMA_SIZE	(0x2400)
#define DMA_ADDRESS(c)	((c)*DMA_SIZE+0x500)	//Save Space for IDT and BDA

#define LOWB(x)	((x)&0xFF)
#define HIB(x)	(((x)>>8)&0xFF)
#define HIW(x)	(((x)>>16)&0xFFFF)

// === TYPES ===
typedef struct
{
	 int	mode;
	char	*address;
} t_dmaChannel;

// === PROTOTYPES ===
 int	DMA_Install();
void	DMA_SetChannel(int Channel, int length, int read);
 int	DMA_ReadData(int channel, int count, void *buffer);

// === CONSTANTS ===
const Uint8 cMASKPORT [8] = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
const Uint8 cMODEPORT [8] = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
const Uint8 cCLEARPORT[8] = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };
const Uint8 cPAGEPORT [8] = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
const Uint8 cADDRPORT [8] = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
const Uint8 cCOUNTPORT[8] = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, ISADMA, DMA_Install, NULL, NULL);
char	*dma_addresses[8];
t_dmaChannel	dma_channels[8];

// === CODE ===
/**
 * \fn int DMA_Install()
 * \brief Initialise DMA channels
 */
int DMA_Install()
{
	Uint	i;
	for(i=8;i--;)
	{
		outb( cMASKPORT[i], 0x04 | (i & 0x3) ); // mask channel
		outb( cCLEARPORT[i], 0x00 );
		outb( cMODEPORT[i], 0x48 | (i & 0x3) );	//Read Flag
		outb( 0xd8, 0xff);	//Reset Flip-Flop
		outb( cADDRPORT[i], LOWB(DMA_ADDRESS(i)) );	// send address
		outb( cADDRPORT[i], HIB(DMA_ADDRESS(i)) );	// send address
		outb( 0xd8, 0xff);	//Reset Flip-Flop
		outb( cCOUNTPORT[i], LOWB(DMA_SIZE) );      // send size
		outb( cCOUNTPORT[i], HIB(DMA_SIZE) );       // send size
		outb( cPAGEPORT[i], LOWB(HIW(DMA_ADDRESS(i))) );	// send page
		outb( cMASKPORT[i], i & 0x3 );              // unmask channel
		
		dma_channels[i].mode = 0;
		dma_addresses[i] = (char*)DMA_ADDRESS(i);
		dma_addresses[i] += KERNEL_BASE;
	}
	return MODULE_ERR_OK;
}

/**
 * \fn void DMA_SetChannel(int Channel, int length, int read)
 * \brief Set DMA Channel Length and RW
 */
void DMA_SetChannel(int Channel, int length, int read)
{
	Uint	chan = Channel & 7;
	read = !!read;
	if(length > DMA_SIZE)	length = DMA_SIZE;
	length --;	//Adjust for DMA
	//__asm__ __volatile__ ("cli");
	outb( cMASKPORT[chan], 0x04 | (chan & 0x3) );		// mask channel
	outb( cCLEARPORT[chan], 0x00 );
	outb( cMODEPORT[chan], (0x44 + (!read)*4) | (chan & 0x3) );
	outb( cADDRPORT[chan], LOWB(DMA_ADDRESS(chan)) );		// send address
	outb( cADDRPORT[chan], HIB(DMA_ADDRESS(chan)) );		// send address
	outb( cPAGEPORT[chan], HIW(DMA_ADDRESS(chan)) );		// send page
	outb( cCOUNTPORT[chan], LOWB(length) );      // send size
	outb( cCOUNTPORT[chan], HIB(length) );       // send size
	outb( cMASKPORT[chan], chan & 0x3 );              // unmask channel
	dma_addresses[chan] = (char*)DMA_ADDRESS(chan);
	dma_addresses[chan] += KERNEL_BASE;
	//__asm__ __volatile__ ("sti");
}

/**
 * \fn void DMA_ReadData(int channel, int count, void *buffer)
 * \brief Read data from a DMA buffer
 */
int DMA_ReadData(int channel, int count, void *buffer)
{
	if(channel < 0 || channel > 7)
		return -1;
	if(count < 0 || count > DMA_SIZE)
		return -2;
	//LogF("memcpy(*0x%x, dma_channels[channel].address, count)\n", buffer
	memcpy(buffer, dma_addresses[channel], count);
	return 0;
}

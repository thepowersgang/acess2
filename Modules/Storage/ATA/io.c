/*
 * Acess2 IDE Harddisk Driver
 * - io.c
 *
 * Disk Input/Output control
 */
#define DEBUG	1
#include <acess.h>
#include <modules.h>	// Needed for error codes
#include <drv_pci.h>
#include "common.h"

// === MACROS ===
#define IO_DELAY()	do{inb(0x80); inb(0x80); inb(0x80); inb(0x80);}while(0)

// === TYPES ===
/**
 * \brief PRDT Entry
 */
typedef struct
{
	Uint32	PBufAddr;	// Physical Buffer Address
	Uint16	Bytes;	// Size of transfer entry
	Uint16	Flags;	// Flags
} __attribute__ ((packed))	tPRDT_Ent;

/**
 * \brief Structure returned by the ATA IDENTIFY command
 */
typedef struct
{
	Uint16	Flags;		// 1
	Uint16	Usused1[9];	// 10
	char	SerialNum[20];	// 20
	Uint16	Usused2[3];	// 23
	char	FirmwareVer[8];	// 27
	char	ModelNumber[40];	// 47
	Uint16	SectPerInt;	// 48 - Low byte only
	Uint16	Unused3;	// 49
	Uint16	Capabilities[2];	// 51
	Uint16	Unused4[2];	// 53
	Uint16	ValidExtData;	// 54
	Uint16	Unused5[5];	 // 59
	Uint16	SizeOfRWMultiple;	// 60
	Uint32	Sectors28;	// 62
	Uint16	Unused6[100-62];
	Uint64	Sectors48;
	Uint16	Unused7[256-104];
} __attribute__ ((packed))	tIdentify;

// === PROTOTYPES ===
 int	ATA_SetupIO(void);
Uint64	ATA_GetDiskSize(int Disk);
Uint16	ATA_GetBasePort(int Disk);
// Read/Write DMA
 int	ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer);
 int	ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, const void *Buffer);
// IRQs
void	ATA_IRQHandlerPri(int UNUSED(IRQ));
void	ATA_IRQHandlerSec(int UNUSED(IRQ));
// Controller IO
Uint8	ATA_int_BusMasterReadByte(int Ofs);
void	ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value);
void	ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value);

// === GLOBALS ===
Uint32	gATA_BusMasterBase = 0;
Uint8	*gATA_BusMasterBasePtr = 0;
 int	gATA_IRQPri = 14;
 int	gATA_IRQSec = 15;
 int	giaATA_ControllerLock[2] = {0};	//!< Spinlocks for each controller
Uint8	gATA_Buffers[2][(MAX_DMA_SECTORS+0xFFF)&~0xFFF] __attribute__ ((section(".padata")));
volatile int	gaATA_IRQs[2] = {0};
tPRDT_Ent	gATA_PRDTs[2] = {
	{0, 512, IDE_PRDT_LAST},
	{0, 512, IDE_PRDT_LAST}
};

// === CODE ===
/**
 * \brief Sets up the ATA controller's DMA mode
 */
int ATA_SetupIO(void)
{
	 int	ent;
	tPAddr	addr;

	ENTER("");

	// Get IDE Controller's PCI Entry
	ent = PCI_GetDeviceByClass(0x0101, 0xFFFF, -1);
	LOG("ent = %i", ent);
	gATA_BusMasterBase = PCI_GetBAR4( ent );
	if( gATA_BusMasterBase == 0 ) {
		Log_Warning("ATA", "It seems that there is no Bus Master Controller on this machine. Get one");
		// TODO: Use PIO mode instead
		LEAVE('i', MODULE_ERR_NOTNEEDED);
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Map memory
	if( !(gATA_BusMasterBase & 1) )
	{
		if( gATA_BusMasterBase < 0x100000 )
			gATA_BusMasterBasePtr = (void*)(KERNEL_BASE | (tVAddr)gATA_BusMasterBase);
		else
			gATA_BusMasterBasePtr = (void*)( MM_MapHWPages( gATA_BusMasterBase, 1 ) + (gATA_BusMasterBase&0xFFF) );
		LOG("gATA_BusMasterBasePtr = %p", gATA_BusMasterBasePtr);
	}
	else {
		// Bit 0 is left set as a flag to other functions
		LOG("gATA_BusMasterBase = 0x%x", gATA_BusMasterBase & ~1);
	}

	// Register IRQs and get Buffers
	IRQ_AddHandler( gATA_IRQPri, ATA_IRQHandlerPri );
	IRQ_AddHandler( gATA_IRQSec, ATA_IRQHandlerSec );

	gATA_PRDTs[0].PBufAddr = MM_GetPhysAddr( (tVAddr)&gATA_Buffers[0] );
	gATA_PRDTs[1].PBufAddr = MM_GetPhysAddr( (tVAddr)&gATA_Buffers[1] );

	LOG("gATA_PRDTs = {PBufAddr: 0x%x, PBufAddr: 0x%x}", gATA_PRDTs[0].PBufAddr, gATA_PRDTs[1].PBufAddr);

	addr = MM_GetPhysAddr( (tVAddr)&gATA_PRDTs[0] );
	LOG("addr = 0x%x", addr);
	ATA_int_BusMasterWriteDWord(4, addr);
	addr = MM_GetPhysAddr( (tVAddr)&gATA_PRDTs[1] );
	LOG("addr = 0x%x", addr);
	ATA_int_BusMasterWriteDWord(12, addr);

	// Enable controllers
	outb(IDE_PRI_BASE+1, 1);
	outb(IDE_SEC_BASE+1, 1);

	// return
	LEAVE('i', MODULE_ERR_OK);
	return MODULE_ERR_OK;
}

Uint64 ATA_GetDiskSize(int Disk)
{
	union {
		Uint16	buf[256];
		tIdentify	identify;
	}	data;
	Uint16	base;
	Uint8	val;
	 int	i;
	ENTER("iDisk", Disk);

	base = ATA_GetBasePort( Disk );

	// Send Disk Selector
	if(Disk == 1 || Disk == 3)
		outb(base+6, 0xB0);
	else
		outb(base+6, 0xA0);
	IO_DELAY();
	
	// Check for a floating bus
	if( 0xFF == inb(base+7) ) {
		LOG("Floating bus");
		LEAVE('i', 0);
		return 0;
	}
	
	// Check for the controller
	outb(base+0x02, 0x66);
	outb(base+0x03, 0xFF);
	if(inb(base+0x02) != 0x66 || inb(base+0x03) != 0xFF) {
		LOG("No controller");
		LEAVE('i', 0);
		return 0;
	}

	// Send IDENTIFY
	outb(base+7, 0xEC);
	IO_DELAY();
	val = inb(base+7);	// Read status
	LOG("val = 0x%02x", val);
	if(val == 0) {
		LEAVE('i', 0);
		return 0;	// Disk does not exist
	}

	// Poll until BSY clears and DRQ sets or ERR is set
	while( ((val & 0x80) || !(val & 0x08)) && !(val & 1))
		val = inb(base+7);

	// Check for an error
	if(val & 1) {
		LEAVE('i', 0);
		return 0;	// Error occured, so return false
	}

	// Read Data
	for( i = 0; i < 256; i++ )
		data.buf[i] = inw(base);

	// Return the disk size
	if(data.identify.Sectors48 != 0)
		return data.identify.Sectors48;
	else
		return data.identify.Sectors28;
}

/**
 * \fn Uint16 ATA_GetPortBase(int Disk)
 * \brief Returns the base port for a given disk
 */
Uint16 ATA_GetBasePort(int Disk)
{
	switch(Disk)
	{
	case 0:	case 1:		return IDE_PRI_BASE;
	case 2:	case 3:		return IDE_SEC_BASE;
	}
	return 0;
}

/**
 * \fn int ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
 */
int ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;
	Uint8	val;

	ENTER("iDisk XAddress iCount pBuffer", Disk, Address, Count, Buffer);

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS) {
		Warning("Passed too many sectors for a bulk DMA read (%i > %i)",
			Count, MAX_DMA_SECTORS);
		LEAVE('i');
		return 0;
	}

	// Get exclusive access to the disk controller
	LOCK( &giaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;

	// Set up transfer
	if( Address > 0x0FFFFFFF )	// Use LBA48
	{
		outb(base+0x6, 0x40 | (disk << 4));
		outb(base+0x2, 0 >> 8);	// Upper Sector Count
		outb(base+0x3, Address >> 24);	// Low 2 Addr
		outb(base+0x4, Address >> 28);	// Mid 2 Addr
		outb(base+0x5, Address >> 32);	// High 2 Addr
	}
	else
	{
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));	// Magic, Disk, High addr
	}

	outb(base+0x01, 0x01);	//?
	outb(base+0x02, (Uint8) Count);		// Sector Count
	outb(base+0x03, (Uint8) Address);		// Low Addr
	outb(base+0x04, (Uint8) (Address >> 8));	// Middle Addr
	outb(base+0x05, (Uint8) (Address >> 16));	// High Addr

	LOG("Starting Transfer");
	LOG("gATA_PRDTs[%i].Bytes = %i", cont, gATA_PRDTs[cont].Bytes);
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_R48);	// Read Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_R28);	// Read Command (LBA28)

	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 9 );	// Read and start

	// Wait for transfer to complete
	while( gaATA_IRQs[cont] == 0 )	Threads_Yield();

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont << 3, 8 );	// Read and stop

	val = inb(base+0x7);
	LOG("Status byte = 0x%02x", val);

	LOG("gATA_PRDTs[%i].Bytes = %i", cont, gATA_PRDTs[cont].Bytes);
	LOG("Transfer Completed & Acknowledged");

	// Copy to destination buffer
	memcpy( Buffer, gATA_Buffers[cont], Count*SECTOR_SIZE );

	// Release controller lock
	RELEASE( &giaATA_ControllerLock[ cont ] );

	LEAVE('i', 1);
	return 1;
}

/**
 * \fn int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
 * \brief Write up to \a MAX_DMA_SECTORS to a disk
 * \param Disk	Disk ID to write to
 * \param Address	LBA of first sector
 * \param Count	Number of sectors to write (must be >= \a MAX_DMA_SECTORS)
 * \param Buffer	Source buffer for data
 */
int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, const void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS)	return 0;

	// Get exclusive access to the disk controller
	LOCK( &giaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Set up transfer
	outb(base+0x01, 0x00);
	if( Address > 0x0FFFFFFF )	// Use LBA48
	{
		outb(base+0x6, 0x40 | (disk << 4));
		outb(base+0x2, 0 >> 8);	// Upper Sector Count
		outb(base+0x3, Address >> 24);	// Low 2 Addr
		outb(base+0x3, Address >> 28);	// Mid 2 Addr
		outb(base+0x3, Address >> 32);	// High 2 Addr
	}
	else
	{
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));	//Disk,Magic,High addr
	}

	outb(base+0x02, (Uint8) Count);		// Sector Count
	outb(base+0x03, (Uint8) Address);		// Low Addr
	outb(base+0x04, (Uint8) (Address >> 8));	// Middle Addr
	outb(base+0x05, (Uint8) (Address >> 16));	// High Addr
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_W48);	// Write Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_W28);	// Write Command (LBA28)

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;

	// Copy to output buffer
	memcpy( gATA_Buffers[cont], Buffer, Count*SECTOR_SIZE );

	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 1 );	// Write and start

	// Wait for transfer to complete
	while( gaATA_IRQs[cont] == 0 )	Threads_Yield();

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont << 3, 0 );	// Write and stop

	// Release controller lock
	RELEASE( &giaATA_ControllerLock[ cont ] );

	return 1;
}

/**
 * \brief Primary ATA Channel IRQ handler
 */
void ATA_IRQHandlerPri(int UNUSED(IRQ))
{
	Uint8	val;

	// IRQ bit set for Primary Controller
	val = ATA_int_BusMasterReadByte( 0x2 );
	LOG("IRQ val = 0x%x", val);
	if(val & 4) {
		LOG("IRQ hit (val = 0x%x)", val);
		ATA_int_BusMasterWriteByte( 0x2, 4 );
		gaATA_IRQs[0] = 1;
		return ;
	}
}

/**
 * \brief Second ATA Channel IRQ handler
 */
void ATA_IRQHandlerSec(int UNUSED(IRQ))
{
	Uint8	val;
	// IRQ bit set for Secondary Controller
	val = ATA_int_BusMasterReadByte( 0xA );
	LOG("IRQ val = 0x%x", val);
	if(val & 4) {
		LOG("IRQ hit (val = 0x%x)", val);
		ATA_int_BusMasterWriteByte( 0xA, 4 );
		gaATA_IRQs[1] = 1;
		return ;
	}
}

/**
 * \brief Read an 8-bit value from a Bus Master register
 * \param Ofs	Register offset
 */
Uint8 ATA_int_BusMasterReadByte(int Ofs)
{
	if( gATA_BusMasterBase & 1 )
		return inb( (gATA_BusMasterBase & ~1) + Ofs );
	else
		return *(Uint8*)(gATA_BusMasterBasePtr + Ofs);
}

/**
 * \brief Writes a byte to a Bus Master Register
 * \param Ofs	Register Offset
 * \param Value	Value to write
 */
void ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value)
{
	if( gATA_BusMasterBase & 1 )
		outb( (gATA_BusMasterBase & ~1) + Ofs, Value );
	else
		*(Uint8*)(gATA_BusMasterBasePtr + Ofs) = Value;
}

/**
 * \brief Writes a 32-bit value to a Bus Master Register
 * \param Ofs	Register offset
 * \param Value	Value to write
 */
void ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value)
{
	if( gATA_BusMasterBase & 1 )
		outd( (gATA_BusMasterBase & ~1) + Ofs, Value );
	else
		*(Uint32*)(gATA_BusMasterBasePtr + Ofs) = Value;
}

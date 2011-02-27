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

// === Constants ===
#define	IDE_PRI_BASE	0x1F0
#define IDE_PRI_CTRL	0x3F6
#define	IDE_SEC_BASE	0x170
#define IDE_SEC_CTRL	0x376

#define	IDE_PRDT_LAST	0x8000
/**
 \enum HddControls
 \brief Commands to be sent to HDD_CMD
*/
enum HddControls {
	HDD_PIO_R28 = 0x20,
	HDD_PIO_R48 = 0x24,
	HDD_DMA_R48 = 0x25,
	HDD_PIO_W28 = 0x30,
	HDD_PIO_W48 = 0x34,
	HDD_DMA_W48 = 0x35,
	HDD_DMA_R28 = 0xC8,
	HDD_DMA_W28 = 0xCA,
	HDD_IDENTIFY = 0xEC
};

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
	Uint32	Sectors28;	// LBA 28 Sector Count
	Uint16	Unused6[100-62];
	Uint64	Sectors48;	// LBA 48 Sector Count
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
Uint32	ATA_int_BusMasterReadDWord(int Ofs);
void	ATA_int_BusMasterWriteByte(int Ofs, Uint8 Value);
void	ATA_int_BusMasterWriteDWord(int Ofs, Uint32 Value);

// === GLOBALS ===
// - BusMaster IO Addresses
Uint32	gATA_BusMasterBase;	//!< True Address (IO/MMIO)
Uint8	*gATA_BusMasterBasePtr;	//!< Paging Mapped MMIO (If needed)
// - IRQs
 int	gATA_IRQPri = 14;
 int	gATA_IRQSec = 15;
volatile int	gaATA_IRQs[2] = {0};
// - Locks to avoid tripping
tMutex	glaATA_ControllerLock[2];
// - Buffers!
Uint8	gATA_Buffers[2][(MAX_DMA_SECTORS+0xFFF)&~0xFFF] __attribute__ ((section(".padata")));
// - PRDTs
tPRDT_Ent	gATA_PRDTs[2] = {
	{0, 512, IDE_PRDT_LAST},
	{0, 512, IDE_PRDT_LAST}
};
tPAddr	gaATA_PRDT_PAddrs[2];

// === CODE ===
/**
 * \brief Sets up the ATA controller's DMA mode
 */
int ATA_SetupIO(void)
{
	 int	ent;

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
	
	LOG("BAR5 = 0x%x", PCI_GetBAR5( ent ));
	LOG("IRQ = %i", PCI_GetIRQ( ent ));
	
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

	gaATA_PRDT_PAddrs[0] = MM_GetPhysAddr( (tVAddr)&gATA_PRDTs[0] );
	LOG("gaATA_PRDT_PAddrs[0] = 0x%x", gaATA_PRDT_PAddrs[0]);
	ATA_int_BusMasterWriteDWord(4, gaATA_PRDT_PAddrs[0]);
	
	gaATA_PRDT_PAddrs[1] = MM_GetPhysAddr( (tVAddr)&gATA_PRDTs[1] );
	LOG("gaATA_PRDT_PAddrs[1] = 0x%x", gaATA_PRDT_PAddrs[1]);
	ATA_int_BusMasterWriteDWord(12, gaATA_PRDT_PAddrs[1]);

	// Enable controllers
	outb(IDE_PRI_BASE+1, 1);
	outb(IDE_SEC_BASE+1, 1);
	
	// Make sure interrupts are ACKed
	ATA_int_BusMasterWriteByte(2, 0x4);
	ATA_int_BusMasterWriteByte(10, 0x4);

	// return
	LEAVE('i', MODULE_ERR_OK);
	return MODULE_ERR_OK;
}

/**
 * \brief Get the size (in sectors) of a disk
 * \param Disk	Disk to get size of
 * \return Number of sectors reported
 * 
 * Does an ATA IDENTIFY
 */
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
	if(Disk & 1)	// Slave
		outb(base+6, 0xB0);
	else	// Master
		outb(base+6, 0xA0);
	IO_DELAY();
	
	// Check for a floating bus
	if( 0xFF == inb(base+7) ) {
		LOG("Floating bus");
		LEAVE('i', 0);
		return 0;
	}
	
	// Check for the controller
	// - Write to two RW ports and attempt to read back
	outb(base+0x02, 0x66);
	outb(base+0x03, 0xFF);
	if(inb(base+0x02) != 0x66 || inb(base+0x03) != 0xFF) {
		LOG("No controller");
		LEAVE('i', 0);
		return 0;
	}

	// Send ATA IDENTIFY
	outb(base+7, HDD_IDENTIFY);
	IO_DELAY();
	val = inb(base+7);	// Read status
	LOG("val = 0x%02x", val);
	if(val == 0) {
		LEAVE('i', 0);
		return 0;	// Disk does not exist
	}

	// Poll until BSY clears or ERR is set
	// TODO: Timeout?
	while( (val & 0x80) && !(val & 1) )
		val = inb(base+7);
	LOG("BSY unset (0x%x)", val);
	// and, wait for DRQ to set
	while( !(val & 0x08) && !(val & 1))
		val = inb(base+7);
	LOG("DRQ set (0x%x)", val);

	// Check for an error
	if(val & 1) {
		LEAVE('i', 0);
		return 0;	// Error occured, so return false
	}

	// Read Data
	for( i = 0; i < 256; i++ )
		data.buf[i] = inw(base);

	// Return the disk size
	if(data.identify.Sectors48 != 0) {
		LEAVE('X', data.identify.Sectors48);
		return data.identify.Sectors48;
	}
	else {
		LEAVE('x', data.identify.Sectors28);
		return data.identify.Sectors28;
	}
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
 * \return Boolean Failure
 */
int ATA_ReadDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;
	Sint64	timeoutTime;
	Uint8	val;

	ENTER("iDisk XAddress iCount pBuffer", Disk, Address, Count, Buffer);

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS) {
		Log_Warning("ATA", "Passed too many sectors for a bulk DMA read (%i > %i)",
			Count, MAX_DMA_SECTORS);
		LEAVE('i');
		return 1;
	}
	
	// Hack to make debug hexdump noticable
	#if 1
	memset(Buffer, 0xFF, Count*SECTOR_SIZE);
	#endif

	// Get exclusive access to the disk controller
	Mutex_Acquire( &glaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;

	#if 1
	if( cont == 0 ) {
		outb(base+IDE_PRI_CTRL, 4);
		IO_DELAY();
		outb(base+IDE_PRI_CTRL, 0);
	}
	else {
		outb(base+IDE_SEC_CTRL, 4);
		IO_DELAY();
		outb(base+IDE_SEC_CTRL, 0);
	}
	#endif

	// Set up transfer
	if( Address > 0x0FFFFFFF )	// Use LBA48
	{
		outb(base+0x6, 0x40 | (disk << 4));
		IO_DELAY();
		outb(base+0x2, 0 >> 8);	// Upper Sector Count
		outb(base+0x3, Address >> 24);	// Low 2 Addr
		outb(base+0x4, Address >> 28);	// Mid 2 Addr
		outb(base+0x5, Address >> 32);	// High 2 Addr
	}
	else
	{
		// Magic, Disk, High Address nibble
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));
		//outb(base+0x06, 0xA0 | (disk << 4) | ((Address >> 24) & 0x0F));
		IO_DELAY();
	}

	//outb(base+0x01, 0x01);	//?
	outb(base+0x02, Count & 0xFF);		// Sector Count
	outb(base+0x03, Address & 0xFF);		// Low Addr
	outb(base+0x04, (Address >> 8) & 0xFF);	// Middle Addr
	outb(base+0x05, (Address >> 16) & 0xFF);	// High Addr

	LOG("Starting Transfer");
	
	// HACK: Ensure the PRDT is reset
	ATA_int_BusMasterWriteDWord(cont*8+4, gaATA_PRDT_PAddrs[cont]);
		
	LOG("gATA_PRDTs[%i].Bytes = %i", cont, gATA_PRDTs[cont].Bytes);
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_R48);	// Read Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_R28);	// Read Command (LBA28)

	// Start transfer
	ATA_int_BusMasterWriteByte( cont * 8, 9 );	// Read and start

	// Wait for transfer to complete
	timeoutTime = now() + ATA_TIMEOUT;
	while( gaATA_IRQs[cont] == 0 && now() < timeoutTime)
	{
		HALT();
//		Threads_Yield();
	}

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont * 8, 8 );	// Read and stop

	val = inb(base+0x7);
	LOG("Status byte = 0x%02x, Controller Status = 0x%02x",
		val, ATA_int_BusMasterReadByte(cont * 8 + 2));

	if( gaATA_IRQs[cont] == 0 ) {
		
		#if 1
		Debug_HexDump("ATA", Buffer, 512);
		#endif
		
		// Release controller lock
		Mutex_Release( &glaATA_ControllerLock[ cont ] );
		Log_Warning("ATA",
			"Read timeout on disk %i (Reading sector 0x%llx)",
			Disk, Address);
		// Return error
		LEAVE('i', 1);
		return 1;
	}
	else {
		LOG("Transfer Completed & Acknowledged");
		// Copy to destination buffer
		memcpy( Buffer, gATA_Buffers[cont], Count*SECTOR_SIZE );
		// Release controller lock
		Mutex_Release( &glaATA_ControllerLock[ cont ] );

		LEAVE('i', 0);
		return 0;
	}
}

/**
 * \fn int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, void *Buffer)
 * \brief Write up to \a MAX_DMA_SECTORS to a disk
 * \param Disk	Disk ID to write to
 * \param Address	LBA of first sector
 * \param Count	Number of sectors to write (must be >= \a MAX_DMA_SECTORS)
 * \param Buffer	Source buffer for data
 * \return Boolean Failure
 */
int ATA_WriteDMA(Uint8 Disk, Uint64 Address, Uint Count, const void *Buffer)
{
	 int	cont = (Disk>>1)&1;	// Controller ID
	 int	disk = Disk & 1;
	Uint16	base;
	Sint64	timeoutTime;

	// Check if the count is small enough
	if(Count > MAX_DMA_SECTORS)	return 1;

	// Get exclusive access to the disk controller
	Mutex_Acquire( &glaATA_ControllerLock[ cont ] );

	// Set Size
	gATA_PRDTs[ cont ].Bytes = Count * SECTOR_SIZE;

	// Get Port Base
	base = ATA_GetBasePort(Disk);

	// Reset IRQ Flag
	gaATA_IRQs[cont] = 0;
	
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
		// Magic, Disk, High Address nibble
		outb(base+0x06, 0xE0 | (disk << 4) | ((Address >> 24) & 0x0F));
	}

	outb(base+0x02, (Uint8) Count);		// Sector Count
	outb(base+0x03, (Uint8) Address);		// Low Addr
	outb(base+0x04, (Uint8) (Address >> 8));	// Middle Addr
	outb(base+0x05, (Uint8) (Address >> 16));	// High Addr
	if( Address > 0x0FFFFFFF )
		outb(base+0x07, HDD_DMA_W48);	// Write Command (LBA48)
	else
		outb(base+0x07, HDD_DMA_W28);	// Write Command (LBA28)

	// Copy to output buffer
	memcpy( gATA_Buffers[cont], Buffer, Count*SECTOR_SIZE );

	// Start transfer
	ATA_int_BusMasterWriteByte( cont << 3, 1 );	// Write and start

	// Wait for transfer to complete
	timeoutTime = now() + ATA_TIMEOUT;
	while( gaATA_IRQs[cont] == 0 && now() < timeoutTime)
	{
//		Threads_Yield();
		HALT();
	}

	// Complete Transfer
	ATA_int_BusMasterWriteByte( cont << 3, 0 );	// Write and stop

	// If the IRQ is unset, return error
	if( gaATA_IRQs[cont] == 0 ) {
		// Release controller lock
		Mutex_Release( &glaATA_ControllerLock[ cont ] );
		return 1;	// Error
	}
	else {
		Mutex_Release( &glaATA_ControllerLock[ cont ] );
		return 0;
	}
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
 * \brief Read an 32-bit value from a Bus Master register
 * \param Ofs	Register offset
 */
Uint32 ATA_int_BusMasterReadDWord(int Ofs)
{
	if( gATA_BusMasterBase & 1 )
		return ind( (gATA_BusMasterBase & ~1) + Ofs );
	else
		return *(Uint32*)(gATA_BusMasterBasePtr + Ofs);
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

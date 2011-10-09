/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * fdc.c
 * - FDC IO Functions
 */
#define DEBUG	1
#include <acess.h>
#include "common.h"
#include <dma.h>

// === CONSTANTS ===
#define MOTOR_ON_DELAY	500
#define MOTOR_OFF_DELAY	2000

enum eMotorState
{
	MOTOR_OFF,
	MOTOR_ATSPEED,
};

enum eFDC_Registers
{
	FDC_DOR  = 0x02,	// Digital Output Register
	FDC_MSR  = 0x04,	// Master Status Register (Read Only)
	FDC_FIFO = 0x05,	// FIFO port
	FDC_CCR  = 0x07 	// Configuration Control Register (write only)
};

enum eFDC_Commands
{
	CMD_SPECIFY = 3,            // Specify parameters
	CMD_WRITE_DATA = 5,         // Write Data
	CMD_READ_DATA = 6,          // Read Data
	CMD_RECALIBRATE = 7,        // Recalibrate a drive
	CMD_SENSE_INTERRUPT = 8,    // Sense (Ack) an interrupt
	CMD_SEEK = 15,              // Seek to a track
};

// === PROTOTYPES ===
 int	FDD_SetupIO(void);
 int	FDD_int_ReadWriteTrack(int Disk, int Track, int bWrite, void *Buffer);
 int	FDD_int_SeekToTrack(int Disk, int Track);
 int	FDD_int_Calibrate(int Disk);
 int	FDD_int_Reset(int Disk);
// --- FIFO
 int	FDD_int_WriteData(Uint16 Base, Uint8 Data);
 int	FDD_int_ReadData(Uint16 Base, Uint8 *Data);
void	FDD_int_SenseInterrupt(Uint16 Base, Uint8 *ST0, Uint8 *Cyl);
// --- Motor Control
 int	FDD_int_StartMotor(int Disk);
 int	FDD_int_StopMotor(int Disk);
void	FDD_int_StopMotorCallback(void *Ptr);
// --- Helpers
 int	FDD_int_HandleST0Error(const char *Fcn, int Disk, Uint8 ST0);
Uint16	FDD_int_GetBase(int Disk, int *Drive);
// --- Interrupt
void	FDD_int_ClearIRQ(void);
 int	FDD_int_WaitIRQ(void);
void	FDD_int_IRQHandler(int IRQ, void *Ptr);

// === GLOBALS ===
/**
 * \brief Marker for IRQ6
 * \todo Convert into a semaphore?
 */
 int	gbFDD_IRQ6Fired;
/**
 * \brief Protector for DMA and IRQ6
 */
tMutex	gFDD_IOMutex;

// === CODE ===
/**
 * \brief Set up FDC IO
 * \return Boolean failure
 *
 * Registers the IRQ handler and resets the controller
 */
int FDD_SetupIO(void)
{
	// Install IRQ6 Handler
	IRQ_AddHandler(6, FDD_int_IRQHandler, NULL);
	
	// Reset controller
	FDD_int_Reset(0);
	// TODO: All controllers
	
	return 0;
}

/**
 * \brief Read/Write data from/to a disk
 * \param Disk Global disk number
 * \param Track Track number (Cyl*2+Head)
 * \param bWrite Toggle write mode
 * \param Buffer Destination/Source buffer
 * \return Boolean failure
 */
int FDD_int_ReadWriteTrack(int Disk, int Track, int bWrite, void *Buffer)
{
	Uint8	cmd;
	 int	i, _disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);
	 int	cyl = Track >> 1, head = Track & 1;

	ENTER("iDisk iTrack BbWrite pBuffer", Disk, Track, bWrite, Buffer);

	Mutex_Acquire( &gFDD_IOMutex );
	
	// Initialise DMA for read/write
	// TODO: Support non 1.44MiB FDs
	DMA_SetChannel(2, BYTES_PER_TRACK, !bWrite);
	
	// Select command
	if( bWrite )
		cmd = CMD_WRITE_DATA | 0xC0;
	else
		cmd = CMD_READ_DATA | 0xC0;

	LOG("cmd = 0x%x", cmd);
	
	// Seek
	if( FDD_int_SeekToTrack(Disk, Track) ) {
		Mutex_Release( &gFDD_IOMutex );
		LEAVE('i', -1);
		return -1;
	}
	LOG("Track seek done");

	for( i = 0; i < 20; i ++ )
	{
		LOG("Starting motor");
		FDD_int_StartMotor(Disk);

		// Write data
		if( bWrite )
			DMA_WriteData(2, BYTES_PER_TRACK, Buffer);	
	
		LOG("Sending command stream");
		FDD_int_WriteData(base, cmd);
		FDD_int_WriteData(base, (head << 2) | _disk);
		FDD_int_WriteData(base, cyl);
		FDD_int_WriteData(base, head);
		FDD_int_WriteData(base, 1);	// First Sector
		FDD_int_WriteData(base, 2);	// Bytes per sector (128*2^n)
		FDD_int_WriteData(base, 18);	// 18 tracks (full disk) - TODO: Non 1.44
		FDD_int_WriteData(base, 0x1B);	// Gap length - TODO: again
		FDD_int_WriteData(base, 0xFF);	// Data length - ?
	
		LOG("Waiting for IRQ");
		FDD_int_WaitIRQ();
	
		// No Sense Interrupt
		
		LOG("Reading result");
		Uint8	st0=0, st1=0, st2=0, bps=0;
		FDD_int_ReadData(base, &st0);
		FDD_int_ReadData(base, &st1);	// st1
		FDD_int_ReadData(base, &st2);	// st2
		FDD_int_ReadData(base, NULL);	// rcy - Mutilated Cyl
		FDD_int_ReadData(base, NULL);	// rhe - Mutilated Head
		FDD_int_ReadData(base, NULL);	// rse - Mutilated sector
		FDD_int_ReadData(base, &bps);	// bps - Should be the same as above

		if( st0 & 0xc0 ) {
			FDD_int_HandleST0Error(__func__, Disk, st0);
			continue ;
		}
	
		if( st2 & 0x02 ) {
			Log_Debug("FDD", "Disk %i is not writable", Disk);
			Mutex_Release( &gFDD_IOMutex );
			LEAVE('i', 2);
			return 2;
		}
		
		if( st0 & 0x08 ) {
			Log_Debug("FDD", "FDD_int_ReadWriteTrack: Drive not ready");
			continue ;
		}


		if( st1 & 0x80 ) {
			Log_Debug("FDD", "FDD_int_ReadWriteTrack: End of cylinder");
			continue ;
		}

		if( st1 & (0x20|0x10|0x04|0x01) ) {
			Log_Debug("FDD", "FDD_int_ReadWriteTrack: st1 = 0x%x", st1);
			continue;
		}
		
		if( st2 & (0x40|0x20|0x10|0x04|0x01) ) {
			Log_Debug("FDD", "FDD_int_ReadWriteTrack: st2 = 0x%x", st2);
			continue ;
		}
		
		if( bps != 0x2 ) {
			Log_Debug("FDD", "Wanted bps = 2 (512), got %i", bps);
			continue ;
		}

		// Read back data
		if( !bWrite )
			DMA_ReadData(2, BYTES_PER_TRACK, Buffer);
		
		LOG("All data done");
		FDD_int_StopMotor(Disk);
		Mutex_Release( &gFDD_IOMutex );
		LEAVE('i', 0);
		return 0;
	}

	Log_Debug("FDD", "%i retries exhausted", i);
	FDD_int_StopMotor(Disk);
	Mutex_Release( &gFDD_IOMutex );
	LEAVE('i', 1);
	return 1;
}

/**
 * \brief Seek to a specific track
 * \param Disk Global disk number
 * \param Track Track number (Cyl*2+Head)
 * \return Boolean failure
 */
int FDD_int_SeekToTrack(int Disk, int Track)
{
	Uint8	st0=0, res_cyl=0;
	 int	cyl, head;
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);;

	ENTER("iDisk iTrack", Disk, Track);
	
	cyl = Track / 2;
	head = Track % 2;

	LOG("cyl = %i, head = %i", cyl, head);
	
	FDD_int_StartMotor(Disk);
	
	for( int i = 0; i < 10; i ++ )
	{
		LOG("Sending command");
		FDD_int_ClearIRQ();
		FDD_int_WriteData(base, CMD_SEEK);
		FDD_int_WriteData(base, (head << 2) + _disk);
		FDD_int_WriteData(base, cyl);
	
		LOG("Waiting for IRQ");
		FDD_int_WaitIRQ();
		FDD_int_SenseInterrupt(base, &st0, &res_cyl);
	
		if( st0 & 0xC0 )
		{
			FDD_int_HandleST0Error(__func__, Disk, st0);
			continue ;
		}
		
		if( res_cyl == cyl ) {
			FDD_int_StopMotor(Disk);
			LEAVE('i', 0);
			return 0;
		}
	}
	
	Log_Error("FDD", "FDD_int_SeekToTrack: 10 retries exhausted\n");
	FDD_int_StopMotor(Disk);
	LEAVE('i', 1);
	return 1;
}

/**
 * \brief Calibrate a drive
 * \param Disk	Global disk number
 */
int FDD_int_Calibrate(int Disk)
{
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);
	FDD_int_StartMotor(Disk);
	
	for( int i = 0; i < 10; i ++ )
	{
		Uint8	st0=0, cyl = -1;
	
		FDD_int_ClearIRQ();	
		FDD_int_WriteData(base, CMD_RECALIBRATE);
		FDD_int_WriteData(base, _disk);
		
		FDD_int_WaitIRQ();
	
		FDD_int_SenseInterrupt(base, &st0, &cyl);
		
		if( st0 & 0xC0 ) {
			FDD_int_HandleST0Error(__func__, Disk, st0);
			continue ;
		}
		
		if( cyl == 0 )
		{
			FDD_int_StopMotor(Disk);
			return 0;
		}
	}
	
	Log_Error("FDD", "FDD_int_Calibrate: Retries exhausted");
	
	return 1;
}

/**
 * \brief Reset a controller
 * \param Base	Controller base address
 */
int FDD_int_Reset(int Disk)
{
	Uint8	tmp;
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);

	tmp = inb(base + FDC_DOR) & 0xF0;
	outb( base + FDC_DOR, 0x00 );
	Time_Delay(1);
	outb( base + FDC_DOR, tmp | 0x0C );

	FDD_int_SenseInterrupt(base, NULL, NULL);

	outb(base + FDC_CCR, 0x00);	// 500KB/s

	FDD_int_WriteData(base, CMD_SPECIFY);	// Step and Head Load Times
	FDD_int_WriteData(base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	FDD_int_WriteData(base, 0x02);	// Head Load Time >> 1

	// TODO: Recalibrate all present disks
	FDD_int_Calibrate(Disk);
	return 0;
}

/**
 * \brief Write a byte to the FIFO
 */
int FDD_int_WriteData(Uint16 Base, Uint8 Data)
{
	for( int i = 0; i < 100; i ++ )
	{
		if( inb(Base + FDC_MSR) & 0x80 )
		{
			outb(Base + FDC_FIFO, Data);
			return 0;
		}
		Time_Delay(10);
	}
	Log_Error("FDD", "Write timeout");
	return 1;
}

/**
 * \brief Read a byte from the FIFO
 */
int FDD_int_ReadData(Uint16 Base, Uint8 *Data)
{
	for( int i = 0; i < 100; i ++ )
	{
		if( inb(Base + FDC_MSR) & 0x80 )
		{
			Uint8 tmp = inb(Base + FDC_FIFO);
			if(Data) *Data = tmp;
			return 0;
		}
		Time_Delay(10);
	}
	Log_Error("FDD", "Read timeout");
	return 1;
}

/**
 * \brief Acknowledge an interrupt
 * \param Base	Controller base address
 * \param ST0	Location to store the ST0 value
 * \param Cyl	Current cylinder
 */
void FDD_int_SenseInterrupt(Uint16 Base, Uint8 *ST0, Uint8 *Cyl)
{
	FDD_int_WriteData(Base, CMD_SENSE_INTERRUPT);
	FDD_int_ReadData(Base, ST0);
	FDD_int_ReadData(Base, Cyl);
}

/**
 * \brief Start the motor on a disk
 */
int FDD_int_StartMotor(int Disk)
{
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);
	
	// Clear the motor off timer	
	Time_RemoveTimer(gaFDD_Disks[Disk].Timer);
	gaFDD_Disks[Disk].Timer = -1;

	// Check if the motor is already on
	if( gaFDD_Disks[Disk].MotorState == MOTOR_ATSPEED )
		return 0;

	// Turn motor on
	outb(base + FDC_DOR, inb(base+FDC_DOR) | (1 << (_disk + 4)));

	// Wait for it to reach speed
	Time_Delay(MOTOR_ON_DELAY);

	gaFDD_Disks[Disk].MotorState = MOTOR_ATSPEED;

	return 0;
}

/**
 * \brief Schedule the motor to stop
 */
int FDD_int_StopMotor(int Disk)
{
	if( gaFDD_Disks[Disk].MotorState != MOTOR_ATSPEED )
		return 0;
	if( gaFDD_Disks[Disk].Timer != -1 )
		return 0;

	gaFDD_Disks[Disk].Timer = Time_CreateTimer(MOTOR_OFF_DELAY, FDD_int_StopMotorCallback, (void*)(tVAddr)Disk);

	return 0;
}

/**
 * \brief Actually stop the motor
 * \param Ptr	Actaully the global disk number
 */
void FDD_int_StopMotorCallback(void *Ptr)
{
	 int	Disk = (tVAddr)Ptr;
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);

	gaFDD_Disks[Disk].Timer = -1;
	gaFDD_Disks[Disk].MotorState = MOTOR_OFF;
	
	outb(base + FDC_DOR, inb(base+FDC_DOR) & ~(1 << (_disk + 4)));

	return ;
}

/**
 * \brief Converts a global disk number into a controller and drive
 * \param Disk	Global disk number
 * \param Drive	Destination for controller disk number
 * \return Controller base address
 */
Uint16 FDD_int_GetBase(int Disk, int *Drive)
{
	if(Drive)	*Drive = Disk & 3;
	switch(Disk >> 2)
	{
	case 0:	return 0x3F0;
	case 1:	return 0x370;
	default:
		return 0;
	}
}

/**
 * \brief Convert a ST0 error value into a message
 * \param Fcn	Calling function name
 * \parma Disk	Global disk number
 * \param ST0	ST0 Value
 * \return Boolean failure
 */
int FDD_int_HandleST0Error(const char *Fcn, int Disk, Uint8 ST0)
{
	static const char *status_type[] = {
		0, "Error", "Invalid", "Drive Error"
	};

	Log_Debug("FDD", "%s: Disk %i ST0 Status = %s (0x%x & 0xC0 = 0x%x)",
		Fcn, Disk, status_type[ST0 >> 6], ST0, ST0 & 0xC0
		);
	return 0;
}

/**
 * \brief Clear the IRQ fired flag
 */
void FDD_int_ClearIRQ(void)
{
	gbFDD_IRQ6Fired = 0;
}

/**
 * \brief Wait for an IRQ to fire
 */
int FDD_int_WaitIRQ(void)
{
	while(gbFDD_IRQ6Fired == 0)
		Threads_Yield();
	return 0;
}

/**
 * \brief IRQ Handler
 * \param IRQ	IRQ Number (unused)
 * \param Ptr	Data Pointer (unused)
 */
void FDD_int_IRQHandler(int IRQ, void *Ptr)
{
	gbFDD_IRQ6Fired = 1;
}


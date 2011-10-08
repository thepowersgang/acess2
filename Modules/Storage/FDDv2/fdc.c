/*
 * Acess2 82077AA FDC
 * - By John Hodge (thePowersGang)
 *
 * fdc.c
 * - FDC IO Functions
 */
#include <acess.h>
#include "common.h"

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
 int	FDD_int_ReadWriteTrack(int Disk, int Track, int bWrite, void *Buffer);

 int	FDD_int_WriteData(Uint16 Base, Uint8 Data);
 int	FDD_int_ReadData(Uint16 Base, Uint8 *Data);
void	FDD_int_SenseInterrupt(Uint16 Base, Uint8 *ST0, Uint8 *Cyl);
 int	FDD_int_Calibrate(int Disk);
 int	FDD_int_Reset(Uint16 Base);
 int	FDD_int_StartMotor(int Disk);
 int	FDD_int_StopMotor(int Disk);
void	FDD_int_StopMotorCallback(void *Ptr);
Uint16	FDD_int_GetBase(int Disk, int *Drive);
void	FDD_int_ClearIRQ(void);
 int	FDD_int_WaitIRQ(void);
void	FDD_int_IRQHandler(int IRQ, void *Ptr);

// === GLOBALS ===
 int	gbFDD_IRQ6Fired;

// === CODE ===
/**
 * \brief Read/Write data from/to a disk
 * \param Disk Global disk number
 * \param Track Track number
 * \param bWrite Toggle write mode
 * \param Buffer Destination/Source buffer
 * \return Boolean failure
 */
int FDD_int_ReadWriteTrack(int Disk, int Track, int bWrite, void *Buffer)
{
	return -1;
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
	
		FDD_int_SenseInterrupt(base, &st0, NULL);
		
		if( st0 & 0xC0 ) {
			static const char *status_type[] = {
				0, "Error", "Invalid", "Drive Error"
			};
			Log_Debug("FDD", "FDD_int_Calibrate: st0 & 0xC0 = 0x%x, %s",
				st0 & 0xC0, status_type[st0 >> 6]
				);
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
int FDD_int_Reset(Uint16 Base)
{
	Uint8	tmp;
	
	tmp = inb(Base + FDC_DOR) & 0xF0;
	outb( Base + FDC_DOR, 0x00 );
	Time_Delay(1);
	outb( Base + FDC_DOR, tmp | 0x0C );

	FDD_int_SenseInterrupt(Base, NULL, NULL);

	outb(Base + FDC_CCR, 0x00);	// 500KB/s

	FDD_int_WriteData(Base, CMD_SPECIFY);	// Step and Head Load Times
	FDD_int_WriteData(Base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	FDD_int_WriteData(Base, 0x02);	// Head Load Time >> 1

	// TODO: Recalibrate all present disks
	FDD_int_Calibrate(0);
	return 0;
}

/**
 * \brief Start the motor on a disk
 */
int FDD_int_StartMotor(int Disk)
{
	 int	_disk;
	Uint16	base = FDD_int_GetBase(Disk, &_disk);
	
	if( gaFDD_Disks[Disk].MotorState == MOTOR_ATSPEED )
		return 0;

	// Clear the motor off timer	
	Time_RemoveTimer(gaFDD_Disks[Disk].Timer);
	gaFDD_Disks[Disk].Timer = -1;

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


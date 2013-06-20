/*
 * Acess2 Kernel - AHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * sata.h
 * - SATA Structures
 */
#ifndef _AHCI__SATA_H_
#define _AHCI__SATA_H_

#include "ata.h"

enum eSATA_FIS_Types
{
	SATA_FIS_H2DRegister = 0x27,
	SATA_FIS_D2HRegister = 0x34,
	SATA_FIS_DMASetup = 0x41,
	SATA_FIS_Data = 0x46,
	SATA_FIS_PIOSetup = 0x5F,
};

struct sSATA_FIS_DMASetup
{
	Uint8	Type;	// = 0x41
	Uint8	Flags;	// [6]: Interrupt, [5]: Direction
	Uint8	_resvd1[2];
	Uint32	DMABufIdLow;
	Uint32	DMABufIdHigh;
	Uint32	_resvd2[1];
	Uint32	DMABufOfs;
	Uint32	DMATransferCount;
	Uint32	_resvd[1];
} PACKED;
struct sSATA_FIS_Data
{
	Uint8	Type;	// = 0x46
	Uint8	_resvd[3];
	Uint32	Data[1];
} PACKED;
struct sSATA_FIS_PIOSetup
{
	Uint8	Type;	// = 0x5F
	Uint8	Flags;
	Uint8	Status;
	Uint8	Error;
	Uint8	SectorNum;
	Uint8	CylLow;
	Uint8	CylHigh;
	Uint8	Dev_Head;
	Uint8	SectorNumExp;
	Uint8	CylLowExp;
	Uint8	CylHighExp;
	Uint8	_resvd1[1];
	Uint8	SectorCount;
	Uint8	SectorCountExp;
	Uint8	_resvd2[1];
	Uint8	E_Status;
	Uint16	TransferCount;
	Uint8	_resvd3[2];
} PACKED;
struct sSATA_FIS_H2DRegister
{
	Uint8	Type;	// = 0x27
	Uint8	Flags;	// [7]: Update to command register
	Uint8	Command;
	Uint8	Features;
	Uint8	SectorNum;
	Uint8	CylLow;
	Uint8	CylHigh;
	Uint8	Dev_Head;
	Uint8	SectorNumExp;
	Uint8	CylLowExp;
	Uint8	CylHighExp;
	Uint8	FeaturesExp;
	Uint8	SectorCount;
	Uint8	SectorCountExp;
	Uint8	_resvd1;
	Uint8	Control;
	Uint8	_resvd2[4];
} PACKED;
struct sSATA_FIS_D2HRegister
{
	Uint8	Type;	// = 0x34
	Uint8	IntResvd;	// [6]: Interrupt bit
	Uint8	Status;
	Uint8	Error;
	Uint8	SectorNum;
	Uint8	CylLow;
	Uint8	CylHigh;
	Uint8	Dev_Head;
	Uint8	SectorNumExp;
	Uint8	CylLowExp;
	Uint8	CylHighExp;
	Uint8	_resvd1[1];
	Uint8	SectorCount;
	Uint8	SectorCountExp;
	Uint8	_resvd[6];
} PACKED;
struct sSATA_FIS_SDB
{
	Uint8	Type;	// = 0xA1
	Uint8	IntResvd;
	Uint8	Status;
	Uint8	Error;
	Uint32	_resvd[1];
} PACKED;

#endif


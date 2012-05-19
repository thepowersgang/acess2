/*
 * Acess2 USB Stack Mass Storage Driver
 * - By John Hodge (thePowersGang)
 *
 * scsi.h
 * - "SCSI Transparent Command Set" handling code
 */
#ifndef _MSC__SCSI_H_
#define _MSC__SCSI_H_

// NOTE: All commands are big-endian

struct sSCSI_Cmd_ReadCapacity16
{
	Uint8	Op;	// 0x9E
	Uint8	Svc;	// 0x10
	Uint64	LBA;	//
	Uint32	AllocationLength;
	Uint8	Flags;
	Uint8	Control;
} PACKED;

struct sSCSI_Ret_ReadCapacity16
{
	Uint64	BlockCount;
	Uint32	BlockSize;
	Uint8	Flags;
	Uint8	_resvd[32-13];
} PACKED;

struct sSCSI_Cmd_Read16
{
	Uint8	Op;	// 0x88
	Uint8	Flags;
	Uint64	LBA;
	Uint32	Length;
	Uint8	GroupNumber;
	Uint8	Control;
} PACKED;

#endif


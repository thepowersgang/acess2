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
enum eSCSI_Ops
{
	READ_CAPACITY_10 = 0x25,
	SERVICE_ACTION_IN_16 = 0x9E
};

struct sSCSI_Cmd_RequestSense
{
	Uint8	Op;	// 0x03
	Uint8	Desc;
	Uint16	_resvd;
	Uint8	AllocationLength;
	Uint8	Control;
} PACKED;

struct sSCSI_SenseData
{
	Uint8	ResponseCode;
	Uint8	_obselete;
	Uint8	Flags;
	Uint32	Information;
	Uint8	AdditionalSenseLength;
	Uint32	CommandSpecInfomation;
	Uint8	AdditionalSenseCode;
	Uint8	AdditionalSenseCodeQual;
	Uint8	FRUC;
	//Uint
} PACKED;

struct sSCSI_Cmd_ReadCapacity10
{
	Uint8	Op;	// 0x25
	Uint8	_resvd1;
	Uint32	LBA;
	Uint16	_resvd2;
	Uint8	Flags;
	Uint8	Control;
} PACKED;

struct sSCSI_Cmd_ReadCapacity16
{
	Uint8	Op;	// 0x9E
	Uint8	Svc;	// 0x10
	Uint64	LBA;	//
	Uint32	AllocationLength;
	Uint8	Flags;
	Uint8	Control;
} PACKED;

struct sSCSI_Ret_ReadCapacity10
{
	Uint32	LastBlock;
	Uint32	BlockSize;
} PACKED;

struct sSCSI_Ret_ReadCapacity16
{
	Uint64	LastBlock;
	Uint32	BlockSize;
	Uint8	Flags;
	Uint8	_resvd[32-13];
} PACKED;

struct sSCSI_Cmd_Read6
{
	Uint8	Op;	// 0x08
	Uint8	LBATop;
	Uint16	LBALower;
	Uint8	Length;
	Uint8	Control;
} PACKED;

struct sSCSI_Cmd_Read10
{
	Uint8	Op;	// 0x28
	Uint8	Flags;
	Uint32	LBA;
	Uint8	GroupNumber;
	Uint16	Length;
	Uint8	Control;
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


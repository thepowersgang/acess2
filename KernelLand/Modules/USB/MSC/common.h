/*
 * Acess2 USB Stack Mass Storage Driver
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Common header
 */
#ifndef _MSC__COMMON_H_
#define _MSC__COMMON_H_

#include <usb_core.h>
#include <Storage/LVM/include/lvm.h>

typedef struct sMSCInfo	tMSCInfo;

struct sMSCInfo
{
	Uint64	BlockCount;
	size_t	BlockSize;
};

extern void	MSC_SendData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, const void *Data);
extern void	MSC_RecvData(tUSBInterface *Dev, size_t CmdLen, const void *CmdData, size_t DataLen, void *Data);

extern tLVM_VolType	gMSC_SCSI_VolType;
extern Uint64 MSC_SCSI_GetSize(tUSBInterface *Dev, size_t *BlockSize);
#endif


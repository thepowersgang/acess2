/*
 * Acess2 USB Stack Mass Storage Driver
 * - By John Hodge (thePowersGang)
 *
 * scsi.c
 * - "SCSI Transparent Command Set" handling code
 */
#define DEBUG	0
#include "common.h"
#include "scsi.h"

// === PROTOTYPES ===
Uint64	MSC_SCSI_GetSize(tUSBInterface *Dev, size_t *BlockSize);
int	MSC_SCSI_ReadData(void *Ptr, Uint64 Block, size_t Count, void *Dest);
int	MSC_SCSI_WriteData(void *Ptr, Uint64 Block, size_t Count, const void *Dest);

// === GLOBALS ===
tLVM_VolType	gMSC_SCSI_VolType = {
	.Name = "USB-MSC-SCSI",
	.Read = MSC_SCSI_ReadData,
	.Write = MSC_SCSI_WriteData
	};

// === CODE ===
Uint64 MSC_SCSI_GetSize(tUSBInterface *Dev, size_t *BlockSize)
{
	struct sSCSI_Cmd_ReadCapacity16	cmd;
	struct sSCSI_Ret_ReadCapacity16	ret;
	
	cmd.Op   = 0x9E;
	cmd.Svc  = 0x10;
	cmd.LBA  = BigEndian64( 0 );
	cmd.AllocationLength = BigEndian32(sizeof(ret));
	cmd.Flags   = 0;
	cmd.Control = 0;

	ret.BlockSize = 0;
	ret.BlockCount = 0;	
	MSC_RecvData(Dev, sizeof(cmd), &cmd, sizeof(ret), &ret);
	
	*BlockSize = BigEndian32(ret.BlockSize);
	return BigEndian64(ret.BlockCount);
}

int MSC_SCSI_ReadData(void *Ptr, Uint64 Block, size_t Count, void *Dest)
{
	tUSBInterface	*Dev = Ptr;
	tMSCInfo	*info = USB_GetDeviceDataPtr(Dev);
	struct sSCSI_Cmd_Read16	cmd;

	// TODO: Bounds checking?
	
	cmd.Op     = 0x88;
	cmd.Flags  = 0;
	cmd.LBA    = BigEndian64(Block);
	cmd.Length = BigEndian32(Count);
	cmd.GroupNumber = 0;
	cmd.Control = 0;
	
	MSC_RecvData(Dev, sizeof(cmd), &cmd, Count*info->BlockSize, Dest);
	// TODO: Error check	

	return Count;
}

int MSC_SCSI_WriteData(void *Ptr, Uint64 Block, size_t Count, const void *Data)
{
	Log_Warning("MSC_SCSI", "TODO: Impliment MSC_SCSI_WriteData");
	return 0;
}


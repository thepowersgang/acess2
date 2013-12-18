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
void	MSC_SCSI_DumpSenseData(tUSBInterface *Dev);
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
	// 1. Attempt a 10 op
	{
		struct sSCSI_Cmd_ReadCapacity10	cmd = {READ_CAPACITY_10,0,0,0,0,0};
		struct sSCSI_Ret_ReadCapacity10	ret = {0,0};
		
		if( MSC_RecvData(Dev, sizeof(cmd), &cmd, sizeof(ret), &ret) != 0 ) {
			// Oops?
			Log_Warning("MSC SCSI", "Command \"Read Capacity (10)\" failed");
			MSC_SCSI_DumpSenseData(Dev);
			return 0;
		}
		
		if( ret.LastBlock != 0xFFFFFFFF )
		{
			*BlockSize = BigEndian32(ret.BlockSize);
			return BigEndian32(ret.LastBlock)+1;
		}
		
		// Size was too large for 32-bits, attempt a 16-byte op
	}

	// 2. Attempt a 16 byte op
	{
		struct sSCSI_Cmd_ReadCapacity16	cmd;
		struct sSCSI_Ret_ReadCapacity16	ret;
		
		cmd.Op   = SERVICE_ACTION_IN_16;
		cmd.Svc  = 0x10;
		cmd.LBA  = BigEndian64( 0 );
		cmd.AllocationLength = BigEndian32(sizeof(ret));
		cmd.Flags   = 0;
		cmd.Control = 0;

		ret.BlockSize = 0;
		ret.LastBlock = 0;
		if( MSC_RecvData(Dev, sizeof(cmd), &cmd, sizeof(ret), &ret) != 0 ) {
			// Oops?
			Log_Warning("MSC SCSI", "Command \"Read Capacity (16)\" failed");
			return 0;
		}
		
		*BlockSize = BigEndian32(ret.BlockSize);
		return BigEndian64(ret.LastBlock)+1;
	}
}

void MSC_SCSI_DumpSenseData(tUSBInterface *Dev)
{
	struct sSCSI_SenseData	ret;
	struct sSCSI_Cmd_RequestSense cmd = {0x03, 0, 0, sizeof(ret), 0};
	
	int rv = MSC_RecvData(Dev, sizeof(cmd), &cmd, sizeof(ret), &ret);
	if( rv != 0 ) {
		// ... oh
		Log_Warning("MSC SCSI", "Command \"Request Sense\" failed, giving up");
		return ;
	}
	
	const char *const sense_keys[16] = {
		"No Sense", "Recovered Error", "Not Ready", "Medium Error",
		"Hardware Error", "Illegal Request", "Unit Attention", "Data Protect",
		"Blank Check", "Vendor Specific", "Copy Aborted", "Aborted Command",
		"-obs-", "Volume Overflow", "Miscompare", "-rsvd-"
	};
	Log_Debug("MSC SCSI", "Dumping Sense Data:");
	Log_Debug("MSC SCSI", ".ResponseCode = %02x", ret.ResponseCode);
	Log_Debug("MSC SCSI", ".Flags = %02x (%s%s%s%s)", ret.Flags,
			(ret.Flags & (1<<7) ? "FILEMARK,":""),
			(ret.Flags & (1<<6) ? "EOM,":""),
			(ret.Flags & (1<<6) ? "ILI,":""),
			sense_keys[ret.Flags & 15]
			);
	Log_Debug("MSC SCSI", ".Information = %08x", BigEndian32(ret.Information));
	Log_Debug("MSC SCSI", ".AdditionalLen = %02x", ret.AdditionalSenseLength);
	Log_Debug("MSC SCSI", ".CommandSpecInfomation = %08x", BigEndian32(ret.CommandSpecInfomation));
	Log_Debug("MSC SCSI", ".ASC  = %02x", ret.AdditionalSenseCode);
	Log_Debug("MSC SCSI", ".ASCQ = %02x", ret.AdditionalSenseCodeQual);
}

int MSC_SCSI_ReadData(void *Ptr, Uint64 Block, size_t Count, void *Dest)
{
	tUSBInterface	*Dev = Ptr;
	tMSCInfo	*info = USB_GetDeviceDataPtr(Dev);
	
	// Error if more than 64 thousand blocks are read at once
	if( Count > (1UL<<16) ) {
		// What the fsck are you smoking?
		Log_Error("MSC SCSI", "Attempt to read >16-bits worth of blocks");
		return 0;
	}
	
	// TODO: Bounds checking?

	 int	ret;
	if( Block < (1UL << 20) && Count <= (1UL << 8) )
	{
		// Read (6)
		struct sSCSI_Cmd_Read6 cmd;
		cmd.Op       = 0x08;
		cmd.LBATop   = Block >> 16;
		cmd.LBALower = BigEndian16( Block & 0xFFFF );
		cmd.Length   = Count & 0xFF;	// 0 == 256
		cmd.Control  = 0;
		
		ret = MSC_RecvData(Dev, sizeof(cmd), &cmd, Count*info->BlockSize, Dest);
	}
	else if( Block <= 0xFFFFFFFF )
	{
		// Read (10)
		struct sSCSI_Cmd_Read10 cmd;
		cmd.Op      = 0x28;
		cmd.Flags   = 0;
		cmd.LBA     = BigEndian32( Block );
		cmd.GroupNumber = 0;
		cmd.Length  = BigEndian16( Count );	// 0 == no blocks
		cmd.Control = 0;
		
		ret = MSC_RecvData(Dev, sizeof(cmd), &cmd, Count*info->BlockSize, Dest);
	}
	else
	{
		// Read (16)
		struct sSCSI_Cmd_Read16	cmd;
		cmd.Op     = 0x88;
		cmd.Flags  = 0;
		cmd.LBA    = BigEndian64(Block);
		cmd.Length = BigEndian32(Count);
		cmd.GroupNumber = 0;
		cmd.Control = 0;
		
		ret = MSC_RecvData(Dev, sizeof(cmd), &cmd, Count*info->BlockSize, Dest);
		// TODO: Error check
	}
	
	if( ret != 0 )
	{
		// TODO: Get read size and find out how much was read
		return 0;
	}
	
	return Count;
}

int MSC_SCSI_WriteData(void *Ptr, Uint64 Block, size_t Count, const void *Data)
{
	Log_Warning("MSC_SCSI", "TODO: Implement MSC_SCSI_WriteData");
	return 0;
}


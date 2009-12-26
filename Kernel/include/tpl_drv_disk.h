/**
 * \file tpl_drv_disk.h
 * \brief Disk Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section dirs VFS Layout
 * Disk drivers have a flexible directory layout. The root directory can
 * contain subdirectories, with the only conditions being that all nodes
 * must support ::eTplDrv_IOCtl with DRV_IOCTL_TYPE returning DRV_TYPE_DISK.
 * And all file nodes representing disk devices (or partitions) and implemeting
 * ::eTplDisk_IOCtl fully
 * 
 * \section files Files
 * When a read or write occurs on a normal file in the disk driver it will
 * read/write the represented device. The accesses need not be aligned to
 * the block size, however aligned reads/writes should be handled specially
 * to improve speed (this can be aided by using ::DrvUtil_ReadBlock and
 * ::DrvUtil_WriteBlock)
 */
#ifndef _TPL_DRV_DISK_H
#define _TPL_DRV_DISK_H

#include <tpl_drv_common.h>

/**
 * \enum eTplDisk_IOCtl
 * \brief Common Disk IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplDisk_IOCtl {
	/**
	 * ioctl(..., void)
	 * \brief Get the block size
	 * \return Size of a hardware block for this device
	 */
	DISK_IOCTL_GETBLOCKSIZE = 4
};

/**
 * \brief IOCtl name strings
 */
#define	DRV_DISK_IOCTLNAMES	"get_block_size"

/**
 * \brief Callback function type used by DrvUtil_ReadBlock and DrvUtil_WriteBlock
 * \param Address	Zero based block number to read
 * \param Count	Number of blocks to read
 * \param Buffer	Destination for read blocks
 * \param Argument	Argument provided in ::DrvUtil_ReadBlock and ::DrvUtil_WriteBlock
 */
typedef Uint	(*tDrvUtil_Callback)(Uint64 Address, Uint Count, void *Buffer, Uint Argument);

/**
 * \brief Reads a range from a block device using aligned reads
 * \param Start	Base byte offset
 * \param Length	Number of bytes to read
 * \param Buffer	Destination for read data
 * \param ReadBlocks	Callback function to read a sequence of blocks
 * \param BlockSize	Size of an individual block
 * \param Argument	An argument to pass to \a ReadBlocks
 * \return Number of bytes read
 */
extern Uint64 DrvUtil_ReadBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, Uint64 BlockSize, Uint Argument);
/**
 * \brief Writes a range to a block device using aligned writes
 * \param Start	Base byte offset
 * \param Length	Number of bytes to write
 * \param Buffer	Destination for read data
 * \param ReadBlocks	Callback function to read a sequence of blocks
 * \param WriteBlocks	Callback function to write a sequence of blocks
 * \param BlockSize	Size of an individual block
 * \param Argument	An argument to pass to \a ReadBlocks and \a WriteBlocks
 * \return Number of bytes written
 */
extern Uint64 DrvUtil_WriteBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, tDrvUtil_Callback WriteBlocks,
	Uint64 BlockSize, Uint Argument);

#endif

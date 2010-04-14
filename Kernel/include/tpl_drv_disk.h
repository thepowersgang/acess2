/**
 * \file tpl_drv_disk.h
 * \brief Disk Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section Nomeclature
 * All addreses are 64-bit counts of bytes from the logical beginning of
 * the disk unless explicitly stated.
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
	DISK_IOCTL_GETBLOCKSIZE = 4,
	
	/**
	 * ioctl(..., tTplDisk_CacheRegion *RegionInfo)
	 * \brief Sets the cache importantce and protocol for a section of
	 *        memory.
	 * \param RegionInfo	Pointer to a region information structure
	 * \return Boolean failure
	 */
	DISK_IOCTL_SETCACHEREGION,
	
	/**
	 * ioctl(..., Uint64 *Info[2])
	 * \brief Asks the driver to precache a region of disk.
	 * \param Region	64-bit Address and Size pair describing the area to cache
	 * \return Number of blocks cached
	 */
	DISK_IOCTL_PRECACHE,
	
	/**
	 * ioclt(..., Uint64 *Region[2])
	 * \brief Asks to driver to flush the region back to disk
	 * \param Region	64-bit Address and Size pair describing the area to flush
	 * \note If Region[0] == -1 then the entire disk's cache is flushed
	 * \return Number of blocks flushed (or 0 for entire disk)
	 */
	DISK_IOCTL_FLUSH
};

/**
 * \brief Describes the cache parameters of a region on the disk
 */
typedef struct sTplDisk_CacheRegion
{
	Uint64	Base;	//!< Base of cache region
	Uint64	Length;	//!< Size of cache region
	/**
	 * \brief Cache Protocol & Flags
	 * 
	 * The low 4 bits denot the cache protocol to be used by the
	 * region (see ::eTplDisk_CacheProtocols for a list).
	 * The high 4 bits denote flags to apply to the cache (see
	 * ::eTplDisk_CacheFlags)
	 */
	Uint8	Flags;
	Uint8	Priority;	//!< Lower is a higher proritory
	/**
	 * \brief Maximum size of cache, in blocks
	 * \note If CacheSize is zero, the implemenation defined limit is used
	 */
	Uint16	CacheSize;
}	tTplDisk_CacheRegion;

/**
 * \brief Cache protocols to use
 */
enum eTplDisk_CacheProtocols
{
	/**
	 * \brief Don't cache the region
	 */
	
	DISK_CACHEPROTO_DONTCACHE,
	/**
	 * \brief Most recently used blocks cached
	 * \note This is the default action for undefined regions
	 */
	DISK_CACHEPROTO_RECENTLYUSED,
	/**
	 * \brief Cache the entire region in memory
	 * 
	 * This is a faster version of setting Length to CacheSize*BlockSize
	 */
	DISK_CACHEPROTO_FULLCACHE,
	
	/**
	 * \brief Cache only on demand
	 * 
	 * Only cache when the ::DISK_IOCTL_PRECACHE IOCtl is used
	 */
	DISK_CACHEPROTO_EXPLICIT
};

/**
 * \brief Flags for the cache
 */
enum eTplDisk_CacheFlags
{
	/**
	 * \brief Write all changes to the region straight back to media
	 */
	DISK_CACHEFLAG_WRITETHROUGH = 0x10
};

/**
 * \brief IOCtl name strings
 */
#define	DRV_DISK_IOCTLNAMES	"get_block_size","set_cache_region","set_precache"

/**
 * \name Disk Driver Utilities
 * \{
 */

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

/**
 * \}
 */

#endif

/*
 * Acess2 Kernel
 * - IO Cache
 * 
 * By thePowersGang (John Hodge)
 */
/**
 * \file iocache.h
 * \brief I/O Caching Helper Subsystem
 * 
 * The IO Cache abstracts caching of disk sectors away from the device
 * driver to reduce code duplication and allow a central location for
 * disk cache management that can be flushed simply by the kernel, without
 * having to ask each driver to do it indivitually.
 */
#ifndef _IOCHACHE_H_
#define _IOCHACHE_H_

// === TYPES ===
/**
 * \brief IO Cache Handle
 */
typedef struct sIOCache	tIOCache;
/**
 * \brief Write Callback
 * 
 * Called to write a sector back to the device
 */
typedef int	(*tIOCache_WriteCallback)(void *ID, Uint64 Sector, const void *Buffer);

// === CONSTANTS ===
/**
 * \brief I/O Cache handling modes
 */
enum eIOCache_Modess {
	/**
	 * \brief Writeback
	 * 
	 * Transparently writes data straight to the device when the cache
	 * is written to.
	 */
	IOCACHE_WRITEBACK,
	/**
	 * \brief Delay Write
	 * 
	 * Only writes when instructed to (by ::IOCache_Flush) or when a
	 * cached sector is being reallocated.
	 */
	IOCACHE_DELAYWRITE,
	/**
	 * \brief Virtual - No Writes
	 * 
	 * Changes to the cache contents are only reflected in memory,
	 * any calls to ::IOCache_Flush will silently return without doing
	 * anything and if a sector is reallocated, all changes will be lost
	 */
	IOCACHE_VIRTUAL
};

// === FUNCTIONS ===
/**
 * \brief Creates a new IO Cache
 * \param Write	Function to call to write a sector to the device
 * \param ID	ID to pass to \a Write
 * \param SectorSize	Size of a cached sector
 * \param CacheSize	Maximum number of objects that can be in the cache at one time
 */
extern tIOCache	*IOCache_Create( tIOCache_WriteCallback Write, void *ID, int SectorSize, int CacheSize );

/**
 * \brief Reads from a cached sector
 * \param Cache	Cache handle returned by ::IOCache_Create
 * \param Sector	Sector's ID number
 * \param Buffer	Destination for the data read
 * \return	1 if the data was read, 0 if the sector is not cached, -1 on error
 */
extern int	IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer );

/**
 * \brief Adds a sector to the cache
 * \param Cache	Cache handle returned by ::IOCache_Create
 * \param Sector	Sector's ID number
 * \param Buffer	Data to cache
 * \return	1 on success, 0 if the sector is already cached, -1 on error
 */
extern int	IOCache_Add( tIOCache *Cache, Uint64 Sector, const void *Buffer );

/**
 * \brief Writes to a cached sector
 * \param Cache	Cache handle returned by ::IOCache_Create
 * \param Sector	Sector's ID number
 * \param Buffer	Data to write to the cache
 * \return	1 if the data was read, 0 if the sector is not cached, -1 on error
 * 
 * If the sector is in the cache, it is updated.
 * Wether the Write callback is called depends on the selected caching
 * behaviour.
 */
extern int	IOCache_Write( tIOCache *Cache, Uint64 Sector, const void *Buffer );

/**
 * \brief Flush altered sectors out to the device
 * \param Cache	Cache handle returned by ::IOCache_Create
 * 
 * This will call the cache's write callback on each altered sector
 * to flush the write cache.
 */
extern void	IOCache_Flush( tIOCache *Cache );

/**
 * \brief Flushes the cache and then removes it
 * \param Cache	Cache handle returned by ::IOCache_Create
 * \note After this is called \a Cache is no longer valid
 * 
 * IOCache_Destroy writes all changed sectors to the device and then
 * deallocates the cache for other systems to use.
 */
extern void	IOCache_Destroy( tIOCache *Cache );

#endif

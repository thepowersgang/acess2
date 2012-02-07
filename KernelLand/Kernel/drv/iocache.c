/*
 * Acess2 Kernel
 * - IO Cache
 * 
 * By thePowersGang (John Hodge)
 * 
 * TODO: Convert to use spare physical pages instead
 */
#define DEBUG	0
#include <acess.h>
#include <iocache.h>

// === TYPES ===
typedef struct sIOCache_Ent	tIOCache_Ent;
typedef struct sIOCache_PageInfo	tIOCache_PageInfo;

// === STRUCTURES ===
struct sIOCache_Ent
{
	tIOCache_Ent	*Next;
	Uint64	Num;
	Sint64	LastAccess;
	Sint64	LastWrite;
	Uint8	Data[];
};

struct sIOCache_PageInfo
{
	tIOCache_PageInfo	*GlobalNext;
	tIOCache_PageInfo	*CacheNext;
	tIOCache	*Owner;
	tPAddr	BasePhys;
	Uint64	BaseOffset;
};

struct sIOCache
{
	tIOCache	*Next;
	 int	SectorSize;
	tMutex	Lock;
	 int	Mode;
	Uint32	ID;
	tIOCache_WriteCallback	Write;
	 int	CacheSize;
	 int	CacheUsed;
	tIOCache_Ent	*Entries;
};

// === GLOBALS ===
tShortSpinlock	glIOCache_Caches;
tIOCache	*gIOCache_Caches = NULL;
 int	giIOCache_NumCaches = 0;
tIOCache_PageInfo	*gIOCache_GlobalPages;

// === CODE ===
/**
 * \fn tIOCache *IOCache_Create( tIOCache_WriteCallback Write, Uint32 ID, int SectorSize, int CacheSize )
 * \brief Creates a new IO Cache
 */
tIOCache *IOCache_Create( tIOCache_WriteCallback Write, Uint32 ID, int SectorSize, int CacheSize )
{
	tIOCache	*ret = calloc( 1, sizeof(tIOCache) );
	
	// Sanity Check
	if(!ret)	return NULL;
	
	// Fill Structure
	ret->SectorSize = SectorSize;
	ret->Mode = IOCACHE_WRITEBACK;
	ret->ID = ID;
	ret->Write = Write;
	ret->CacheSize = CacheSize;
	
	// Append to list
	SHORTLOCK( &glIOCache_Caches );
	ret->Next = gIOCache_Caches;
	gIOCache_Caches = ret;
	SHORTREL( &glIOCache_Caches );
	
	// Return
	return ret;
}

/**
 * \fn int IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer )
 * \brief Read from a cached sector
 */
int IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer )
{
	
	ENTER("pCache XSector pBuffer", Cache, Sector, Buffer);
	
	// Sanity Check!
	if(!Cache || !Buffer) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Lock
	Mutex_Acquire( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		Mutex_Release( &Cache->Lock );
		LEAVE('i', -1);
		return -1;
	}

	#if IOCACHE_USE_PAGES
	tIOCache_PageInfo	*page;
	size_t	offset = (Sector*Cache->SectorSize) % PAGE_SIZE;
	Uint64	wanted_base = (Sector*Cache->SectorSize) & ~(PAGE_SIZE-1);
	for( page = Cache->Pages; page; page = page->CacheNext )
	{
		void	*tmp;
		if(page->BaseOffset < WantedBase)	continue;
		if(page->BaseOffset > WantedBase)	break;
		tmp = MM_MapTemp( page->BasePhys );
		memcpy( Buffer, tmp + offset, Cache->SectorSize ); 
		MM_FreeTemp( tmp );
	}
	#else	
	tIOCache_Ent	*ent;
	// Search the list
	for( ent = Cache->Entries; ent; ent = ent->Next )
	{
		// Have we found what we are looking for?
		if( ent->Num == Sector ) {
			memcpy(Buffer, ent->Data, Cache->SectorSize);
			ent->LastAccess = now();
			Mutex_Release( &Cache->Lock );
			LEAVE('i', 1);
			return 1;
		}
		// It's a sorted list, so as soon as we go past `Sector` we know
		// it's not there
		if(ent->Num > Sector)	break;
	}
	#endif
	
	Mutex_Release( &Cache->Lock );
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn int IOCache_Add( tIOCache *Cache, Uint64 Sector, void *Buffer )
 * \brief Cache a sector
 */
int IOCache_Add( tIOCache *Cache, Uint64 Sector, void *Buffer )
{
	tIOCache_Ent	*ent, *prev;
	tIOCache_Ent	*new;
	tIOCache_Ent	*oldest = NULL, *oldestPrev;
	
	// Sanity Check!
	if(!Cache || !Buffer)
		return -1;
	
	// Lock
	Mutex_Acquire( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		Mutex_Release( &Cache->Lock );
		return -1;
	}
	
	// Search the list
	prev = (tIOCache_Ent*)&Cache->Entries;
	for( ent = Cache->Entries; ent; prev = ent, ent = ent->Next )
	{
		// Is it already here?
		if( ent->Num == Sector ) {
			Mutex_Release( &Cache->Lock );
			return 0;
		}
		
		// Check if we have found the oldest entry
		if( !oldest || oldest->LastAccess > ent->LastAccess ) {
			oldest = ent;
			oldestPrev = prev;
		}
		
		// Here we go!
		if(ent->Num > Sector)
			break;
	}
	
	// Create the new entry
	new = malloc( sizeof(tIOCache_Ent) + Cache->SectorSize );
	new->Next = ent;
	new->Num = Sector;
	new->LastAccess = now();
	new->LastWrite = 0;	// Zero is special, it means unmodified
	memcpy(new->Data, Buffer, Cache->SectorSize);
	
	// Have we reached the maximum cached entries?
	if( Cache->CacheUsed == Cache->CacheSize )
	{
		tIOCache_Ent	*savedPrev = prev;
		oldestPrev = (tIOCache_Ent*)&Cache->Entries;
		// If so, search for the least recently accessed entry
		for( ; ent; prev = ent, ent = ent->Next )
		{	
			// Check if we have found the oldest entry
			if( !oldest || oldest->LastAccess > ent->LastAccess ) {
				oldest = ent;
				oldestPrev = prev;
			}
		}
		if( !oldest ) {
			Log_Error("IOCache", "Cache full, but also empty");
			return -1;
		}
		// Remove from list, write back and free
		oldestPrev->Next = oldest->Next;
		if(oldest->LastWrite && Cache->Mode != IOCACHE_VIRTUAL)
			Cache->Write(Cache->ID, oldest->Num, oldest->Data);
		free(oldest);
		
		// Decrement the used count
		Cache->CacheUsed --;
		
		// Restore `prev`
		prev = savedPrev;
	}
	
	// Append to list
	prev->Next = new;
	Cache->CacheUsed ++;
	
	// Release Spinlock
	Mutex_Release( &Cache->Lock );
	
	// Return success
	return 1;
}

/**
 * \fn int IOCache_Write( tIOCache *Cache, Uint64 Sector, void *Buffer )
 * \brief Read from a cached sector
 */
int IOCache_Write( tIOCache *Cache, Uint64 Sector, void *Buffer )
{
	tIOCache_Ent	*ent;
	
	// Sanity Check!
	if(!Cache || !Buffer)
		return -1;
	// Lock
	Mutex_Acquire( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		Mutex_Release( &Cache->Lock );
		return -1;
	}
	
	// Search the list
	for( ent = Cache->Entries; ent; ent = ent->Next )
	{
		// Have we found what we are looking for?
		if( ent->Num == Sector ) {
			memcpy(ent->Data, Buffer, Cache->SectorSize);
			ent->LastAccess = ent->LastWrite = now();
			
			if(Cache->Mode == IOCACHE_WRITEBACK) {
				Cache->Write(Cache->ID, Sector, Buffer);
				ent->LastWrite = 0;
			}
			
			Mutex_Release( &Cache->Lock );
			return 1;
		}
		// It's a sorted list, so as soon as we go past `Sector` we know
		// it's not there
		if(ent->Num > Sector)	break;
	}
	
	Mutex_Release( &Cache->Lock );
	return 0;
}

/**
 * \fn void IOCache_Flush( tIOCache *Cache )
 * \brief Flush a cache
 */
void IOCache_Flush( tIOCache *Cache )
{
	tIOCache_Ent	*ent;
	
	if( Cache->Mode == IOCACHE_VIRTUAL )	return;
	
	// Lock
	Mutex_Acquire( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		Mutex_Release( &Cache->Lock );
		return;
	}
	
	// Write All
	for( ent = Cache->Entries; ent; ent = ent->Next )
	{
		Cache->Write(Cache->ID, ent->Num, ent->Data);
		ent->LastWrite = 0;
	}
	
	Mutex_Release( &Cache->Lock );
}

/**
 * \fn void IOCache_Destroy( tIOCache *Cache )
 * \brief Destroy a cache
 */
void IOCache_Destroy( tIOCache *Cache )
{
	tIOCache_Ent	*ent, *prev = NULL;
	
	// Lock
	Mutex_Acquire( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		Mutex_Release( &Cache->Lock );
		return;
	}
	
	// Free All
	for(ent = Cache->Entries;
		ent;
		prev = ent, ent = ent->Next, free(prev) )
	{
		if( Cache->Mode != IOCACHE_VIRTUAL )
		{
			Cache->Write(Cache->ID, ent->Num, ent->Data);
			ent->LastWrite = 0;
		}
	}
	
	Cache->CacheSize = 0;
	
	Mutex_Release( &Cache->Lock );
	
	// Remove from list
	SHORTLOCK( &glIOCache_Caches );
	{
		tIOCache	*cache;
		tIOCache	*prev_cache = (tIOCache*)&gIOCache_Caches;
		for(cache = gIOCache_Caches;
			cache;
			prev_cache = cache, cache = cache->Next )
		{
			if(cache == Cache) {
				prev_cache->Next = cache->Next;
				break;
			}
		}
	}
	SHORTREL( &glIOCache_Caches );
	
	free(Cache);
}

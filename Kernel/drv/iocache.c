/*
 * Acess2 Kernel
 * - IO Cache
 * 
 * By thePowersGang (John Hodge)
 */
#define DEBUG	0
#include <acess.h>
#include <iocache.h>

// === TYPES ===
typedef struct sIOCache_Ent	tIOCache_Ent;

// === STRUCTURES ===
struct sIOCache_Ent
{
	tIOCache_Ent	*Next;
	Uint64	Num;
	Sint64	LastAccess;
	Sint64	LastWrite;
	Uint8	Data[];
};

struct sIOCache
{
	tIOCache	*Next;
	 int	SectorSize;
	 int	Lock;
	 int	Mode;
	Uint32	ID;
	tIOCache_WriteCallback	Write;
	 int	CacheSize;
	 int	CacheUsed;
	tIOCache_Ent	*Entries;
};

// === GLOBALS ===
 int	glIOCache_Caches;
tIOCache	*gIOCache_Caches = NULL;
 int	giIOCache_NumCaches = 0;

// === CODE ===
/**
 * \fn tIOCache *IOCache_Create( tIOCache_WriteCallback Write, Uint32 ID, int SectorSize, int CacheSize )
 * \brief Creates a new IO Cache
 */
tIOCache *IOCache_Create( tIOCache_WriteCallback Write, Uint32 ID, int SectorSize, int CacheSize )
{
	tIOCache	*ret = malloc( sizeof(tIOCache) );
	
	// Sanity Check
	if(!ret)	return NULL;
	
	// Fill Structure
	ret->SectorSize = SectorSize;
	ret->Mode = IOCACHE_WRITEBACK;
	ret->ID = ID;
	ret->Write = Write;
	ret->CacheSize = CacheSize;
	ret->CacheUsed = 0;
	ret->Entries = 0;
	
	// Append to list
	LOCK( &glIOCache_Caches );
	ret->Next = gIOCache_Caches;
	gIOCache_Caches = ret;
	RELEASE( &glIOCache_Caches );
	
	// Return
	return ret;
}

/**
 * \fn int IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer )
 * \brief Read from a cached sector
 */
int IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer )
{
	tIOCache_Ent	*ent;
	
	ENTER("pCache XSector pBuffer", Cache, Sector, Buffer);
	
	// Sanity Check!
	if(!Cache || !Buffer) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Lock
	LOCK( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		RELEASE( &Cache->Lock );
		LEAVE('i', -1);
		return -1;
	}
	
	// Search the list
	for( ent = Cache->Entries; ent; ent = ent->Next )
	{
		// Have we found what we are looking for?
		if( ent->Num == Sector ) {
			memcpy(Buffer, ent->Data, Cache->SectorSize);
			ent->LastAccess = now();
			RELEASE( &Cache->Lock );
			LEAVE('i', 1);
			return 1;
		}
		// It's a sorted list, so as soon as we go past `Sector` we know
		// it's not there
		if(ent->Num > Sector)	break;
	}
	
	RELEASE( &Cache->Lock );
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
	LOCK( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		RELEASE( &Cache->Lock );
		return -1;
	}
	
	// Search the list
	prev = (tIOCache_Ent*)&Cache->Entries;
	for( ent = Cache->Entries; ent; prev = ent, ent = ent->Next )
	{
		// Is it already here?
		if( ent->Num == Sector ) {
			RELEASE( &Cache->Lock );
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
	RELEASE( &Cache->Lock );
	
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
	LOCK( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		RELEASE( &Cache->Lock );
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
			
			RELEASE( &Cache->Lock );
			return 1;
		}
		// It's a sorted list, so as soon as we go past `Sector` we know
		// it's not there
		if(ent->Num > Sector)	break;
	}
	
	RELEASE( &Cache->Lock );
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
	LOCK( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		RELEASE( &Cache->Lock );
		return;
	}
	
	// Write All
	for( ent = Cache->Entries; ent; ent = ent->Next )
	{
		Cache->Write(Cache->ID, ent->Num, ent->Data);
		ent->LastWrite = 0;
	}
	
	RELEASE( &Cache->Lock );
}

/**
 * \fn void IOCache_Destroy( tIOCache *Cache )
 * \brief Destroy a cache
 */
void IOCache_Destroy( tIOCache *Cache )
{
	tIOCache_Ent	*ent, *prev = NULL;
	
	// Lock
	LOCK( &Cache->Lock );
	if(Cache->CacheSize == 0) {
		RELEASE( &Cache->Lock );
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
	
	RELEASE( &Cache->Lock );
	
	// Remove from list
	LOCK( &glIOCache_Caches );
	{
		tIOCache	*ent;
		tIOCache	*prev = (tIOCache*)&gIOCache_Caches;
		for(ent = gIOCache_Caches;
			ent;
			prev = ent, ent = ent->Next )
		{
			if(ent == Cache) {
				prev->Next = ent->Next;
				break;
			}
		}
	}
	RELEASE( &glIOCache_Caches );
	
	free(Cache);
}

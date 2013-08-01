/*
 * Acess2 Kernel
 * - By thePowersGang (John Hodge)
 * 
 * dummy_iocache.c
 * - Dummied-out Block IO Caching
 */
#define DEBUG	0
#include <acess.h>
#include <iocache.h>

struct sIOCache
{
	tIOCache_WriteCallback	Write;
	void	*ID;
};

// === CODE ===
tIOCache *IOCache_Create( tIOCache_WriteCallback Write, void *ID, int SectorSize, int CacheSize )
{
	tIOCache *ret = malloc(sizeof(tIOCache));
	ret->Write = Write;
	ret->ID = ID;
	return ret;
}
int IOCache_Read( tIOCache *Cache, Uint64 Sector, void *Buffer )
{
	return 0;
}
int IOCache_Add( tIOCache *Cache, Uint64 Sector, const void *Buffer )
{
	return 1;
}
int IOCache_Write( tIOCache *Cache, Uint64 Sector, const void *Buffer )
{
	#ifndef DISABLE_WRITE
	Cache->Write(Cache->ID, Sector, Buffer);
	#endif
	return 1;
}
void IOCache_Flush( tIOCache *Cache )
{
}
void IOCache_Destroy( tIOCache *Cache )
{
	free(Cache);
}


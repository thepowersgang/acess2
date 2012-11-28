/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * vfs/nodecache.c
 * - VFS Node Caching facility
 */
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"

// === TYPES ===
typedef struct sCachedInode {
	struct sCachedInode	*Next;
	tVFS_Node	Node;
} tCachedInode;
typedef struct sInodeCache {
	struct sInodeCache	*Next;
	 int	Handle;
	tCachedInode	*FirstNode;	// Sorted List
	Uint64	MaxCached;		// Speeds up Searching
} tInodeCache;

// === PROTOTYPES ===
tInodeCache	*Inode_int_GetFSCache(int Handle);

// === GLOBALS ===
 int	gVFS_NextInodeHandle = 1;
tShortSpinlock	glVFS_InodeCache;
tInodeCache	*gVFS_InodeCache = NULL;

// === CODE ===
/**
 * \fn int Inode_GetHandle()
 */
int Inode_GetHandle()
{
	tInodeCache	*ent;
	
	ent = malloc( sizeof(tInodeCache) );
	ent->MaxCached = 0;
	ent->Handle = gVFS_NextInodeHandle++;
	ent->Next = NULL;	ent->FirstNode = NULL;
	
	// Add to list
	SHORTLOCK( &glVFS_InodeCache );
	ent->Next = gVFS_InodeCache;
	gVFS_InodeCache = ent;
	SHORTREL( &glVFS_InodeCache );
	
	return ent->Handle;
}

/**
 * \fn tVFS_Node *Inode_GetCache(int Handle, Uint64 Inode)
 * \brief Gets a node from the cache
 */
tVFS_Node *Inode_GetCache(int Handle, Uint64 Inode)
{
	tInodeCache	*cache;
	tCachedInode	*ent;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache)	return NULL;
	
	if(Inode > cache->MaxCached)	return NULL;
	
	// Search Cache
	ent = cache->FirstNode;
	for( ; ent; ent = ent->Next )
	{
		if(ent->Node.Inode < Inode)	continue;
		if(ent->Node.Inode > Inode)	return NULL;
		ent->Node.ReferenceCount ++;
		return &ent->Node;
	}
	
	return NULL;	// Should never be reached
}

/**
 * \fn tVFS_Node *Inode_CacheNode(int Handle, tVFS_Node *Node)
 */
tVFS_Node *Inode_CacheNode(int Handle, tVFS_Node *Node)
{
	tInodeCache	*cache;
	tCachedInode	*newEnt, *ent, *prev = NULL;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache)	return NULL;
	
	if(Node->Inode > cache->MaxCached)
		cache->MaxCached = Node->Inode;
	
	// Search Cache
	ent = cache->FirstNode;
	for( ; ent; prev = ent, ent = ent->Next )
	{
		if(ent->Node.Inode < Node->Inode)	continue;
		if(ent->Node.Inode == Node->Inode) {
			ent->Node.ReferenceCount ++;
			return &ent->Node;
		}
		break;
	}
	
	// Create new entity
	newEnt = malloc(sizeof(tCachedInode));
	newEnt->Next = ent;
	memcpy(&newEnt->Node, Node, sizeof(tVFS_Node));
	if( prev )
		prev->Next = newEnt;
	else
		cache->FirstNode = newEnt;
	newEnt->Node.ReferenceCount = 1;

	LOG("Cached %llx as %p", Node->Inode, &newEnt->Node);

	return &newEnt->Node;
}

/**
 * \fn void Inode_UncacheNode(int Handle, Uint64 Inode)
 * \brief Dereferences/Removes a cached node
 */
int Inode_UncacheNode(int Handle, Uint64 Inode)
{
	tInodeCache	*cache;
	tCachedInode	*ent, *prev;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache) {
		Log_Notice("Inode", "Invalid cache handle %i used", Handle);
		return -1;
	}

	ENTER("iHandle XInode", Handle, Inode);

	if(Inode > cache->MaxCached) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Search Cache
	ent = cache->FirstNode;
	prev = NULL;
	for( ; ent; prev = ent, ent = ent->Next )
	{
		if(ent->Node.Inode < Inode)	continue;
		if(ent->Node.Inode > Inode) {
			LEAVE('i', -1);
			return -1;
		}
		break;
	}

	LOG("ent = %p", ent);

	if( !ent ) {
		LEAVE('i', -1);
		return -1;
	}

	ent->Node.ReferenceCount --;
	// Check if node needs to be freed
	if(ent->Node.ReferenceCount == 0)
	{
		if( prev )
			prev->Next = ent->Next;
		else
			cache->FirstNode = ent->Next;
		if(ent->Node.Inode == cache->MaxCached)
		{
			if(ent != cache->FirstNode && prev)
				cache->MaxCached = prev->Node.Inode;
			else
				cache->MaxCached = 0;
		}
		
		if(ent->Node.Data)
			free(ent->Node.Data);	
		free(ent);
		LOG("Freed");
		LEAVE('i', 1);
		return 1;
	}
	else
	{
		LEAVE('i', 0);
		return 0;
	}
}

/**
 * \fn void Inode_ClearCache(int Handle)
 * \brief Removes a cache
 */
void Inode_ClearCache(int Handle)
{
	tInodeCache	*cache;
	tInodeCache	*prev = NULL;
	tCachedInode	*ent, *next;
	
	// Find the cache
	for(
		cache = gVFS_InodeCache;
		cache && cache->Handle < Handle;
		prev = cache, cache = cache->Next
		);
	if(!cache || cache->Handle != Handle)	return;
	
	// Search Cache
	ent = cache->FirstNode;
	while( ent )
	{
		ent->Node.ReferenceCount = 1;
		next = ent->Next;
		
		if(ent->Node.Type && ent->Node.Type->Close)
			ent->Node.Type->Close( &ent->Node );
		free(ent);
		
		ent = next;
	}
	
	// Free Cache
	if(prev == NULL)
		gVFS_InodeCache = cache->Next;
	else
		prev->Next = cache->Next;
	free(cache);
}

/**
 * \fn tInodeCache *Inode_int_GetFSCache(int Handle)
 * \brief Gets a cache given it's handle
 */
tInodeCache *Inode_int_GetFSCache(int Handle)
{
	tInodeCache	*cache = gVFS_InodeCache;
	// Find Cache
	for( ; cache; cache = cache->Next )
	{
		if(cache->Handle > Handle)	continue;
		if(cache->Handle < Handle) {
			Warning("Inode_int_GetFSCache - Handle %i not in cache\n", Handle);
			return NULL;
		}
		break;
	}
	if(!cache) {
		Warning("Inode_int_GetFSCache - Handle %i not in cache [NULL]\n", Handle);
		return NULL;
	}
	
	return cache;
}

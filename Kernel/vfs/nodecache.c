/*
 * AcessMicro VFS
 * - File IO Passthru's
 */
#include <common.h>
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
 int	gilVFS_InodeCache = 0;
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
	LOCK( &gilVFS_InodeCache );
	ent->Next = gVFS_InodeCache;
	gVFS_InodeCache = ent;
	RELEASE( &gilVFS_InodeCache );
	
	return gVFS_NextInodeHandle-1;
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
	tCachedInode	*newEnt, *ent, *prev;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache)	return NULL;
	
	if(Node->Inode > cache->MaxCached)
		cache->MaxCached = Node->Inode;
	
	// Search Cache
	ent = cache->FirstNode;
	prev = (tCachedInode*) &cache->FirstNode;
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
	prev->Next = newEnt;
		
	return &newEnt->Node;
}

/**
 * \fn void Inode_UncacheNode(int Handle, Uint64 Inode)
 * \brief Dereferences/Removes a cached node
 */
void Inode_UncacheNode(int Handle, Uint64 Inode)
{
	tInodeCache	*cache;
	tCachedInode	*ent, *prev;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache)	return;
	
	if(Inode > cache->MaxCached)	return;
	
	// Search Cache
	ent = cache->FirstNode;
	prev = (tCachedInode*) &cache->FirstNode;	// Special case removal
	for( ; ent; prev = ent, ent = ent->Next )
	{
		if(ent->Node.Inode < Inode)	continue;
		if(ent->Node.Inode > Inode)	return;
		ent->Node.ReferenceCount --;
		// Check if node needs to be freed
		if(ent->Node.ReferenceCount == 0)
		{
			prev->Next = ent->Next;
			if(ent->Node.Inode == cache->MaxCached)
			{
				if(ent != cache->FirstNode)
					cache->MaxCached = prev->Node.Inode;
				else
					cache->MaxCached = 0;
			}
				
			free(ent);
		}
		return;
	}
}

/**
 * \fn void Inode_ClearCache(int Handle)
 * \brief Removes a cache
 */
void Inode_ClearCache(int Handle)
{
	tInodeCache	*cache;
	tInodeCache	*prev = (tInodeCache*) &gVFS_InodeCache;
	tCachedInode	*ent, *next;
	
	cache = Inode_int_GetFSCache(Handle);
	if(!cache)	return;
	
	// Search Cache
	ent = cache->FirstNode;
	while( ent )
	{
		ent->Node.ReferenceCount = 1;
		next = ent->Next;
		
		if(ent->Node.Close)
			ent->Node.Close( &ent->Node );
		free(ent);
		
		ent = next;
	}
	
	// Free Cache
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

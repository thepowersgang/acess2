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
struct sInodeCache
{
	struct sInodeCache	*Next;
	tInode_CleanUpNode	CleanUpNode;
	tCachedInode	*FirstNode;	// Sorted List
	Uint64	MaxCached;		// Speeds up Searching
};

// === GLOBALS ===
tShortSpinlock	glVFS_InodeCache;
tInodeCache	*gVFS_InodeCache = NULL;

// === CODE ===

// Create a new inode cache
tInodeCache *Inode_GetHandle(tInode_CleanUpNode CleanUpNode)
{
	tInodeCache	*ent;
	
	ent = malloc( sizeof(tInodeCache) );
	ent->MaxCached = 0;
	ent->Next = NULL;
	ent->FirstNode = NULL;
	ent->CleanUpNode = CleanUpNode;
	
	// Add to list
	SHORTLOCK( &glVFS_InodeCache );
	ent->Next = gVFS_InodeCache;
	gVFS_InodeCache = ent;
	SHORTREL( &glVFS_InodeCache );
	
	return ent;
}

/**
 * \fn tVFS_Node *Inode_GetCache(int Handle, Uint64 Inode)
 * \brief Gets a node from the cache
 */
tVFS_Node *Inode_GetCache(tInodeCache *Cache, Uint64 Inode)
{
	tCachedInode	*ent;
	
	if(Inode > Cache->MaxCached)	return NULL;
	
	// Search Cache
	for( ent = Cache->FirstNode; ent; ent = ent->Next )
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
tVFS_Node *Inode_CacheNode(tInodeCache *Handle, tVFS_Node *Node)
{
	return Inode_CacheNodeEx(Handle, Node, sizeof(*Node));
}

tVFS_Node *Inode_CacheNodeEx(tInodeCache *Cache, tVFS_Node *Node, size_t Size)
{
	tCachedInode	*newEnt, *prev = NULL;

	ASSERT(Size >= sizeof(tVFS_Node));	
	
	if(Node->Inode > Cache->MaxCached)
		Cache->MaxCached = Node->Inode;
	
	// Search Cache
	tCachedInode *ent;
	for( ent = Cache->FirstNode; ent; prev = ent, ent = ent->Next )
	{
		if(ent->Node.Inode < Node->Inode)	continue;
		if(ent->Node.Inode == Node->Inode) {
			ent->Node.ReferenceCount ++;
			return &ent->Node;
		}
		break;
	}
	
	// Create new entity
	newEnt = malloc(sizeof(tCachedInode) + (Size - sizeof(tVFS_Node)));
	newEnt->Next = ent;
	memcpy(&newEnt->Node, Node, Size);
	if( prev )
		prev->Next = newEnt;
	else
		Cache->FirstNode = newEnt;
	newEnt->Node.ReferenceCount = 1;

	LOG("Cached %llx as %p", Node->Inode, &newEnt->Node);

	return &newEnt->Node;
}

/**
 * \fn void Inode_UncacheNode(int Handle, Uint64 Inode)
 * \brief Dereferences/Removes a cached node
 */
int Inode_UncacheNode(tInodeCache *Cache, Uint64 Inode)
{
	tCachedInode	*ent, *prev;
	
	ENTER("pHandle XInode", Cache, Inode);

	if(Inode > Cache->MaxCached) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Search Cache
	prev = NULL;
	for( ent = Cache->FirstNode; ent; prev = ent, ent = ent->Next )
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
			Cache->FirstNode = ent->Next;
		if(ent->Node.Inode == Cache->MaxCached)
		{
			if(ent != Cache->FirstNode && prev)
				Cache->MaxCached = prev->Node.Inode;
			else
				Cache->MaxCached = 0;
		}
	
		if(Cache->CleanUpNode)
			Cache->CleanUpNode(&ent->Node);
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
void Inode_ClearCache(tInodeCache *Cache)
{
	tInodeCache	*prev = NULL;
	tCachedInode	*ent, *next;
	
	// Find the cache
	for( prev = (void*)&gVFS_InodeCache; prev && prev->Next != Cache; prev = prev->Next )
		;
	if( !prev ) {
		// Oops?
		return;
	}
	
	// Search Cache
	for( ent = Cache->FirstNode; ent; ent = next )
	{
		next = ent->Next;
		ent->Node.ReferenceCount = 1;
		
		// Usually has the side-effect of freeing this node
		// TODO: Ensure that node is freed
		if(ent->Node.Type && ent->Node.Type->Close)
			ent->Node.Type->Close( &ent->Node );
		else
			free(ent);
	}
	
	// Free Cache
	if(prev == NULL)
		gVFS_InodeCache = Cache->Next;
	else
		prev->Next = Cache->Next;
	free(Cache);
}


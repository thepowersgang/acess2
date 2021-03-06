/*
 * Acess2 FAT12/16/32 Driver
 * - By John Hodge (thePowersGang)
 *
 * nodecache.c
 * - FAT-Specific node caching
 */
#include <acess.h>
#include <vfs.h>
#include "common.h"

// === PROTOTYPES ===
extern tVFS_Node	*FAT_int_CacheNode(tFAT_VolInfo *Disk, const tVFS_Node *Node);

// === CODE ===
tTime FAT_int_GetAcessTimestamp(Uint16 Date, Uint16 Time, Uint8 MS)
{
	return MS * 10 + timestamp(
		// Seconds         Minutes              Hours
		(Time & 0x1F) * 2, (Time >> 5) & 0x3F, (Time >> 11) & 0x1F,
		// Day             Month                    Year
		(Date & 0x1F) - 1, ((Date >> 5) & 0xF) - 1, 1980 + ((Date >> 9) & 0xFF)
		);
}

void FAT_int_GetFATTimestamp(tTime AcessTimestamp, Uint16 *Date, Uint16 *Time, Uint8 *MS)
{
	 int	y, m, d;
	 int	h, min, s, ms;
	format_date(AcessTimestamp, &y, &m, &d, &h, &min, &s, &ms);
	if(Date)
		*Date = (d + 1) | ((m + 1) << 5) | ((y - 1980) << 9);
	if(Time)
		*Time = (s / 2) | (min << 5) | (h << 11);
	if(MS)
		*MS = (ms / 10) + (s & 1) * 100;
}

/**
 * \brief Creates a tVFS_Node structure for a given file entry
 * \param Parent	Parent directory VFS node
 * \param Entry	File table entry for the new node
 */
tVFS_Node *FAT_int_CreateNode(tVFS_Node *Parent, fat_filetable *Entry)
{
	tVFS_Node	node;
	tVFS_Node	*ret;
	tFAT_VolInfo	*disk = Parent->ImplPtr;

	ENTER("pParent pEntry", Parent, Entry);
	LOG("disk = %p", disk);
	
	if( (ret = FAT_int_GetNode(disk, Entry->cluster | (Entry->clusterHi<<16))) )
	{
		if( (ret->Inode >> 32) != 0 )
		{
			LEAVE('p', ret);
			return ret;
		}
	}
	else
	{
		ret = &node;
		memset(ret, 0, sizeof(tVFS_Node));
	}
	
	// Set Other Data
	// 0-27: Cluster, 32-59: Parent Cluster
	ret->Inode = Entry->cluster | (Entry->clusterHi<<16) | (Parent->Inode << 32);
	LOG("node.Inode = %llx", ret->Inode);
	ret->ImplInt = 0;
	// Disk Pointer
	ret->ImplPtr = disk;
	ret->Size = Entry->size;
	LOG("Entry->size = %i", Entry->size);
	// root:root
	ret->UID = 0;	ret->GID = 0;
	ret->NumACLs = 1;
	
	ret->Flags = 0;
	if(Entry->attrib & ATTR_DIRECTORY)
		ret->Flags |= VFS_FFLAG_DIRECTORY;
	if(Entry->attrib & ATTR_READONLY) {
		ret->Flags |= VFS_FFLAG_READONLY;
		ret->ACLs = &gVFS_ACL_EveryoneRX;	// R-XR-XR-X
	}
	else {
		ret->ACLs = &gVFS_ACL_EveryoneRWX;	// RWXRWXRWX
	}
	
	// Create timestamps
	ret->CTime = FAT_int_GetAcessTimestamp(Entry->cdate, Entry->ctime, Entry->ctimems);
	ret->MTime = FAT_int_GetAcessTimestamp(Entry->mdate, Entry->mtime, 0);
	ret->ATime = FAT_int_GetAcessTimestamp(Entry->adate, 0, 0);
	
	// Set pointers
	if(ret->Flags & VFS_FFLAG_DIRECTORY) {
		//Log_Debug("FAT", "Directory %08x has size 0x%x", node.Inode, node.Size);
		ret->Type = &gFAT_DirType;	
		ret->Size = -1;
	}
	else {
		ret->Type = &gFAT_FileType;
	}

	// Cache node only if we're not updating a cache
	if( ret == &node ) {
		ret = FAT_int_CacheNode(disk, &node);
	}
	
	LEAVE('p', ret);
	return ret;
}

tVFS_Node *FAT_int_CreateIncompleteDirNode(tFAT_VolInfo *Disk, Uint32 Cluster)
{
	if( Cluster == Disk->rootOffset )
		return &Disk->rootNode;
	
	// If the directory isn't in the cache, what do?
	// - we want to lock it such that we don't collide, but don't want to put crap data in the cache
	// - Put a temp node in with a flag that indicates it's incomplete?
	
	Mutex_Acquire(&Disk->lNodeCache);
	tFAT_CachedNode	*cnode, *prev = NULL;

	for(cnode = Disk->NodeCache; cnode; prev = cnode, cnode = cnode->Next)
	{
		if( (cnode->Node.Inode & 0xFFFFFFFF) == Cluster ) {
			cnode->Node.ReferenceCount ++;
			Mutex_Release(&Disk->lNodeCache);
			return &cnode->Node;
		}
	}	

	// Create a temporary node?
	cnode = calloc( sizeof(tFAT_CachedNode), 1 );
	cnode->Node.Inode = Cluster;
	cnode->Node.ReferenceCount = 1;
	cnode->Node.ImplPtr = Disk;
	cnode->Node.Type = &gFAT_DirType;	
	cnode->Node.Size = -1;

	if( prev )
		prev->Next = cnode;
	else
		Disk->NodeCache = cnode;

	Mutex_Release(&Disk->lNodeCache);
	return &cnode->Node;
}

tVFS_Node *FAT_int_GetNode(tFAT_VolInfo *Disk, Uint32 Cluster)
{
	if( Cluster == Disk->rootOffset )
		return &Disk->rootNode;
	Mutex_Acquire(&Disk->lNodeCache);
	tFAT_CachedNode	*cnode;

	for(cnode = Disk->NodeCache; cnode; cnode = cnode->Next)
	{
		if( (cnode->Node.Inode & 0xFFFFFFFF) == Cluster ) {
			cnode->Node.ReferenceCount ++;
			Mutex_Release(&Disk->lNodeCache);
			return &cnode->Node;
		}
	}

	Mutex_Release(&Disk->lNodeCache);
	return NULL;
}

tVFS_Node *FAT_int_CacheNode(tFAT_VolInfo *Disk, const tVFS_Node *Node)
{
	tFAT_CachedNode	*cnode, *prev = NULL;
	Mutex_Acquire(&Disk->lNodeCache);
	
	for(cnode = Disk->NodeCache; cnode; prev = cnode, cnode = cnode->Next )
	{
		if( cnode->Node.Inode == Node->Inode ) {
			cnode->Node.ReferenceCount ++;
			Mutex_Release(&Disk->lNodeCache);
			return &cnode->Node;
		}
	}
	
	cnode = malloc(sizeof(tFAT_CachedNode));
	cnode->Next = NULL;
	memcpy(&cnode->Node, Node, sizeof(tVFS_Node));
	cnode->Node.ReferenceCount = 1;
	
	if( prev )
		prev->Next = cnode;
	else
		Disk->NodeCache = cnode;
	
	Mutex_Release(&Disk->lNodeCache);
	return &cnode->Node;
}

int FAT_int_DerefNode(tVFS_Node *Node)
{
	tFAT_VolInfo	*Disk = Node->ImplPtr;
	tFAT_CachedNode	*cnode, *prev = NULL;
	 int	bFreed = 0;

	if( Node == &Disk->rootNode )
		return 0;

	Mutex_Acquire(&Disk->lNodeCache);
	Node->ReferenceCount --;
	for(cnode = Disk->NodeCache; cnode; prev = cnode, cnode = cnode->Next )
	{
		if(Node == &cnode->Node) {
			if(prev)
				prev->Next = cnode->Next;
			else
				Disk->NodeCache = cnode->Next;
			break;
		}
	}
	if(Node->ReferenceCount == 0 && cnode) {
		// Already out of the list :)
		if(cnode->Node.Data)
			free(cnode->Node.Data);
		VFS_CleanupNode(&cnode->Node);
		free(cnode);
		bFreed = 1;
	}
	Mutex_Release(&Disk->lNodeCache);
	if( !cnode ) {
		// Not here?
		return -1;
	}
	
	return bFreed;
}

void FAT_int_ClearNodeCache(tFAT_VolInfo *Disk)
{
	// TODO: In theory when this is called, all handles will be closed
}

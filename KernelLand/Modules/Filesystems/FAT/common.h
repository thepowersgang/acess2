/*
 * Acess2 FAT Filesystem Driver
 * - By John Hodge (thePowersGang)
 * 
 * common.h
 * - FAT internal common header
 */
#ifndef _FS__FAT__COMMON_H_
#define _FS__FAT__COMMON_H_

#include "fs_fat.h"
#include <vfs.h>

#define CACHE_FAT	0	//!< Caches the FAT in memory
#define USE_LFN		1	//!< Enables the use of Long File Names
#define	SUPPORT_WRITE	1	//!< Enables write support

#define FAT_FLAG_DIRTY	0x10000
#define FAT_FLAG_DELETE	0x20000

typedef struct sFAT_VolInfo tFAT_VolInfo;
#if USE_LFN
typedef struct sFAT_LFNCacheEnt	tFAT_LFNCacheEnt;
typedef struct sFAT_LFNCache	tFAT_LFNCache;
#endif
typedef struct sFAT_CachedNode	tFAT_CachedNode;

/**
 * \brief Internal IDs for FAT types
 */
enum eFatType
{
	FAT12,	//!< FAT12 Volume
	FAT16,	//!< FAT16 Volume
	FAT32,	//!< FAT32 Volume
};

// === TYPES ===
struct sFAT_VolInfo
{
	 int	fileHandle;	//!< File Handle
	enum eFatType	type;	//!< FAT Variant
	char	name[12];	//!< Volume Name (With NULL Terminator)
	Uint32	firstDataSect;	//!< First data sector
	Uint32	rootOffset;	//!< Root Offset (clusters)
	Uint32	ClusterCount;	//!< Total Cluster Count
	fat_bootsect	bootsect;	//!< Boot Sector
	tVFS_Node	rootNode;	//!< Root Node
	size_t	BytesPerCluster;
	
	tMutex	lNodeCache;
	tFAT_CachedNode	*NodeCache;
	
	tMutex	lFAT;   	//!< Lock to prevent double-writing to the FAT
	#if CACHE_FAT
	Uint32	*FATCache;	//!< FAT Cache
	#endif
};

#if USE_LFN
/**
 * \brief Long-Filename cache entry
 */
struct sFAT_LFNCacheEnt
{
	 int	ID;
	Uint16	Data[256];
};
/**
 * \brief Long-Filename cache
 */
struct sFAT_LFNCache
{
	 int	NumEntries;
	tFAT_LFNCacheEnt	Entries[];
};
#endif

struct sFAT_CachedNode
{
	struct sFAT_CachedNode	*Next;
	tVFS_Node	Node;
};

// --- General Helpers ---
extern int	FAT_int_GetAddress(tVFS_Node *Node, Uint64 Offset, Uint64 *Addr, Uint32 *Cluster);
extern tTime	FAT_int_GetAcessTimestamp(Uint16 Date, Uint16 Time, Uint8 MS);
extern void	FAT_int_GetFATTimestamp(tTime AcessTimestamp, Uint16 *Date, Uint16 *Time, Uint8 *MS);

// --- Node Caching ---
// NOTE: FAT uses its own node cache that references by cluster (not the inode value that the Inode_* cache uses)
//       because tVFS_Node.Inode contains the parent directory inode
extern tVFS_Node	*FAT_int_CreateNode(tVFS_Node *Parent, fat_filetable *Entry);
extern tVFS_Node	*FAT_int_CreateIncompleteDirNode(tFAT_VolInfo *Disk, Uint32 Cluster);
extern tVFS_Node	*FAT_int_GetNode(tFAT_VolInfo *Disk, Uint32 Cluster);
extern int	FAT_int_DerefNode(tVFS_Node *Node);
extern void	FAT_int_ClearNodeCache(tFAT_VolInfo *Disk);

// --- FAT Access ---
extern Uint32	FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 Cluster);
#if SUPPORT_WRITE
extern Uint32	FAT_int_AllocateCluster(tFAT_VolInfo *Disk, Uint32 Previous);
extern Uint32	FAT_int_FreeCluster(tFAT_VolInfo *Disk, Uint32 Cluster);
#endif
extern void	FAT_int_ReadCluster(tFAT_VolInfo *Disk, Uint32 Cluster, int Length, void *Buffer);
extern void	FAT_int_WriteCluster(tFAT_VolInfo *Disk, Uint32 Cluster, const void *Buffer);

// --- Directory Access ---
extern int	FAT_ReadDir(tVFS_Node *Node, int ID, char Dest[FILENAME_MAX]);
extern tVFS_Node	*FAT_FindDir(tVFS_Node *Node, const char *Name);
extern tVFS_Node	*FAT_GetNodeFromINode(tVFS_Node *Root, Uint64 Inode);
extern int	FAT_int_GetEntryByCluster(tVFS_Node *DirNode, Uint32 Cluster, fat_filetable *Entry);
#if SUPPORT_WRITE
extern int	FAT_int_WriteDirEntry(tVFS_Node *Node, int ID, fat_filetable *Entry);
extern tVFS_Node	*FAT_Mknod(tVFS_Node *Node, const char *Name, Uint Flags);
extern int	FAT_Link(tVFS_Node *DirNode, const char *NewName, tVFS_Node *Node);
extern int	FAT_Unlink(tVFS_Node *DirNode, const char *OldName);
#endif
extern void	FAT_CloseFile(tVFS_Node *node);

// === GLOBALS ===
extern tVFS_NodeType	gFAT_DirType;
extern tVFS_NodeType	gFAT_FileType;

#endif


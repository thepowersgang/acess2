/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * common.h - Common Types and Definitions
 */
#ifndef _NTFS__COMMON_H_
#define _NTFS__COMMON_H_

#include <acess.h>
#include <vfs.h>
#include "ntfs.h"

typedef struct sNTFS_File	tNTFS_File;
typedef struct sNTFS_Directory	tNTFS_Directory;
typedef struct sNTFS_Disk	tNTFS_Disk;
typedef struct sNTFS_Attrib	tNTFS_Attrib;
typedef struct sNTFS_AttribDataRun	tNTFS_AttribDataRun;

// === STRUCTURES ===

struct sNTFS_File
{
	tVFS_Node	Node;
	tNTFS_Attrib	*Data;
};

struct sNTFS_Directory
{
	tVFS_Node	Node;
	tNTFS_Attrib	*I30Root;
	tNTFS_Attrib	*I30Allocation;
};

/**
 * In-memory representation of an NTFS Disk
 */
struct sNTFS_Disk
{
	 int	FD;
	 int	CacheHandle;
	 
	 int	ClusterSize;
	
	Uint64	MFTBase;
	Uint32	MFTRecSize;
	
	tNTFS_Attrib	*MFTDataAttr;
	tNTFS_Attrib	*MFTBitmapAttr;

	 int	InodeCache;
	
	tNTFS_Directory	RootDir;
};

struct sNTFS_AttribDataRun
{
	Uint64	Count;
	Uint64	LCN;
};

struct sNTFS_Attrib
{
	tNTFS_Disk	*Disk;
	Uint32	Type;

	char	*Name;

	Uint64	DataSize;	

	 int	IsResident;
	union {
		void	*ResidentData;
		struct {
			 int	nRuns;
			 int	CompressionUnitL2Size;	// 0 = uncompressed
			Uint64	FirstPopulatedCluster;
			tNTFS_AttribDataRun	*Runs;
		} NonResident;
	};
};

extern tVFS_NodeType	gNTFS_DirType;
extern tVFS_NodeType	gNTFS_FileType;

extern int	NTFS_int_ApplyUpdateSequence(void *Buf, size_t BufLen, const Uint16 *Sequence, size_t NumEntries);
// -- MFT Access / Manipulation
extern tNTFS_FILE_Header	*NTFS_GetMFT(tNTFS_Disk *Disk, Uint32 MFTEntry);
extern void	NTFS_ReleaseMFT(tNTFS_Disk *Disk, Uint32 MFTEntry, tNTFS_FILE_Header *Entry);
extern tNTFS_Attrib	*NTFS_GetAttrib(tNTFS_Disk *Disk, Uint32 MFTEntry, int Type, const char *Name, int DesIdx);
extern size_t	NTFS_ReadAttribData(tNTFS_Attrib *Attrib, Uint64 Offset, size_t Length, void *Buffer);
// -- dir.c
extern int	NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
extern tVFS_Node	*NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);

#endif

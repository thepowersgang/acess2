/*
 * Acess2 Kernel - RAM Disk Support
 * - By John Hodge (thePowersGang)
 *
 * ramdisk.h
 * - Core header
 */
#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#include <vfs.h>

#define RAMFS_NDIRECT	12

typedef struct sRAMFS_Inode	tRAMFS_Inode;
typedef struct sRAMFS_File	tRAMFS_File;
typedef struct sRAMFS_DirEnt	tRAMFS_DirEnt;
typedef struct sRAMFS_Dir	tRAMFS_Dir;
typedef struct sRAMDisk 	tRAMDisk;

struct sRAMFS_Inode
{
	tRAMDisk	*Disk;
	tVFS_Node	Node;
	 int	Type;	// 0: Normal, 1: Dir, 2: Symlink
	 int	nLink;
};

struct sRAMFS_File
{
	tRAMFS_Inode	Inode;
	size_t	Size;
	 int	nAllocPageSlots;
	Uint32	PagesDirect[RAMFS_NDIRECT];
	Uint32	Indirect1Page;	// PAGE_SIZE/sizeof(Uint32) = 1024 on x86
	Uint32	Indirect2Page;	// ~1 Million on x86
};

struct sRAMFS_DirEnt
{
	tRAMFS_DirEnt	*Next;
	tRAMFS_Inode	*Inode;
	Uint8	NameLen;
	char	Name[];
};

struct sRAMFS_Dir
{
	tRAMFS_Inode	Inode;

	tRAMFS_DirEnt	*FirstEnt;
	tRAMFS_DirEnt	*LastEnt;
};

struct sRAMDisk
{
	tRAMFS_Dir	RootDir;
	
	 int	MaxPages;
	Uint32	*Bitmap;
	 int	nUsedPages;
	tPAddr	PhysPages[];
};

#endif


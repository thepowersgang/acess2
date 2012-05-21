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

typedef struct sRAMDisk_Inode	tRAMDisk_Inode;
typedef struct sRAMDisk_File	tRAMDisk_File;
typedef struct sRAMDisk_DirEnt	tRAMDisk_DirEnt;
typedef struct sRAMDisk_Dir	tRAMDisk_Dir;
typedef struct sRAMDisk 	tRAMDisk;

struct sRAMDisk_Inode
{
	tRAMDisk	*Disk;
	tVFS_Node	Node;
	 int	Type;	// 0: Normal, 1: Dir, 2: Symlink
	 int	nLink;
};

struct sRAMDisk_File
{
	tRAMDisk_Inode	Inode;
	size_t	Size;
	 int	nAllocPageSlots;
	Uint32	PagesDirect[12];
	Uint32	Indirect1Page;	// PAGE_SIZE/sizeof(Uint32) = 1024 on x86
	Uint32	Indirect2Page;	// ~1 Million on x86
};

struct sRAMDisk_DirEnt
{
	tRAMDisk_DirEnt	*Next;
	tRAMDisk_Inode	*Inode;
	Uint8	NameLen;
	char	Name[];
};

struct sRAMDisk_Dir
{
	tRAMDisk_Inode	Inode;

	tRAMDisk_DirEnt	*FirstEnt;
	tRAMDisk_DirEnt	*LastEnt;
};

struct sRAMDisk
{
	tRAMDisk_Dir	RootDir;
	
	 int	MaxPages;
	 int	nUsedPages;
	tPAddr	PhysPages[];
};

#endif


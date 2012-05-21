/*
 * Acess2 Kernel - RAM Disk Support
 * - By John Hodge (thePowersGang)
 *
 * ramdisk.c
 * - Core File
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include "ramdisk.h"
#define VERSION	VER2(0,1)

#define MIN_RAMDISK_SIZE	(1*1024*1024)
#define MAX_RAMDISK_SIZE	(64*1024*1024)

// === PROTOTYPES ===
 int	RAMDisk_Install(char **Arguments);
void	RAMDisk_Cleanup(void);
// --- Mount/Unmount ---
tVFS_Node	*RAMDisk_InitDevice(const char *Device, const char **Options);
void	RAMDisk_Unmount(tVFS_Node *Node);
// --- Directories ---
char	*RAMDisk_ReadDir(tVFS_Node *Node, int Index);
tVFS_Node	*RAMDisk_FindDir(tVFS_Node *Node, const char *Name);
 int	RAMDisk_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	RAMDisk_Link(tVFS_Node *DirNode, const char *Name, tVFS_Node *Node);
 int	RAMDisk_Unlink(tVFS_Node *Node, const char *Name);
// --- Files ---
size_t	RAMDisk_Read(tVFS_Node *Node, off_t Offset, size_t Size, void *Buffer);
size_t	RAMDisk_Write(tVFS_Node *Node, off_t Offset, size_t Size, const void *Buffer);
// --- Internals --
void	RAMDisk_int_RefFile(tRAMDisk_Inode *Inode);
void	RAMDisk_int_DerefFile(tRAMDisk_Inode *Inode);
void	*_GetPage(tRAMDisk_File *File, int Page);
void	_DropPage(void *Page);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, FS_RAMDisk, RAMDisk_Install, RAMDisk_Cleanup, NULL);
tVFS_Driver	gRAMDisk_Driver = {
	.Name = "RAMDisk",
	.InitDevice = RAMDisk_InitDevice,
	.Unmount = RAMDisk_Unmount
	// TODO: GetNodeFromInode
	};
tVFS_NodeType	gRAMDisk_DirNodeType = {
	.ReadDir = RAMDisk_ReadDir,
	.FindDir = RAMDisk_FindDir,
	.MkNod   = RAMDisk_MkNod,
//	.Link    = RAMDisk_Link,
//	.Unlink  = RAMDisk_Unink
	};
tVFS_NodeType	gRAMDisk_FileNodeType = {
	.Read  = RAMDisk_Read,
	.Write = RAMDisk_Write
	};

// === CODE ===
int RAMDisk_Install(char **Arguments)
{
	VFS_AddDriver( &gRAMDisk_Driver );
	return 0;
}

void RAMDisk_Cleanup(void)
{
	
}


// --- Mount/Unmount
tVFS_Node *RAMDisk_InitDevice(const char *Device, const char **Options)
{
	size_t	size = 0;
	tRAMDisk	*rd;

	if( Options ) 
	{
		for( int i = 0; Options[i]; i ++ )
		{
			if( strcmp(Options[i], "megs") == '=' )
			{
				size = atoi(Options[i] + 5) * 1024 * 1024;
			}
		}
	}

	if( size > MAX_RAMDISK_SIZE || size < MIN_RAMDISK_SIZE )
		return NULL;

	 int	n_pages = size / PAGE_SIZE;
	
	rd = calloc(1, sizeof(tRAMDisk) + n_pages * sizeof(tPAddr));
	rd->MaxPages = n_pages;
	rd->RootDir.Inode.Disk = rd;
	rd->RootDir.Inode.Node.ImplPtr = &rd->RootDir;
	rd->RootDir.Inode.Node.Flags = VFS_FFLAG_DIRECTORY;
	rd->RootDir.Inode.Node.Size = -1;
	rd->RootDir.Inode.Node.Type = &gRAMDisk_DirNodeType;

	return &rd->RootDir.Inode.Node;
}

void RAMDisk_Unmount(tVFS_Node *Node)
{
	Log_Warning("RAMDisk", "TODO: Impliment unmounting");
}

// --- Directories ---
char *RAMDisk_ReadDir(tVFS_Node *Node, int Index)
{
	tRAMDisk_Dir	*dir = Node->ImplPtr;
	for( tRAMDisk_DirEnt *d = dir->FirstEnt; d; d = d->Next )
	{
		if( Index -- == 0 )
			return strdup(d->Name);
	}
	return NULL;
}

tVFS_Node *RAMDisk_FindDir(tVFS_Node *Node, const char *Name)
{
	tRAMDisk_Dir	*dir = Node->ImplPtr;
	for( tRAMDisk_DirEnt *d = dir->FirstEnt; d; d = d->Next )
	{
		if( strcmp(d->Name, Name) == 0 )
			return &d->Inode->Node;
	}
	
	return NULL;
}

int RAMDisk_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tRAMDisk_Dir	*dir = Node->ImplPtr;
	if( RAMDisk_FindDir(Node, Name) != NULL )
		return -1;
	
	tRAMDisk_DirEnt	*de = malloc( sizeof(tRAMDisk_DirEnt) + strlen(Name) + 1 );
	de->Next = NULL;
	de->NameLen = strlen(Name);
	strcpy(de->Name, Name);

	if( Flags & VFS_FFLAG_DIRECTORY ) {
		tRAMDisk_Dir	*newdir = calloc(1, sizeof(tRAMDisk_Dir));
		newdir->Inode.Node.Type = &gRAMDisk_DirNodeType;
		de->Inode = &newdir->Inode;
	}
	else {
		tRAMDisk_File	*newfile = calloc(1, sizeof(tRAMDisk_File));
		newfile->Inode.Node.Type = &gRAMDisk_FileNodeType;
		de->Inode = &newfile->Inode;
	}
	de->Inode->Disk = dir->Inode.Disk;
	de->Inode->Node.Flags = Flags;
	de->Inode->Node.ImplPtr = de->Inode;

	RAMDisk_int_RefFile(de->Inode);

	// TODO: Lock?
	if(dir->FirstEnt)
		dir->LastEnt->Next = de;
	else
		dir->FirstEnt = de;
	dir->LastEnt = de;

	return 0;
}

int RAMDisk_Link(tVFS_Node *DirNode, const char *Name, tVFS_Node *FileNode)
{
	tRAMDisk_Dir	*dir = DirNode->ImplPtr;
	tRAMDisk_DirEnt	*dp;

	for( dp = dir->FirstEnt; dp; dp = dp->Next )
	{
		if( strcmp(dp->Name, Name) == 0 )
			return -1;
	}
	
	dp = malloc( sizeof(tRAMDisk_DirEnt) + strlen(Name) + 1 );
	dp->Next = NULL;
	dp->Inode = FileNode->ImplPtr;
	RAMDisk_int_RefFile(dp->Inode);
	strcpy(dp->Name, Name);

	// TODO: Lock?	
	if(dir->FirstEnt)
		dir->LastEnt->Next = dp;
	else
		dir->FirstEnt = dp;
	dir->LastEnt = dp;
	
	return 0;
}

int RAMDisk_Unlink(tVFS_Node *Node, const char *Name)
{
	tRAMDisk_Dir	*dir = Node->ImplPtr;
	tRAMDisk_DirEnt	*dp, *p = NULL;

	// TODO: Is locking needed?

	for( dp = dir->FirstEnt; dp; p = dp, dp = dp->Next )
	{
		if( strcmp(dp->Name, Name) == 0 )
			break ;
	}
	if( !dp )	return -1;
	
	RAMDisk_int_DerefFile( dp->Inode );

	if( !p )
		dir->FirstEnt = dp->Next;
	else
		p->Next = dp->Next;
	if( dir->LastEnt == dp )
		dir->LastEnt = p;
	free(dp);

	return 0;
}

// --- Files ---
size_t RAMDisk_Read(tVFS_Node *Node, off_t Offset, size_t Size, void *Buffer)
{
	tRAMDisk_File	*file = Node->ImplPtr;
	 int	page_ofs;
	char	*page_virt;
	size_t	rem;
	
	if( Offset >= file->Size )
		return 0;
	
	if( Offset + Size > file->Size )
		Size = file->Size - Size;

	if( Size == 0 )
		return 0;

	page_ofs = Offset / PAGE_SIZE;
	Offset %= PAGE_SIZE;
	rem = Size;

	page_virt = _GetPage(file, page_ofs++);
	if( Offset + Size <= PAGE_SIZE ) {
		memcpy(Buffer, page_virt + Offset, Size);
		return Size;
	}
	
	memcpy(Buffer, page_virt + Offset, PAGE_SIZE - Offset);
	Buffer += PAGE_SIZE - Offset;
	rem -= PAGE_SIZE - Offset;
	
	while( rem >= PAGE_SIZE )
	{
		_DropPage(page_virt);
		page_virt = _GetPage(file, page_ofs++);
		memcpy(Buffer, page_virt, PAGE_SIZE);
		Buffer += PAGE_SIZE;
		rem -= PAGE_SIZE;
	}

	if( rem > 0 )
	{
		page_virt = _GetPage(file, page_ofs);
		memcpy(Buffer, page_virt, rem);
	}

	_DropPage(page_virt);
	
	return Size;
}

size_t RAMDisk_Write(tVFS_Node *Node, off_t Offset, size_t Size, const void *Buffer)
{
	return 0;
}



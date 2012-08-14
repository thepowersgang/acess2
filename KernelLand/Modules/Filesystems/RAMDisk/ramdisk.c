/*
 * Acess2 Kernel - RAM Disk Support
 * - By John Hodge (thePowersGang)
 *
 * ramdisk.c
 * - Core File
 */
#define DEBUG	1
#include <acess.h>
#include <modules.h>
#include "ramdisk.h"
#define VERSION	VER2(0,1)

#define MIN_RAMDISK_SIZE	(1*1024*1024)
#define MAX_RAMDISK_SIZE	(64*1024*1024)

// === PROTOTYPES ===
 int	RAMFS_Install(char **Arguments);
 int	RAMFS_Cleanup(void);
// --- Mount/Unmount ---
tVFS_Node	*RAMFS_InitDevice(const char *Device, const char **Options);
void	RAMFS_Unmount(tVFS_Node *Node);
// --- Directories ---
char	*RAMFS_ReadDir(tVFS_Node *Node, int Index);
tVFS_Node	*RAMFS_FindDir(tVFS_Node *Node, const char *Name);
 int	RAMFS_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	RAMFS_Link(tVFS_Node *DirNode, const char *Name, tVFS_Node *Node);
 int	RAMFS_Unlink(tVFS_Node *Node, const char *Name);
// --- Files ---
size_t	RAMFS_Read(tVFS_Node *Node, off_t Offset, size_t Size, void *Buffer);
size_t	RAMFS_Write(tVFS_Node *Node, off_t Offset, size_t Size, const void *Buffer);
// --- Internals --
void	RAMFS_int_RefFile(tRAMFS_Inode *Inode);
void	RAMFS_int_DerefFile(tRAMFS_Inode *Inode);
void	*RAMFS_int_GetPage(tRAMFS_File *File, int Page, int bCanAlloc);
void	RAMFS_int_DropPage(void *Page);
Uint32	RAMFS_int_AllocatePage(tRAMDisk *Disk);
void	RAMFS_int_RefFile(tRAMFS_Inode *Inode);
void	RAMFS_int_DerefFile(tRAMFS_Inode *Inode);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, FS_RAMDisk, RAMFS_Install, RAMFS_Cleanup, NULL);
tVFS_Driver	gRAMFS_Driver = {
	.Name = "ramfs",
	.InitDevice = RAMFS_InitDevice,
	.Unmount = RAMFS_Unmount
	// TODO: GetNodeFromInode
	};
tVFS_NodeType	gRAMFS_DirNodeType = {
	.ReadDir = RAMFS_ReadDir,
	.FindDir = RAMFS_FindDir,
	.MkNod   = RAMFS_MkNod,
	.Link    = RAMFS_Link,
	.Unlink  = RAMFS_Unlink
	};
tVFS_NodeType	gRAMFS_FileNodeType = {
	.Read  = RAMFS_Read,
	.Write = RAMFS_Write
	};

// === CODE ===
int RAMFS_Install(char **Arguments)
{
	VFS_AddDriver( &gRAMFS_Driver );
	return 0;
}

int RAMFS_Cleanup(void)
{
	return 0;
}


// --- Mount/Unmount
const char *_GetOption(const char *Data, const char *Option)
{
	 int	len = strlen(Option);
	if( strncmp(Data, Option, len) != 0 )
		return NULL;
	if( Data[len] != '=' )
		return NULL;
	return Data + len + 1;
}

tVFS_Node *RAMFS_InitDevice(const char *Device, const char **Options)
{
	size_t	size = 0;
	tRAMDisk	*rd;

	if( Options ) 
	{
		const char *v;
		for( int i = 0; Options[i]; i ++ )
		{
			LOG("Options[%i] = '%s'", i, Options[i]);
			if( (v = _GetOption(Options[i], "megs")) )
			{
				size = atoi(v) * 1024 * 1024;
				LOG("Size set to %iMiB", size>>20);
			}
		}
	}

	LOG("Disk Size %iKiB", size>>10);

	if( size > MAX_RAMDISK_SIZE || size < MIN_RAMDISK_SIZE )
		return NULL;

	 int	n_pages = size / PAGE_SIZE;
	
	rd = calloc(1, sizeof(tRAMDisk) + n_pages * sizeof(tPAddr) + n_pages / 8);
	rd->MaxPages = n_pages;
	rd->RootDir.Inode.Disk = rd;
	rd->RootDir.Inode.Node.ImplPtr = &rd->RootDir;
	rd->RootDir.Inode.Node.Flags = VFS_FFLAG_DIRECTORY;
	rd->RootDir.Inode.Node.Size = -1;
	rd->RootDir.Inode.Node.Type = &gRAMFS_DirNodeType;
	rd->Bitmap = (void*)&rd->PhysPages[ n_pages ];

	for( int i = 0; i < n_pages; i ++ )
	{
		rd->PhysPages[i] = MM_AllocPhys();
		if( !rd->PhysPages[i] ) {
			// Um... oops?
			break;
		}
	}

	LOG("Mounted");
	return &rd->RootDir.Inode.Node;
}

void RAMFS_Unmount(tVFS_Node *Node)
{
	Log_Warning("RAMDisk", "TODO: Impliment unmounting");
}

// --- Directories ---
char *RAMFS_ReadDir(tVFS_Node *Node, int Index)
{
	tRAMFS_Dir	*dir = Node->ImplPtr;
	for( tRAMFS_DirEnt *d = dir->FirstEnt; d; d = d->Next )
	{
		if( Index -- == 0 ) {
			LOG("Return %s", d->Name);
			return strdup(d->Name);
		}
	}
	LOG("Return NULL");
	return NULL;
}

tVFS_Node *RAMFS_FindDir(tVFS_Node *Node, const char *Name)
{
	tRAMFS_Dir	*dir = Node->ImplPtr;

	for( tRAMFS_DirEnt *d = dir->FirstEnt; d; d = d->Next )
	{
		if( strcmp(d->Name, Name) == 0 ) {
			LOG("Return %p", &d->Inode->Node);
			return &d->Inode->Node;
		}
	}
	
	LOG("Return NULL");
	return NULL;
}

int RAMFS_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tRAMFS_Dir	*dir = Node->ImplPtr;
	if( RAMFS_FindDir(Node, Name) != NULL )
		return -1;
	
	tRAMFS_DirEnt	*de = malloc( sizeof(tRAMFS_DirEnt) + strlen(Name) + 1 );
	de->Next = NULL;
	de->NameLen = strlen(Name);
	strcpy(de->Name, Name);

	if( Flags & VFS_FFLAG_DIRECTORY ) {
		tRAMFS_Dir	*newdir = calloc(1, sizeof(tRAMFS_Dir));
		newdir->Inode.Node.Type = &gRAMFS_DirNodeType;
		de->Inode = &newdir->Inode;
	}
	else {
		tRAMFS_File	*newfile = calloc(1, sizeof(tRAMFS_File));
		newfile->Inode.Node.Type = &gRAMFS_FileNodeType;
		de->Inode = &newfile->Inode;
	}
	de->Inode->Disk = dir->Inode.Disk;
	de->Inode->Node.Flags = Flags;
	de->Inode->Node.ImplPtr = de->Inode;

	RAMFS_int_RefFile(de->Inode);

	// TODO: Lock?
	if(dir->FirstEnt)
		dir->LastEnt->Next = de;
	else
		dir->FirstEnt = de;
	dir->LastEnt = de;

	return 0;
}

int RAMFS_Link(tVFS_Node *DirNode, const char *Name, tVFS_Node *FileNode)
{
	tRAMFS_Dir	*dir = DirNode->ImplPtr;
	tRAMFS_DirEnt	*dp;

	for( dp = dir->FirstEnt; dp; dp = dp->Next )
	{
		if( strcmp(dp->Name, Name) == 0 )
			return -1;
	}
	
	dp = malloc( sizeof(tRAMFS_DirEnt) + strlen(Name) + 1 );
	dp->Next = NULL;
	dp->Inode = FileNode->ImplPtr;
	RAMFS_int_RefFile(dp->Inode);
	strcpy(dp->Name, Name);

	// TODO: Lock?	
	if(dir->FirstEnt)
		dir->LastEnt->Next = dp;
	else
		dir->FirstEnt = dp;
	dir->LastEnt = dp;
	
	return 0;
}

int RAMFS_Unlink(tVFS_Node *Node, const char *Name)
{
	tRAMFS_Dir	*dir = Node->ImplPtr;
	tRAMFS_DirEnt	*dp, *p = NULL;

	// TODO: Is locking needed?

	// Find the directory entry
	for( dp = dir->FirstEnt; dp; p = dp, dp = dp->Next )
	{
		if( strcmp(dp->Name, Name) == 0 )
			break ;
	}
	if( !dp )	return -1;
	
	// Dereference the file
	RAMFS_int_DerefFile( dp->Inode );

	// Remove and free directory entry
	if(!p)
		dir->FirstEnt = dp->Next;
	else
		p->Next = dp->Next;
	if( dir->LastEnt == dp )
		dir->LastEnt = p;
	free(dp);

	return 0;
}

// --- Files ---
size_t RAMFS_int_DoIO(tRAMFS_File *File, off_t Offset, size_t Size, void *Buffer, int bRead)
{
	size_t	rem;
	Uint8	*page_virt;
	 int	page;

	if( Offset >= File->Size )	return 0;
	
	if( Offset + Size > File->Size )	Size = File->Size - Size;

	if( Size == 0 )		return 0;

	page = Offset / PAGE_SIZE;
	Offset %= PAGE_SIZE;
	rem = Size;

	page_virt = RAMFS_int_GetPage(File, page++, !bRead);
	if(!page_virt)	return 0;
	
	if( Offset + Size <= PAGE_SIZE )
	{
		if( bRead )
			memcpy(Buffer, page_virt + Offset, Size);
		else
			memcpy(page_virt + Offset, Buffer, Size);
		return 0;
	}
	
	if( bRead )
		memcpy(Buffer, page_virt + Offset, PAGE_SIZE - Offset);
	else
		memcpy(page_virt + Offset, Buffer, PAGE_SIZE - Offset);
	Buffer += PAGE_SIZE - Offset;
	rem -= PAGE_SIZE - Offset;
	
	while( rem >= PAGE_SIZE )
	{
		RAMFS_int_DropPage(page_virt);
		page_virt = RAMFS_int_GetPage(File, page++, !bRead);
		if(!page_virt)	return Size - rem;
		
		if( bRead )
			memcpy(Buffer, page_virt, PAGE_SIZE);
		else
			memcpy(page_virt, Buffer, PAGE_SIZE);
		
		Buffer += PAGE_SIZE;
		rem -= PAGE_SIZE;
	}

	if( rem > 0 )
	{
		page_virt = RAMFS_int_GetPage(File, page, !bRead);
		if(!page_virt)	return Size - rem;
		if( bRead )
			memcpy(Buffer, page_virt, rem);
		else
			memcpy(page_virt, Buffer, rem);
	}

	RAMFS_int_DropPage(page_virt);
	
	return Size;
}

size_t RAMFS_Read(tVFS_Node *Node, off_t Offset, size_t Size, void *Buffer)
{
	tRAMFS_File	*file = Node->ImplPtr;
	
	return RAMFS_int_DoIO(file, Offset, Size, Buffer, 1);
}

size_t RAMFS_Write(tVFS_Node *Node, off_t Offset, size_t Size, const void *Buffer)
{
	tRAMFS_File	*file = Node->ImplPtr;
	
	// TODO: Limit checks?
	if( Offset == file->Size )
		file->Size += Size;

	return RAMFS_int_DoIO(file, Offset, Size, (void*)Buffer, 0);
}

// --- Internals ---
void *RAMFS_int_GetPage(tRAMFS_File *File, int Page, int bCanAlloc)
{
	Uint32	page_id = 0;
	Uint32	*page_in_1 = NULL;
	Uint32	*page_in_2 = NULL;

	 int	ofs = 0, block;

	if( Page < 0 )	return NULL;
	
	if( Page < RAMFS_NDIRECT )
	{
		if( File->PagesDirect[Page] )
			page_id = File->PagesDirect[Page];
	}
	else if( Page - RAMFS_NDIRECT < PAGE_SIZE/4 )
	{
		ofs = Page - RAMFS_NDIRECT;
		if( File->Indirect1Page == 0 ) {
			if( !bCanAlloc )
				return NULL;
			else
				File->Indirect1Page = RAMFS_int_AllocatePage(File->Inode.Disk) + 1;
		}
		page_in_1 = MM_MapTemp( File->Inode.Disk->PhysPages[File->Indirect1Page-1] );
		page_id = page_in_1[ofs];
	}
	else if( Page - RAMFS_NDIRECT - PAGE_SIZE/4 < (PAGE_SIZE/4)*(PAGE_SIZE/4) )
	{
		block = (Page - RAMFS_NDIRECT - PAGE_SIZE/4) / (PAGE_SIZE/4);
		ofs   = (Page - RAMFS_NDIRECT - PAGE_SIZE/4) % (PAGE_SIZE/4);
		if( File->Indirect2Page == 0 ){
			if( !bCanAlloc )
				return NULL;
			else
				File->Indirect2Page = RAMFS_int_AllocatePage(File->Inode.Disk) + 1;
		}

		page_in_2 = MM_MapTemp( File->Inode.Disk->PhysPages[File->Indirect2Page-1] );
		if( page_in_2[block] == 0 ) {
			if( !bCanAlloc )
				return NULL;
			else
				page_in_2[block] = RAMFS_int_AllocatePage(File->Inode.Disk) + 1;
		}

		page_in_1 = MM_MapTemp( File->Inode.Disk->PhysPages[page_in_2[block] - 1] );
		page_id = page_in_1[ofs];
	}

	if( page_id == 0 )
	{
		if( !bCanAlloc )
			return NULL;
		
		page_id = RAMFS_int_AllocatePage(File->Inode.Disk) + 1;
		if(page_in_1)
			page_in_1[ofs] = page_id;
		else
			File->PagesDirect[Page] = page_id;
	}
	
	MM_FreeTemp( page_in_1 );
	MM_FreeTemp( page_in_2 );

	return MM_MapTemp( File->Inode.Disk->PhysPages[page_id - 1] );
}

void RAMFS_int_DropPage(void *Page)
{
	MM_FreeTemp( Page );
}

Uint32 RAMFS_int_AllocatePage(tRAMDisk *Disk)
{
	 int	i, j;

	// Quick check
	if( Disk->nUsedPages == Disk->MaxPages )
		return 0;

	// Find a chunk with at least one free page
	for( i = 0; i < Disk->MaxPages / 32; i ++ )
	{
		if( Disk->Bitmap[i] != -1 )
			break;
	}
	if( i == Disk->MaxPages / 32 ) {
		Log_Error("RAMFS", "Bookkeeping error, count and bitmap disagree");
		return 0;
	}

	// Find the exact page
	for( j = 0; j < 32; j ++ )
	{
		if( !(Disk->Bitmap[i] & (1U << j)) )
			break ;
	}
	ASSERT(j < 32);

	return i * 32 + j;
}

void RAMFS_int_RefFile(tRAMFS_Inode *Inode)
{
	Inode->nLink ++;
}

void RAMFS_int_DerefFile(tRAMFS_Inode *Inode)
{
	Inode->nLink --;
	if( Inode->nLink >= 0 )
		return ;

	Log_Error("RAMFS", "TODO: Clean up files when deleted");

	// Need to delete file
	switch( Inode->Type )
	{
	case 0:	// File
		break ;
	case 1:	// Directory
		break ;
	case 2:	// Symlink
		break ;
	}
	free(Inode);
}

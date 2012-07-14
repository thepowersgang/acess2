/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * lvm.h
 * - LVM Core definitions
 */
#define DEBUG	1
#define VERSION	VER2(0,1)
#include "lvm_int.h"
#include <fs_devfs.h>
#include <modules.h>
#include <api_drv_disk.h>

// === PROTOTYPES ===
// ---
 int	LVM_Initialise(char **Arguments);
 int	LVM_Cleanup(void);
// ---
char	*LVM_Root_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*LVM_Root_FindDir(tVFS_Node *Node, const char *Name);
char	*LVM_Vol_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*LVM_Vol_FindDir(tVFS_Node *Node, const char *Name);
size_t	LVM_Vol_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	LVM_Vol_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
size_t	LVM_SubVol_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	LVM_SubVol_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);

Uint	LVM_int_DrvUtil_ReadBlock(Uint64 Address, Uint Count, void *Buffer, void *Argument);
Uint	LVM_int_DrvUtil_WriteBlock(Uint64 Address, Uint Count, const void *Buffer, void *Argument);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, LVM, LVM_Initialise, LVM_Cleanup, NULL);
tVFS_NodeType	gLVM_RootNodeType = {
	.ReadDir = LVM_Root_ReadDir,
	.FindDir = LVM_Root_FindDir
};
tVFS_NodeType	gLVM_VolNodeType = {
	.ReadDir = LVM_Vol_ReadDir,
	.FindDir = LVM_Vol_FindDir,
	.Read = LVM_Vol_Read,
	.Write = LVM_Vol_Write
};
tVFS_NodeType	gLVM_SubVolNodeType = {
	.Read = LVM_SubVol_Read,
	.Write = LVM_SubVol_Write
};
tDevFS_Driver	gLVM_DevFS = {
	NULL, "LVM",
	{.Flags = VFS_FFLAG_DIRECTORY, .Type = &gLVM_RootNodeType, .Size = -1}
};

tLVM_Vol	*gpLVM_FirstVolume;
tLVM_Vol	*gpLVM_LastVolume = (void*)&gpLVM_FirstVolume;

// === CODE ===
int LVM_Initialise(char **Arguments)
{
	DevFS_AddDevice( &gLVM_DevFS );
	return 0;
}

int LVM_Cleanup(void)
{
	// Attempt to destroy all volumes
	tLVM_Vol	*vol, *prev = NULL, *next;
	
	// TODO: Locks?
	for( vol = gpLVM_FirstVolume; vol; prev = vol, vol = next )
	{
		next = vol->Next;
		 int	nFree = 0;
		
		for( int i = 0; i < vol->nSubVolumes; i ++ )
		{
			tLVM_SubVolume	*sv;
			sv = vol->SubVolumes[i];
			if( sv == NULL ) {
				nFree ++;
				continue;
			}

			Mutex_Acquire(&sv->Node.Lock);
			if(sv->Node.ReferenceCount == 0) {
				nFree ++;
				vol->SubVolumes[i] = NULL;
			}
			Mutex_Release(&sv->Node.Lock);
			
			Mutex_Acquire(&sv->Node.Lock);
			LOG("Removed subvolume %s:%s", vol->Name, sv->Name);
			free(sv);
		}
		
		if( nFree != vol->nSubVolumes )
			continue ;

		if(prev)
			prev->Next = next;
		else
			gpLVM_FirstVolume = next;

		Mutex_Acquire(&vol->DirNode.Lock);
		Mutex_Acquire(&vol->VolNode.Lock);
		if( vol->Type->Cleanup )
			vol->Type->Cleanup( vol->Ptr );
		LOG("Removed volume %s", vol->Name);
		free(vol);
	}

	if( gpLVM_FirstVolume )	
		return EBUSY;
	
	return EOK;
}

// --------------------------------------------------------------------
// VFS Inteface
// --------------------------------------------------------------------
char *LVM_Root_ReadDir(tVFS_Node *Node, int ID)
{
	tLVM_Vol	*vol;
	
	if( ID < 0 )	return NULL;	

	for( vol = gpLVM_FirstVolume; vol && ID --; vol = vol->Next );
	
	if(vol)
		return strdup(vol->Name);
	else
		return NULL;
}
tVFS_Node *LVM_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	tLVM_Vol	*vol;
	for( vol = gpLVM_FirstVolume; vol; vol = vol->Next )
	{
		if( strcmp(vol->Name, Name) == 0 )
		{
			return &vol->DirNode;
		}
	}
	return NULL;
}

char *LVM_Vol_ReadDir(tVFS_Node *Node, int ID)
{
	tLVM_Vol	*vol = Node->ImplPtr;
	
	if( ID < 0 || ID >= vol->nSubVolumes+1 )
		return NULL;

	if( ID == 0 )
		return strdup("ROOT");
	else
		return strdup( vol->SubVolumes[ID-1]->Name );
}
tVFS_Node *LVM_Vol_FindDir(tVFS_Node *Node, const char *Name)
{
	tLVM_Vol	*vol = Node->ImplPtr;

	if( strcmp("ROOT", Name) == 0 )
		return &vol->VolNode;
	
	for( int i = 0; i < vol->nSubVolumes; i ++ )
	{
		if( strcmp(vol->SubVolumes[i]->Name, Name) == 0 )
		{
			return &vol->SubVolumes[i]->Node;
		}
	}

	return NULL;
}

size_t LVM_Vol_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	tLVM_Vol	*vol = Node->ImplPtr;
	Uint64	byte_size = vol->BlockCount * vol->BlockSize;	

	if( Offset > byte_size )
		return 0;
	if( Length > byte_size )
		Length = byte_size;
	if( Offset + Length > byte_size )
		Length = byte_size - Offset;

	return DrvUtil_ReadBlock(
		Offset, Length, Buffer, 
		LVM_int_DrvUtil_ReadBlock, vol->BlockSize, vol
		);
}

size_t LVM_Vol_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	return 0;
}

size_t LVM_SubVol_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	tLVM_SubVolume	*sv = Node->ImplPtr;
	Uint64	byte_size = sv->BlockCount * sv->Vol->BlockSize;

	if( Offset > byte_size )
		return 0;
	if( Length > byte_size )
		Length = byte_size;
	if( Offset + Length > byte_size )
		Length = byte_size - Offset;

	LOG("Reading (0x%llx+0x%llx)+0x%x to %p",
		(Uint64)(sv->FirstBlock * sv->Vol->BlockSize), Offset,
		Length, Buffer
		);
	
	Offset += sv->FirstBlock * sv->Vol->BlockSize;	

	return DrvUtil_ReadBlock(
		Offset, Length, Buffer, 
		LVM_int_DrvUtil_ReadBlock, sv->Vol->BlockSize, sv->Vol
		);
}
size_t LVM_SubVol_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	tLVM_SubVolume	*sv = Node->ImplPtr;
	Uint64	byte_size = sv->BlockCount * sv->Vol->BlockSize;

	if( Offset > byte_size )
		return 0;
	if( Length > byte_size )
		Length = byte_size;
	if( Offset + Length > byte_size )
		Length = byte_size - Offset;

	Offset += sv->FirstBlock * sv->Vol->BlockSize;	
	
	return DrvUtil_WriteBlock(
		Offset, Length, Buffer,
		LVM_int_DrvUtil_ReadBlock, LVM_int_DrvUtil_WriteBlock,
		sv->Vol->BlockSize, sv->Vol
		);
}

Uint LVM_int_DrvUtil_ReadBlock(Uint64 Address, Uint Count, void *Buffer, void *Argument)
{
	return LVM_int_ReadVolume( Argument, Address, Count, Buffer );
}

Uint LVM_int_DrvUtil_WriteBlock(Uint64 Address, Uint Count, const void *Buffer, void *Argument)
{
	return LVM_int_WriteVolume( Argument, Address, Count, Buffer );
}


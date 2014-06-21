/* 
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/shm.c
 * - Shared memory "device"
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <memfs_helpers.h> 
#include <semaphore.h>

// === TYPES ===
#define PAGES_PER_BLOCK	1024
typedef struct sSHM_BufferBlock
{
	struct sSHM_BufferBlock	*Next;
	tPAddr	Pages[PAGES_PER_BLOCK];
} tSHM_BufferBlock;
typedef struct
{
	tMemFS_FileHdr	FileHdr;
	tVFS_Node	Node;
	tSHM_BufferBlock	FirstBlock;
} tSHM_Buffer;

// === PROTOTYPES ===
 int	SHM_Install(char **Arguments);
 int	SHM_Uninstall(void);
tSHM_Buffer	*SHM_CreateBuffer(const char *Name);
void	SHM_DeleteBuffer(tSHM_Buffer *Buffer);
// - Root directory
 int	SHM_ReadDir(tVFS_Node *Node, int Id, char Dest[FILENAME_MAX]);
tVFS_Node	*SHM_FindDir(tVFS_Node *Node, const char *Filename, Uint Flags);
tVFS_Node	*SHM_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	SHM_Unlink(tVFS_Node *Node, const char *OldName);
// - Buffers
void	SHM_Reference(tVFS_Node *Node);
void	SHM_Close(tVFS_Node *Node);
size_t	SHM_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	SHM_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	SHM_MMap(struct sVFS_Node *Node, off_t Offset, int Length, void *Dest);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, SHM, SHM_Install, SHM_Uninstall, NULL);
tMemFS_DirHdr	gSHM_RootDir = {
	.FileHdr = {.Name = "SHMRoot"}
};
tVFS_NodeType	gSHM_DirNodeType = {
	.TypeName = "SHM Root",
	.ReadDir = SHM_ReadDir,
	.FindDir = SHM_FindDir,
	.MkNod   = SHM_MkNod,
	.Unlink  = SHM_Unlink,
};
tVFS_NodeType	gSHM_FileNodeType = {
	.TypeName = "SHM Buffer",
	.Read  = SHM_Read,
	.Write = SHM_Write,
	.Close = SHM_Close,
	.MMap  = SHM_MMap,
	.Reference = SHM_Reference,
};
tDevFS_Driver	gSHM_DriverInfo = {
	.Name = "shm",
	.RootNode = {
		.Size = 0,
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRW,
		.Flags = VFS_FFLAG_DIRECTORY,
		.Type = &gSHM_DirNodeType
	}
};

// === CODE ===
int SHM_Install(char **Arguments)
{
	MemFS_InitDir(&gSHM_RootDir);
	DevFS_AddDevice( &gSHM_DriverInfo );
	return MODULE_ERR_OK;
}
int SHM_Uninstall(void)
{
	return MODULE_ERR_OK;
}
tSHM_Buffer *SHM_CreateBuffer(const char *Name)
{
	tSHM_Buffer *ret = calloc(1, sizeof(tSHM_Buffer) + strlen(Name) + 1);
	MemFS_InitFile(&ret->FileHdr);
	ret->FileHdr.Name = (const char*)(ret+1);
	strcpy((char*)ret->FileHdr.Name, Name);
	ret->Node.ImplPtr = ret;
	ret->Node.Type = &gSHM_FileNodeType;
	ret->Node.ReferenceCount = 1;
	return ret;
}
void SHM_DeleteBuffer(tSHM_Buffer *Buffer)
{
	ASSERTCR(Buffer->Node.ReferenceCount,==,0,);
	
	// TODO: Destroy multi-block nodes
	ASSERT(Buffer->FirstBlock.Next == NULL);
	
	free(Buffer);
}
// - Root directory
int SHM_ReadDir(tVFS_Node *Node, int Id, char Dest[FILENAME_MAX])
{
	return MemFS_ReadDir(&gSHM_RootDir, Id, Dest);
}

/**
 * \brief Open a shared memory buffer
 *
 * \note Opening 'anon' will always succeed, and will create an anonymous mapping
 */
tVFS_Node *SHM_FindDir(tVFS_Node *Node, const char *Filename, Uint Flags)
{
	if( strcmp(Filename, "anon") == 0 )
	{
		tSHM_Buffer *ret = SHM_CreateBuffer("");
		return &ret->Node;
	}
	
	tMemFS_FileHdr	*file = MemFS_FindDir(&gSHM_RootDir, Filename);
	if( !file )
		return NULL;
	
	return &((tSHM_Buffer*)file)->Node;
}


/**
 * \brief Create a named shared memory file
 */
tVFS_Node *SHM_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	if( MemFS_FindDir(&gSHM_RootDir, Name) )
		return NULL;
	
	tSHM_Buffer *ret = SHM_CreateBuffer(Name);
	if( !MemFS_Insert(&gSHM_RootDir, &ret->FileHdr) )
	{
		ret->Node.ReferenceCount = 0;
		SHM_DeleteBuffer(ret);
		return NULL;
	}

	return &ret->Node;
}

/**
 * \breif Remove a named shared memory buffer (will be deleted when all references drop)
 */
int SHM_Unlink(tVFS_Node *Node, const char *OldName)
{
	tMemFS_FileHdr	*file = MemFS_Remove(&gSHM_RootDir, OldName);
	if( !file )
		return 1;

	tSHM_Buffer *buf = (tSHM_Buffer*)file;
	if( buf->Node.ReferenceCount == 0 )
	{
		SHM_DeleteBuffer(buf);
	}
	else
	{
		// dangling references, let them clean themselves up later
	}
	return 0;
}
// - Buffers
void SHM_Reference(tVFS_Node *Node)
{
	Node->ReferenceCount ++;
}
void SHM_Close(tVFS_Node *Node)
{
	Node->ReferenceCount --;
	if( Node->ReferenceCount == 0 )
	{
		// TODO: How to tell if a buffer should be deleted here?
		UNIMPLEMENTED();
	}
}
size_t SHM_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	UNIMPLEMENTED();
	return -1;
}
size_t SHM_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	// TODO: Should first write determine the fixed size of the buffer?
	UNIMPLEMENTED();
	return -1;
}
int SHM_MMap(struct sVFS_Node *Node, off_t Offset, int Length, void *Dest)
{
	UNIMPLEMENTED();
	return 1;
}


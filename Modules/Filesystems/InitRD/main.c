/*
 * Acess OS
 * InitRD Driver Version 1
 */
#include "initrd.h"
#include <modules.h>

// === IMPORTS ==
extern tVFS_Node	gInitRD_RootNode;

// === PROTOTYPES ===
 int	InitRD_Install(char **Arguments);
tVFS_Node	*InitRD_InitDevice(char *Device, char **Arguments);
void	InitRD_Unmount(tVFS_Node *Node);
Uint64	InitRD_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Size, void *Buffer);
char	*InitRD_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*InitRD_FindDir(tVFS_Node *Node, char *Name);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0A, FS_InitRD, InitRD_Install, NULL);
tVFS_Driver	gInitRD_FSInfo = {
	"initrd", 0, InitRD_InitDevice, InitRD_Unmount, NULL
	};

/**
 * \brief Register initrd with the kernel
 */
int InitRD_Install(char **Arguments)
{
	VFS_AddDriver( &gInitRD_FSInfo );
	return MODULE_ERR_OK;
}

/**
 * \brief Mount the InitRD
 */
tVFS_Node *InitRD_InitDevice(char *Device, char **Arguments)
{
	return &gInitRD_RootNode;
}

/**
 * \brief Unmount the InitRD
 */
void InitRD_Unmount(tVFS_Node *Node)
{
}

/**
 * \brief Read from a file
 */
Uint64 InitRD_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	if(Offset > Node->Size)
		return 0;
	if(Offset + Length > Node->Size)
		Length = Node->Size - Offset;
	
	memcpy(Buffer, Node->ImplPtr+Offset, Length);
	
	return Length;
}

/**
 * \brief Read from a directory
 */
char *InitRD_ReadDir(tVFS_Node *Node, int ID)
{
	tInitRD_File	*dir = Node->ImplPtr;
	
	if(ID >= Node->Size)
		return NULL;
	
	return strdup(dir[ID].Name);
}

/**
 * \brief Find an element in a directory
 */
tVFS_Node *InitRD_FindDir(tVFS_Node *Node, char *Name)
{
	 int	i;
	tInitRD_File	*dir = Node->ImplPtr;
	
	Log("InirRD_FindDir: Name = '%s'", Name);
	
	for( i = 0; i < Node->Size; i++ )
	{
		if(strcmp(Name, dir[i].Name) == 0)
			return dir[i].Node;
	}
	
	return NULL;
}

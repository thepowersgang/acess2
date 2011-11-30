/*
 * Acess OS
 * InitRD Driver Version 1
 */
#include "initrd.h"
#include <modules.h>

#define DUMP_ON_MOUNT	1

// === IMPORTS ==
extern tVFS_Node	gInitRD_RootNode;
extern const int	giInitRD_NumFiles;
extern tVFS_Node * const	gInitRD_FileList[];

// === PROTOTYPES ===
 int	InitRD_Install(char **Arguments);
tVFS_Node	*InitRD_InitDevice(const char *Device, const char **Arguments);
void	InitRD_Unmount(tVFS_Node *Node);
tVFS_Node	*InitRD_GetNodeFromINode(tVFS_Node *Root, Uint64 Inode);
Uint64	InitRD_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Size, void *Buffer);
char	*InitRD_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*InitRD_FindDir(tVFS_Node *Node, const char *Name);
void	InitRD_DumpDir(tVFS_Node *Node, int Indent);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0A, FS_InitRD, InitRD_Install, NULL);
tVFS_Driver	gInitRD_FSInfo = {
	"initrd", 0, InitRD_InitDevice, InitRD_Unmount, InitRD_GetNodeFromINode
	};

/**
 * \brief Register initrd with the kernel
 */
int InitRD_Install(char **Arguments)
{
	Log_Notice("InitRD", "Installed");
	VFS_AddDriver( &gInitRD_FSInfo );
	
	return MODULE_ERR_OK;
}

/**
 * \brief Mount the InitRD
 */
tVFS_Node *InitRD_InitDevice(const char *Device, const char **Arguments)
{
	#if DUMP_ON_MOUNT
	InitRD_DumpDir( &gInitRD_RootNode, 0 );
	#endif
	Log_Notice("InitRD", "Mounted (%i files)", giInitRD_NumFiles);
	return &gInitRD_RootNode;
}

/**
 * \brief Unmount the InitRD
 */
void InitRD_Unmount(tVFS_Node *Node)
{
}

/**
 */
tVFS_Node *InitRD_GetNodeFromINode(tVFS_Node *Root, Uint64 Inode)
{
	if( Inode >= giInitRD_NumFiles )	return NULL;
	return gInitRD_FileList[Inode];
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
tVFS_Node *InitRD_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	i;
	tInitRD_File	*dir = Node->ImplPtr;
	
	LOG("Name = '%s'", Name);
	
	for( i = 0; i < Node->Size; i++ )
	{
		if(strcmp(Name, dir[i].Name) == 0)
			return dir[i].Node;
	}
	
	return NULL;
}

void InitRD_DumpDir(tVFS_Node *Node, int Indent)
{
	 int	i;
	char	indent[Indent+1];
	tInitRD_File	*dir = Node->ImplPtr;
	
	for( i = 0; i < Indent;	i++ )	indent[i] = ' ';
	indent[i] = '\0';
	
	for( i = 0; i < Node->Size; i++ )
	{
		Log_Debug("InitRD", "%s- %p %s", indent, dir[i].Node, dir[i].Name);
		if(dir[i].Node->Flags & VFS_FFLAG_DIRECTORY)
			InitRD_DumpDir(dir[i].Node, Indent+1);
	}
}

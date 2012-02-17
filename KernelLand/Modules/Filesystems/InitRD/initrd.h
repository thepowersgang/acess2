/*
 */
#ifndef _INITRD_H_
#define _INITRD_H_

#include <acess.h>
#include <vfs.h>

typedef struct sInitRD_File
{
	char	*Name;
	tVFS_Node	*Node;
}	tInitRD_File;


// === Functions ===
extern size_t	InitRD_ReadFile(tVFS_Node *Node, off_t Offset, size_t Size, void *Buffer);
extern char	*InitRD_ReadDir(tVFS_Node *Node, int ID);
extern tVFS_Node	*InitRD_FindDir(tVFS_Node *Node, const char *Name);

// === Globals ===
tVFS_NodeType	gInitRD_DirType;
tVFS_NodeType	gInitRD_FileType;

#endif

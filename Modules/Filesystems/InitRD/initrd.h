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
extern Uint64	InitRD_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Size, void *Buffer);
extern char	*InitRD_ReadDir(tVFS_Node *Node, int ID);
extern tVFS_Node	*InitRD_FindDir(tVFS_Node *Node, char *Name);

#endif

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


// === Globals ===
tVFS_NodeType	gInitRD_DirType;
tVFS_NodeType	gInitRD_FileType;

#endif

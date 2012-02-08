/* 
 * AcessMicro VFS
 * - RAM Filesystem Coommon Structures
 */
#ifndef _RAMFS_H
#define _RAMFS_H
#include <vfs.h>

typedef struct sRamFS_File {
	struct sRamFS_File	*Next;
	struct sRamFS_File	*Parent;
	char	Name[32];
	tVFS_Node	Node;
	union {
		struct sRamFS_File	*FirstChild;
		char	*Bytes;
	} Data;
} tRamFS_File;

#endif 

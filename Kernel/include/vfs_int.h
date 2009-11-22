/* 
 * Acess Micro - VFS Server Ver 1
 */
#ifndef _VFS_INT_H
#define _VFS_INT_H

#include "vfs.h"

// === TYPES ===
typedef struct sVFS_Mount {
	struct sVFS_Mount	*Next;
	char	*MountPoint;
	 int	MountPointLen;
	char	*Device;
	char	*Options;
	tVFS_Driver	*Filesystem;
	tVFS_Node	*RootNode;
	char	StrData[];
} tVFS_Mount;

typedef struct sVFS_Handle {
	tVFS_Node	*Node;
	tVFS_Mount	*Mount;
	Uint64	Position;
	Uint	Mode;
} tVFS_Handle;

typedef struct sVFS_Proc {
	struct sVFS_Proc	*Next;
	 int	ID;
	 int	CwdLen;
	Uint	UID, GID;
	char	*Cwd;
	 int	MaxHandles;
	tVFS_Handle	Handles[];
} tVFS_Proc;

// === GLOBALS ===
extern tVFS_Mount	*gVFS_Mounts;

// === PROTOTYPES ===
// --- OPEN.C ---
extern char	*VFS_GetAbsPath(char *Path);
extern tVFS_Node	*VFS_ParsePath(char *Path, char **TruePath);
extern tVFS_Handle	*VFS_GetHandle(int FD);
// --- ACLS.C ---
extern int	VFS_CheckACL(tVFS_Node *Node, Uint Permissions);

#endif

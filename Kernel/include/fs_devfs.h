/*
 * Acess 2
 * Device Filesystem (DevFS)
 * - vfs/fs/devfs.c
 */
#ifndef _FS_DEVFS_H
#define _FS_DEVFS_H
#include <vfs.h>

// === TYPES ===
typedef struct sDevFS_Driver {
	struct sDevFS_Driver	*Next;
	char	*Name;
	tVFS_Node	RootNode;
} tDevFS_Driver;

// === FUNCTIONS ===
extern int	DevFS_AddDevice(tDevFS_Driver *Dev);

#endif

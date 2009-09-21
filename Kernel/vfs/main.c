/* 
 * Acess 2
 * Virtual File System
 */
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"

// === IMPORTS ===
extern tVFS_Driver	gRootFS_Info;
extern tVFS_Driver	gDevFS_Info;

// === GLOBALS ===
tVFS_Node	NULLNode = {0};
static int	siDriverListLock = 0;
tVFS_Driver	*gVFS_Drivers = NULL;

// === CODE ===
/**
 * \fn int VFS_Init()
 * \brief Initialises the VFS for use by the kernel and user
 */
int VFS_Init()
{
	// Core Drivers
	gVFS_Drivers = &gRootFS_Info;
	gVFS_Drivers->Next = &gDevFS_Info;
	
	VFS_Mount("root", "/", "rootfs", "");
	VFS_MkDir("/Devices");
	VFS_MkDir("/Mount");
	VFS_Mount("dev", "/Devices", "devfs", "");
	
	CFGINT(CFG_VFS_MAXFILES) = 32;
	return 0;
}

/**
 * \fn char *VFS_GetTruePath(char *Path)
 * \brief Gets the true path (non-symlink) of a file
 */
char *VFS_GetTruePath(char *Path)
{
	tVFS_Node	*node;
	char	*ret;
	
	node = VFS_ParsePath(Path, &ret);
	
	if(!node)	return NULL;
	if(node->Close)	node->Close(node);
	
	return ret;
}

/**
 * \fn void VFS_GetMemPath(void *Base, Uint Length, char *Dest)
 * \brief Create a VFS memory pointer path
 */
void VFS_GetMemPath(void *Base, Uint Length, char *Dest)
{
	Log("VFS_GetMemPath: (Base=%p, Length=0x%x, Dest=%p)", Base, Length, Dest);
	Dest[0] = '$';
	itoa( &Dest[1], (Uint)Base, 16, BITS/4, '0' );
	Dest[BITS/4+1] = ':';
	itoa( &Dest[BITS/4+2], Length, 16, BITS/4, '0' );
	
	Log("VFS_GetMemPath: Dest = \"%s\"", Dest);
}

/**
 * \fn tVFS_Driver *VFS_GetFSByName(char *Name)
 * \brief Gets a filesystem structure given a name
 */
tVFS_Driver *VFS_GetFSByName(char *Name)
{
	tVFS_Driver	*drv = gVFS_Drivers;
	
	for(;drv;drv=drv->Next)
	{
		if(strcmp(drv->Name, Name) == 0)
			return drv;
	}
	return NULL;
}

/**
 * \fn int VFS_AddDriver(tVFS_Driver *Info)
 */
int VFS_AddDriver(tVFS_Driver *Info)
{
	if(!Info)	return  -1;
	
	LOCK( &siDriverListLock );
	Info->Next = gVFS_Drivers;
	gVFS_Drivers = Info;
	RELEASE( &siDriverListLock );
	
	return 0;
}

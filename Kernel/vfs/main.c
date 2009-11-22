/* 
 * Acess 2
 * Virtual File System
 */
#include <common.h>
#include <fs_sysfs.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"

// === IMPORTS ===
extern tVFS_Driver	gRootFS_Info;
extern tVFS_Driver	gDevFS_Info;

// === PROTOTYPES ===
 int	VFS_Init();
char	*VFS_GetTruePath(char *Path);
void	VFS_GetMemPath(void *Base, Uint Length, char *Dest);
tVFS_Driver	*VFS_GetFSByName(char *Name);
 int	VFS_AddDriver(tVFS_Driver *Info);
void	VFS_UpdateDriverFile();

// === GLOBALS ===
tVFS_Node	NULLNode = {0};
tSpinlock	siDriverListLock = 0;
tVFS_Driver	*gVFS_Drivers = NULL;
char	*gsVFS_DriverFile = NULL;
 int	giVFS_DriverFileID = 0;

char	*gsVFS_MountFile = NULL;
 int	giVFS_MountFileID = 0;

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
	VFS_UpdateDriverFile();
	
	// Register with SysFS
	giVFS_MountFileID = SysFS_RegisterFile("VFS/Mounts", NULL, 0);
	giVFS_DriverFileID = SysFS_RegisterFile("VFS/Drivers", NULL, 0);
	
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
	char	*ret, *tmp;
	
	tmp = VFS_GetAbsPath(Path);
	if(tmp == NULL)	return NULL;
	node = VFS_ParsePath(tmp, &ret);
	free(tmp);
	
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
	
	VFS_UpdateDriverFile();
	
	return 0;
}

/**
 * \fn void VFS_UpdateDriverFile()
 * \brief Updates the driver list file
 */
void VFS_UpdateDriverFile()
{
	tVFS_Driver	*drv;
	 int	len = 0;
	char	*buf;
	
	// Format:
	// <name>\n
	for( drv = gVFS_Drivers; drv; drv = drv->Next )
	{
		len += 1 + strlen(drv->Name);
	}
	buf = malloc(len+1);
	len = 0;
	for( drv = gVFS_Drivers; drv; drv = drv->Next )
	{
		strcpy( &buf[len], drv->Name );
		len += strlen(drv->Name);
		buf[len++] = '\n';
	}
	buf[len] = '\0';
	
	SysFS_UpdateFile( giVFS_DriverFileID, buf, len );
	if(gsVFS_DriverFile)	free(gsVFS_DriverFile);
	gsVFS_DriverFile = buf;
}

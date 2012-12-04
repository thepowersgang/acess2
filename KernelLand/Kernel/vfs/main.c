/* 
 * Acess 2
 * Virtual File System
 */
#include <acess.h>
#include <fs_sysfs.h>
#include <threads.h>
#include <vfs.h>
#include <vfs_int.h>
#include <vfs_ext.h>

// === IMPORTS ===
extern tVFS_Driver	gRootFS_Info;
extern tVFS_Driver	gDevFS_Info;

// === PROTOTYPES ===
#if 0
 int	VFS_Init(void);
char	*VFS_GetTruePath(const char *Path);
void	VFS_GetMemPath(char *Dest, void *Base, Uint Length);
tVFS_Driver	*VFS_GetFSByName(const char *Name);
 int	VFS_AddDriver(tVFS_Driver *Info);
#endif
void	VFS_UpdateDriverFile(void);

// === EXPORTS ===
EXPORT(VFS_AddDriver);

// === GLOBALS ===
tVFS_Node	NULLNode = {.Type=NULL};
tShortSpinlock	slDriverListLock;
tVFS_Driver	*gVFS_Drivers = NULL;
char	*gsVFS_DriverFile = NULL;
 int	giVFS_DriverFileID = 0;

char	*gsVFS_MountFile = NULL;
 int	giVFS_MountFileID = 0;

// === CODE ===
/**
 * \fn int VFS_Init(void)
 * \brief Initialises the VFS for use by the kernel and user
 */
int VFS_Init(void)
{
	// Core Drivers
	gVFS_Drivers = &gRootFS_Info;
	gVFS_Drivers->Next = &gDevFS_Info;
	VFS_UpdateDriverFile();
	
	// Register with SysFS
	giVFS_MountFileID = SysFS_RegisterFile("VFS/Mounts", NULL, 0);
	giVFS_DriverFileID = SysFS_RegisterFile("VFS/Drivers", NULL, 0);
	
	if( VFS_Mount("root", "/", "rootfs", "") != 0 ) {
		Log_KernelPanic("VFS", "Unable to mount root (Where the **** is rootfs?)");
		return -1;
	}
	VFS_MkDir("/Devices");
	VFS_MkDir("/Mount");
	VFS_Mount("dev", "/Devices", "devfs", "");
		
	Log_Debug("VFS", "Setting max files");
	*Threads_GetMaxFD() = 32;
	return 0;
}

void VFS_Deinit(void)
{
	SysFS_RemoveFile(giVFS_MountFileID);
	free(gsVFS_MountFile);
	SysFS_RemoveFile(giVFS_DriverFileID);
	free(gsVFS_DriverFile);
}

/**
 * \fn char *VFS_GetTruePath(const char *Path)
 * \brief Gets the true path (non-symlink) of a file
 */
char *VFS_GetTruePath(const char *Path)
{
	tVFS_Node	*node;
	char	*ret, *tmp;
	
	tmp = VFS_GetAbsPath(Path);
	if(tmp == NULL)	return NULL;
	//Log(" VFS_GetTruePath: tmp = '%s'", tmp);
	node = VFS_ParsePath(tmp, &ret, NULL);
	free(tmp);
	//Log(" VFS_GetTruePath: node=%p, ret='%s'", node, ret);
	
	if(!node)	return NULL;
	if(node->Type->Close)	node->Type->Close(node);
	
	return ret;
}

/**
 * \fn void VFS_GetMemPath(char *Dest, void *Base, Uint Length)
 * \brief Create a VFS memory pointer path
 */
void VFS_GetMemPath(char *Dest, void *Base, Uint Length)
{
	Dest[0] = '$';
	itoa( &Dest[1], (tVAddr)Base, 16, BITS/4, '0' );
	Dest[BITS/4+1] = ':';
	itoa( &Dest[BITS/4+2], Length, 16, BITS/4, '0' );
	Dest[BITS/2+2] = '\0';
}

/**
 * \fn tVFS_Driver *VFS_GetFSByName(const char *Name)
 * \brief Gets a filesystem structure given a name
 */
tVFS_Driver *VFS_GetFSByName(const char *Name)
{
	tVFS_Driver	*drv = gVFS_Drivers;
	
	for(;drv;drv=drv->Next)
	{
//		Log("strcmp('%s' (%p), '%s') == 0?", drv->Name, drv->Name, Name);
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
	
	SHORTLOCK( &slDriverListLock );
	Info->Next = gVFS_Drivers;
	gVFS_Drivers = Info;
	SHORTREL( &slDriverListLock );
	
	VFS_UpdateDriverFile();
	
	return 0;
}

/**
 * \fn void VFS_UpdateDriverFile(void)
 * \brief Updates the driver list file
 */
void VFS_UpdateDriverFile(void)
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

void VFS_CleanupNode(tVFS_Node *Node)
{
	
}


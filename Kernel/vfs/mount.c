/* 
 * Acess Micro - VFS Server version 1
 */
#include <common.h>
#include <vfs.h>
#include <vfs_int.h>

// === GLOBALS ===
 int	glVFS_MountList = 0;
tVFS_Mount	*gMounts;
tVFS_Mount	*gRootMount = NULL;

// === CODE ===
/**
 * \fn int VFS_Mount(char *Device, char *MountPoint, char *Filesystem, char *ArgString)
 * \brief Mount a device
 * \param Device	Device string to mount
 * \param MountPoint	Destination for the mount
 * \param Filesystem	Filesystem to use for the mount
 * \param ArgString		Options to be passed to the filesystem
 * \return -1 on Invalid FS, -2 on No Mem, 0 on success
 */
int VFS_Mount(char *Device, char *MountPoint, char *Filesystem, char *ArgString)
{
	tVFS_Mount	*mnt;
	tVFS_Driver	*fs;
	 int	deviceLen = strlen(Device);
	 int	mountLen = strlen(MountPoint);
	 int	argLen = strlen(ArgString);
	
	// Get the filesystem
	fs = VFS_GetFSByName(Filesystem);
	if(!fs) {
		Warning("VFS_Mount - Unknown FS Type '%s'", Filesystem);
		return -1;
	}
	
	// Create mount information
	mnt = malloc( sizeof(tVFS_Mount)+deviceLen+1+mountLen+1+argLen+1 );
	if(!mnt) {
		return -2;
	}
	
	// HACK: Forces VFS_ParsePath to fall back on root  
	if(mountLen == 1 && MountPoint[0] == '/')
		mnt->MountPointLen = 0;
	else
		mnt->MountPointLen = mountLen;
	
	// Fill Structure
	mnt->Filesystem = fs;
	
	mnt->Device = &mnt->StrData[0];
	memcpy( mnt->Device, Device, deviceLen+1 );
	
	mnt->MountPoint = &mnt->StrData[deviceLen+1];
	memcpy( mnt->MountPoint, MountPoint, mountLen+1 );
	
	mnt->Options = &mnt->StrData[deviceLen+1+mountLen+1];
	memcpy( mnt->Options, ArgString, argLen+1 );
	
	// Initialise Volume
	mnt->RootNode = fs->InitDevice(Device, ArgString);
	if(!mnt->RootNode) {
		free(mnt);
		return -2;
	}
	
	// Set root
	if(!gRootMount)	gRootMount = mnt;
	
	// Add to mount list
	LOCK( &glVFS_MountList );
	mnt->Next = gMounts;
	gMounts = mnt;
	RELEASE( &glVFS_MountList );
	
	Log("VFS_Mount: Mounted '%s' to '%s' ('%s')", Device, MountPoint, Filesystem);
	
	return 0;
}

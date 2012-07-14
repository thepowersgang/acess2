/* 
 * Acess Micro - VFS Server version 1
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <vfs_int.h>
#include <fs_sysfs.h>

// === IMPORTS ===
extern int	giVFS_MountFileID;
extern char	*gsVFS_MountFile;

// === PROTOTYPES ===
#if 0
 int	VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options);
#endif
void	VFS_UpdateMountFile(void);

// === GLOBALS ===
tRWLock	glVFS_MountList;
tVFS_Mount	*gVFS_Mounts;
tVFS_Mount	*gVFS_RootMount = NULL;
Uint32	giVFS_NextMountIdent = 1;

// === CODE ===
/**
 * \brief Mount a device
 * \param Device	Device string to mount
 * \param MountPoint	Destination for the mount
 * \param Filesystem	Filesystem to use for the mount
 * \param Options		Options to be passed to the filesystem
 * \return -1 on Invalid FS, -2 on No Mem, 0 on success
 * 
 * Mounts the filesystem on \a Device at \a MountPoint using the driver
 * \a Filesystem. The options in the string \a Options is passed to the
 * driver's mount.
 */
int VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options)
{
	tVFS_Mount	*mnt, *parent_mnt;
	tVFS_Driver	*fs;
	 int	deviceLen = strlen(Device);
	 int	mountLen = strlen(MountPoint);
	 int	argLen = strlen(Options);
	
	// Get the filesystem
	fs = VFS_GetFSByName(Filesystem);
	if(!fs) {
		Log_Warning("VFS", "VFS_Mount - Unknown FS Type '%s'", Filesystem);
		return -1;
	}
	
	// Create mount information
	mnt = malloc( sizeof(tVFS_Mount)+deviceLen+1+mountLen+1+argLen+1 );
	if(!mnt) {
		return -2;
	}

	// Validate the mountpoint target
	// - Only if / is mounted
	if( gVFS_Mounts )
	{
		tVFS_Node *mpnode = VFS_ParsePath(MountPoint, NULL, &parent_mnt);
		if( !mpnode ) {
			Log_Warning("VFS", "VFS_Mount - Mountpoint '%s' does not exist", MountPoint);
			return -1;
		}
		if( mpnode->Type->Close )
			mpnode->Type->Close(mpnode);
		if( parent_mnt->RootNode == mpnode ) {
			Log_Warning("VFS", "VFS_Mount - Attempt to mount over '%s' (%s)",
				MountPoint, parent_mnt->MountPoint);
			return -1;
		}
	}
	
	// HACK: Forces VFS_ParsePath to fall back on root  
	if(mountLen == 1 && MountPoint[0] == '/')
		mnt->MountPointLen = 0;
	else
		mnt->MountPointLen = mountLen;
	
	// Fill Structure
	mnt->Filesystem = fs;
	mnt->OpenHandleCount = 0;
	
	mnt->Device = &mnt->StrData[0];
	memcpy( mnt->Device, Device, deviceLen+1 );
	
	mnt->MountPoint = &mnt->StrData[deviceLen+1];
	memcpy( mnt->MountPoint, MountPoint, mountLen+1 );
	
	mnt->Options = &mnt->StrData[deviceLen+1+mountLen+1];
	memcpy( mnt->Options, Options, argLen+1 );
	
	// Parse options string
	char	*str = mnt->Options;
	 int	nArg = 0;
	do {
		nArg ++;
	} while( (str = strchr(str, ',')) );

	char	*args[nArg + 1];
	str = mnt->Options;
	nArg = 0;
	do {
		args[nArg++] = str;
		str = strchr(str, ',');
		if(str)	*str = '\0';
	} while( str );
	args[nArg] = 0;	// NULL terminal
	
	// Initialise Volume
	mnt->RootNode = fs->InitDevice(Device, (const char **)args);
	if(!mnt->RootNode) {
		free(mnt);
		parent_mnt->OpenHandleCount --;
		return -2;
	}

	// Repair the options string
	while( nArg -- > 1 )
		args[nArg][-1] = ',';

	mnt->Identifier = giVFS_NextMountIdent++;
	#if 0
	// Ensure identifiers don't repeat
	// - Only a problem if there have been 4 billion mounts
	while( giVFS_NextMountIdent == 0 || VFS_GetMountByIdent(giVFS_NextMountIdent) )
		giVFS_NextMountIdent ++;
	#endif
	
	// Set root
	if(!gVFS_RootMount)	gVFS_RootMount = mnt;
	
	// Add to mount list
	RWLock_AcquireWrite( &glVFS_MountList );
	{
		mnt->Next = NULL;
		if(gVFS_Mounts) {
			tVFS_Mount	*tmp;
			for( tmp = gVFS_Mounts; tmp->Next; tmp = tmp->Next );
			tmp->Next = mnt;
		}
		else {
			gVFS_Mounts = mnt;
		}
	}
	RWLock_Release( &glVFS_MountList );
	
	Log_Log("VFS", "Mounted '%s' to '%s' ('%s')", Device, MountPoint, Filesystem);
	
	VFS_UpdateMountFile();
	
	return 0;
}

int VFS_Unmount(const char *Mountpoint)
{
	tVFS_Mount	*mount, *prev = NULL;
	RWLock_AcquireWrite( &glVFS_MountList );
	for( mount = gVFS_Mounts; mount; prev = mount, mount = mount->Next )
	{
		if( strcmp(Mountpoint, mount->MountPoint) == 0 ) {
			if( mount->OpenHandleCount ) {
				LOG("Mountpoint busy");
				RWLock_Release(&glVFS_MountList);
				return EBUSY;
			}
			if(prev)
				prev->Next = mount->Next;
			else
				gVFS_Mounts = mount->Next;
			break;
		}
	}
	RWLock_Release( &glVFS_MountList );
	if( !mount ) {
		LOG("Mountpoint not found");
		return ENOENT;
	}
	
	Log_Warning("VFS", "TODO: Impliment unmount");

	// Decrease the open handle count for the mountpoint filesystem.
	tVFS_Mount	*mpmnt;
	tVFS_Node *mpnode = VFS_ParsePath(mount->MountPoint, NULL, &mpmnt);
	if(mpnode)
	{
		mpmnt->OpenHandleCount -= 2;	// -1 for _ParsePath here, -1 for in _Mount
	}

	mount->Filesystem->Unmount( mount->RootNode );
	free(mount);

	return EOK;
}

int VFS_UnmountAll(void)
{
	 int	nUnmounted = 0;
	tVFS_Mount	*mount, *prev = NULL, *next;

	RWLock_AcquireWrite( &glVFS_MountList );
	// If we've unmounted the final filesystem, all good
	if( gVFS_Mounts == NULL) {
		RWLock_Release( &glVFS_MountList );
		return -1;
	}

	for( mount = gVFS_Mounts; mount; prev = mount, mount = next )
	{
		next = mount->Next;
		// Can't unmount stuff with open handles
		if( mount->OpenHandleCount > 0 ) {
			LOG("%p (%s) has open handles (%i of them)",
				mount, mount->MountPoint, mount->OpenHandleCount);
			continue;
		}
		
		if(prev)
			prev->Next = mount->Next;
		else
			gVFS_Mounts = mount->Next;
		
		if( mount->Filesystem->Unmount ) {
			mount->Filesystem->Unmount( mount->RootNode );
		}
		else {
			Log_Error("VFS", "%s (%s) does not have an unmount method, not calling it",
				mount->MountPoint, mount->Filesystem->Name);
		}
		free(mount);
		mount = prev;
		nUnmounted ++;
	}
	RWLock_Release( &glVFS_MountList );

	return nUnmounted;
}

/**
 * \brief Gets a mount point given the identifier
 */
tVFS_Mount *VFS_GetMountByIdent(Uint32 MountID)
{
	tVFS_Mount	*mnt;
	
	RWLock_AcquireRead(&glVFS_MountList);
	for(mnt = gVFS_Mounts; mnt; mnt = mnt->Next)
	{
		if(mnt->Identifier == MountID)
			break;
	}
	if(mnt)
		mnt->OpenHandleCount ++;
	RWLock_Release(&glVFS_MountList);
	return mnt;
}

/**
 * \brief Updates the mount file buffer
 * 
 * Updates the ProcFS mounts file buffer to match the current mounts list.
 */
void VFS_UpdateMountFile(void)
{
	 int	len = 0;
	char	*buf;
	tVFS_Mount	*mnt;
	
	// Format:
	// <device>\t<location>\t<type>\t<options>\n
	
	RWLock_AcquireRead( &glVFS_MountList );
	for(mnt = gVFS_Mounts; mnt; mnt = mnt->Next)
	{
		len += 4 + strlen(mnt->Device) + strlen(mnt->MountPoint)
			+ strlen(mnt->Filesystem->Name) + strlen(mnt->Options);
	}
	RWLock_Release( &glVFS_MountList );
	
	buf = malloc( len + 1 );
	len = 0;
	RWLock_AcquireRead( &glVFS_MountList );
	for(mnt = gVFS_Mounts; mnt; mnt = mnt->Next)
	{
		strcpy( &buf[len], mnt->Device );
		len += strlen(mnt->Device);
		buf[len++] = '\t';
		
		strcpy( &buf[len], mnt->MountPoint );
		len += strlen(mnt->MountPoint);
		buf[len++] = '\t';
		
		strcpy( &buf[len], mnt->Filesystem->Name );
		len += strlen(mnt->Filesystem->Name);
		buf[len++] = '\t';
		
		strcpy( &buf[len], mnt->Options );
		len += strlen(mnt->Options);
		buf[len++] = '\n';
	}
	RWLock_Release( &glVFS_MountList );
	buf[len] = 0;
	
	SysFS_UpdateFile( giVFS_MountFileID, buf, len );
	if( gsVFS_MountFile )	free( gsVFS_MountFile );
	gsVFS_MountFile = buf;
}

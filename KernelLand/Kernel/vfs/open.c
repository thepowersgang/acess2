/*
 * Acess2 VFS
 * - Open, Close and ChDir
 */
#define SANITY	1
#define DEBUG	0
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"
#include <threads.h>

// === CONSTANTS ===
#define	OPEN_MOUNT_ROOT	1
#define MAX_PATH_SLASHES	256
#define MAX_NESTED_LINKS	4
#define MAX_PATH_LEN	255
#define MAX_MARSHALLED_HANDLES	16	// Max outstanding

// === IMPORTS ===
extern tVFS_Mount	*gVFS_RootMount;
extern tVFS_Node	*VFS_MemFile_Create(const char *Path);

// === TYPES ===
typedef struct sVFS_MarshaledHandle
{
	Uint32	Magic;
	tTime	AllocTime;
	tVFS_Handle	Handle;
} tVFS_MarshalledHandle;

// === PROTOTYPES ===
void	_ReferenceMount(tVFS_Mount *Mount, const char *DebugTag);
void	_DereferenceMount(tVFS_Mount *Mount, const char *DebugTag);
 int	VFS_int_CreateHandle(tVFS_Node *Node, tVFS_Mount *Mount, int Mode);

// === GLOBALS ===
tMutex	glVFS_MarshalledHandles;
tVFS_MarshalledHandle	gaVFS_MarshalledHandles[MAX_MARSHALLED_HANDLES];

// === CODE ===
void _ReferenceMount(tVFS_Mount *Mount, const char *DebugTag)
{
//	Log_Debug("VFS", "%s: inc. mntpt '%s' to %i", DebugTag, Mount->MountPoint, Mount->OpenHandleCount+1);
	Mount->OpenHandleCount ++;
}
void _DereferenceMount(tVFS_Mount *Mount, const char *DebugTag)
{
//	Log_Debug("VFS", "%s: dec. mntpt '%s' to %i", DebugTag, Mount->MountPoint, Mount->OpenHandleCount-1);
	ASSERT(Mount->OpenHandleCount > 0);
	Mount->OpenHandleCount --;
}
/**
 * \fn char *VFS_GetAbsPath(const char *Path)
 * \brief Create an absolute path from a relative one
 */
char *VFS_GetAbsPath(const char *Path)
{
	char	*ret;
	 int	pathLen = strlen(Path);
	char	*pathComps[MAX_PATH_SLASHES];
	char	*tmpStr;
	int		iPos = 0;
	int		iPos2 = 0;
	const char	*chroot = *Threads_GetChroot(NULL);
	 int	chrootLen;
	const char	*cwd = *Threads_GetCWD(NULL);
	 int	cwdLen;
	
	ENTER("sPath", Path);
	
	// Memory File
	if(Path[0] == '$') {
		ret = malloc(strlen(Path)+1);
		if(!ret) {
			Log_Warning("VFS", "VFS_GetAbsPath: malloc() returned NULL");
			return NULL;
		}
		strcpy(ret, Path);
		LEAVE('p', ret);
		return ret;
	}
	
	// - Fetch ChRoot
	if( chroot == NULL )
		chroot = "";
	chrootLen = strlen(chroot);
	// Trim trailing slash off chroot
	if( chrootLen && chroot[chrootLen - 1] == '/' )
		chrootLen -= 1;
	
	// Check if the path is already absolute
	if(Path[0] == '/') {
		ret = malloc(chrootLen + pathLen + 1);
		if(!ret) {
			Log_Warning("VFS", "VFS_GetAbsPath: malloc() returned NULL");
			return NULL;
		}
		strcpy(ret + chrootLen, Path);
	}
	else {
		if(cwd == NULL) {
			cwd = "/";
			cwdLen = 1;
		}
		else {
			cwdLen = strlen(cwd);
		}
		// Prepend the current directory
		ret = malloc(chrootLen + cwdLen + 1 + pathLen + 1 );
		strcpy(ret+chrootLen, cwd);
		ret[cwdLen] = '/';
		strcpy(ret+chrootLen+cwdLen+1, Path);
		//Log("ret = '%s'", ret);
	}
	
	// Parse Path
	pathComps[iPos++] = tmpStr = ret+chrootLen+1;
	while(*tmpStr)
	{
		if(*tmpStr++ == '/')
		{
			pathComps[iPos++] = tmpStr;
			if(iPos == MAX_PATH_SLASHES) {
				LOG("Path '%s' has too many elements", Path);
				free(ret);
				LEAVE('n');
				return NULL;
			}
		}
	}
	pathComps[iPos] = NULL;
	
	// Cleanup
	iPos2 = iPos = 0;
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		// Always Increment iPos
		iPos++;
		// ..
		if(tmpStr[0] == '.' && tmpStr[1] == '.'	&& (tmpStr[2] == '/' || tmpStr[2] == '\0') )
		{
			if(iPos2 != 0)
				iPos2 --;
			continue;
		}
		// .
		if(tmpStr[0] == '.' && (tmpStr[1] == '/' || tmpStr[1] == '\0') )
		{
			continue;
		}
		// Empty
		if(tmpStr[0] == '/' || tmpStr[0] == '\0')
		{
			continue;
		}
		
		// Set New Position
		pathComps[iPos2] = tmpStr;
		iPos2++;
	}
	pathComps[iPos2] = NULL;
	
	// Build New Path
	iPos2 = chrootLen + 1;	iPos = 0;
	ret[0] = '/';
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		while(*tmpStr && *tmpStr != '/')
		{
			ret[iPos2++] = *tmpStr;
			tmpStr++;
		}
		ret[iPos2++] = '/';
		iPos++;
	}
	if(iPos2 > 1)
		ret[iPos2-1] = 0;
	else
		ret[iPos2] = 0;

	// Prepend the chroot
	if(chrootLen)
		memcpy( ret, chroot, chrootLen );
	
	LEAVE('s', ret);
//	Log_Debug("VFS", "VFS_GetAbsPath: RETURN '%s'", ret);
	return ret;
}

/**
 * \fn char *VFS_ParsePath(const char *Path, char **TruePath)
 * \brief Parses a path, resolving sysmlinks and applying permissions
 */
tVFS_Node *VFS_ParsePath(const char *Path, char **TruePath, tVFS_Mount **MountPoint)
{
	tVFS_Mount	*mnt, *longestMount;
	 int	cmp, retLength = 0;
	 int	ofs, nextSlash;
	 int	iNestedLinks = 0;
	tVFS_Node	*curNode, *tmpNode;
	char	*tmp;
	char	path_buffer[MAX_PATH_LEN+1];
	
	ENTER("sPath pTruePath", Path, TruePath);
	
	// HACK: Memory File
	if(Threads_GetUID() == 0 && Path[0] == '$') {
		if(TruePath) {
			*TruePath = malloc(strlen(Path)+1);
			strcpy(*TruePath, Path);
		}
		curNode = VFS_MemFile_Create(Path);
		if(MountPoint) {
			*MountPoint = NULL;
		}
		LEAVE('p', curNode);
		return curNode;
	}

restart_parse:	
	// For root we always fast return
	if(Path[0] == '/' && Path[1] == '\0') {
		if(TruePath) {
			*TruePath = malloc( gVFS_RootMount->MountPointLen+1 );
			strcpy(*TruePath, gVFS_RootMount->MountPoint);
		}
		_ReferenceMount(gVFS_RootMount, "ParsePath - Fast Tree Root");
		if(MountPoint)	*MountPoint = gVFS_RootMount;
		LEAVE('p', gVFS_RootMount->RootNode);
		return gVFS_RootMount->RootNode;
	}
	
	// Check if there is anything mounted
	if(!gVFS_Mounts) {
		Log_Error("VFS", "VFS_ParsePath - No filesystems mounted");
		return NULL;
	}
	
	// Find Mountpoint
	longestMount = gVFS_RootMount;
	RWLock_AcquireRead( &glVFS_MountList );
	for(mnt = gVFS_Mounts; mnt; mnt = mnt->Next)
	{
		// Quick Check
		if( Path[mnt->MountPointLen] != '/' && Path[mnt->MountPointLen] != '\0')
			continue;
		// Length Check - If the length is smaller than the longest match sofar
		if(mnt->MountPointLen < longestMount->MountPointLen)	continue;
		// String Compare
		cmp = strncmp(Path, mnt->MountPoint, mnt->MountPointLen);
		// Not a match, continue
		if(cmp != 0)	continue;
		
		#if OPEN_MOUNT_ROOT
		// Fast Break - Request Mount Root
		if(Path[mnt->MountPointLen] == '\0') {
			if(TruePath) {
				*TruePath = malloc( mnt->MountPointLen+1 );
				strcpy(*TruePath, mnt->MountPoint);
			}
			if(MountPoint)
				*MountPoint = mnt;
			RWLock_Release( &glVFS_MountList );
			LOG("Mount %p root", mnt);
			_ReferenceMount(mnt, "ParsePath - Mount Root");
			LEAVE('p', mnt->RootNode);
			return mnt->RootNode;
		}
		#endif
		longestMount = mnt;
	}
	if(!longestMount) {
		Log_Panic("VFS", "VFS_ParsePath - No mount for '%s'", Path);
		return NULL;
	}
	
	_ReferenceMount(longestMount, "ParsePath");
	RWLock_Release( &glVFS_MountList );
	
	// Save to shorter variable
	mnt = longestMount;
	
	LOG("mnt = {MountPoint:\"%s\"}", mnt->MountPoint);
	
	// Initialise String
	if(TruePath)
	{
		// Assumes that the resultant path (here) will not be > strlen(Path) + 1
		*TruePath = malloc( strlen(Path) + 1 );
		strcpy(*TruePath, mnt->MountPoint);
		retLength = mnt->MountPointLen;
	}
	
	curNode = mnt->RootNode;
	curNode->ReferenceCount ++;	
	// Parse Path
	ofs = mnt->MountPointLen+1;
	for(; (nextSlash = strpos(&Path[ofs], '/')) != -1; ofs += nextSlash + 1)
	{
		char	pathEle[nextSlash+1];
		
		// Empty String
		if(nextSlash == 0)	continue;
		
		memcpy(pathEle, &Path[ofs], nextSlash);
		pathEle[nextSlash] = 0;
	
		// Check permissions on root of filesystem
		if( !VFS_CheckACL(curNode, VFS_PERM_EXEC) ) {
			LOG("Permissions failure on '%s'", Path);
			errno = EPERM;
			goto _error;
		}
		
		// Check if the node has a FindDir method
		if( !curNode->Type )
		{
			LOG("Finddir failure on '%s' - No type", Path);
			Log_Error("VFS", "Node at '%s' has no type (mount %s:%s)",
				Path, mnt->Filesystem->Name, mnt->MountPoint);
			goto _error;
		}
		if( !curNode->Type->FindDir )
		{
			LOG("Finddir failure on '%s' - No FindDir method in %s", Path, curNode->Type->TypeName);
			goto _error;
		}
		LOG("FindDir{=%p}(%p, '%s')", curNode->Type->FindDir, curNode, pathEle);
		// Get Child Node
		tmpNode = curNode->Type->FindDir(curNode, pathEle, 0);
		LOG("tmpNode = %p", tmpNode);
		_CloseNode( curNode );
		curNode = tmpNode;
		
		// Error Check
		if(!curNode) {
			LOG("Node '%s' not found in dir '%s'", pathEle, Path);
			errno = ENOENT;
			goto _error;
		}
		
		// Handle Symbolic Links
		if(curNode->Flags & VFS_FFLAG_SYMLINK) {
			if(TruePath) {
				free(*TruePath);
				*TruePath = NULL;
			}
			if(!curNode->Type || !curNode->Type->Read) {
				Log_Warning("VFS", "VFS_ParsePath - Read of symlink node %p'%s' is NULL",
					curNode, Path);
				errno = EINTERNAL;
				goto _error;
			}
			
			if(iNestedLinks > MAX_NESTED_LINKS) {
				Log_Notice("VFS", "VFS_ParsePath - Nested link limit exceeded");
				errno = ENOENT;
				goto _error;
			}
			
			// Parse Symlink Path
			// - Just update the path variable and restart the function
			// > Count nested symlinks and limit to some value (counteracts loops)
			{
				 int	remlen = strlen(Path) - (ofs + nextSlash);
				if( curNode->Size + remlen > MAX_PATH_LEN ) {
					Log_Warning("VFS", "VFS_ParsePath - Symlinked path too long");
					errno = ENOENT;
					goto _error;
				}
				curNode->Type->Read( curNode, 0, curNode->Size, path_buffer, 0 );
				path_buffer[ curNode->Size ] = '\0';
				LOG("path_buffer = '%s'", path_buffer);
				strcat(path_buffer, Path + ofs+nextSlash);
				// TODO: Pass to VFS_GetAbsPath to handle ../. in the symlink
				
				Path = path_buffer;
//				Log_Debug("VFS", "VFS_ParsePath: Symlink translated to '%s'", Path);
				iNestedLinks ++;
			}

			// EVIL: Goto :)
			LOG("Symlink -> '%s', restart", Path);
			_DereferenceMount(mnt, "ParsePath - sym");
			goto restart_parse;
		}
		
		// Handle Non-Directories
		if( !(curNode->Flags & VFS_FFLAG_DIRECTORY) )
		{
			Log_Warning("VFS", "VFS_ParsePath - Path segment is not a directory");
			errno = ENOTDIR;
			goto _error;
		}
		
		// Check if path needs extending
		if(!TruePath)	continue;
		
		// Increase buffer space
		tmp = realloc( *TruePath, retLength + strlen(pathEle) + 1 + 1 );
		// Check if allocation succeeded
		if(!tmp) {
			Log_Warning("VFS", "VFS_ParsePath - Unable to reallocate true path buffer");
			errno = ENOMEM;
			goto _error;
		}
		*TruePath = tmp;
		// Append to path
		(*TruePath)[retLength] = '/';
		strcpy(*TruePath+retLength+1, pathEle);
		
		LOG("*TruePath = '%s'", *TruePath);
		
		// - Extend Path
		retLength += nextSlash + 1;
	}

	// Check final finddir call	
	if( !curNode->Type || !curNode->Type->FindDir ) {
		Log_Warning("VFS", "VFS_ParsePath - FindDir doesn't exist for element of '%s'", Path);
		errno = ENOENT;
		goto _error;
	}
	
	// Get last node
	LOG("FindDir(%p, '%s')", curNode, &Path[ofs]);
	tmpNode = curNode->Type->FindDir(curNode, &Path[ofs], 0);
	LOG("tmpNode = %p", tmpNode);
	// Check if file was found
	if(!tmpNode) {
		LOG("Node '%s' not found in dir '%.*s'", &Path[ofs], ofs, Path);
		errno = ENOENT;
		goto _error;
	}
	_CloseNode( curNode );
	
	if(TruePath)
	{
		// Increase buffer space
		tmp = realloc(*TruePath, retLength + strlen(&Path[ofs]) + 1 + 1);
		// Check if allocation succeeded
		if(!tmp) {
			Log_Warning("VFS", "VFS_ParsePath -  Unable to reallocate true path buffer");
			errno = ENOMEM;
			goto _error;
		}
		*TruePath = tmp;
		// Append to path
		(*TruePath)[retLength] = '/';
		strcpy(*TruePath + retLength + 1, &Path[ofs]);
		// - Extend Path
		//retLength += strlen(tmpNode->Name) + 1;
	}

	if( MountPoint ) {
		*MountPoint = mnt;
	}
	
	// Leave the mointpoint's count increased
	
	LEAVE('p', tmpNode);
	return tmpNode;

_error:
	_CloseNode( curNode );
	
	if(TruePath && *TruePath) {
		free(*TruePath);
		*TruePath = NULL;
	}
	// Open failed, so decrement the open handle count
	_DereferenceMount(mnt, "ParsePath - error");
	LEAVE('n');
	return NULL;
}

/**
 * \brief Create and return a handle number for the given node and mode
 */
int VFS_int_CreateHandle( tVFS_Node *Node, tVFS_Mount *Mount, int Mode )
{
	 int	i;
	
	ENTER("pNode pMount xMode", Node, Mount, Mode);

	i = 0;
	i |= (Mode & VFS_OPENFLAG_EXEC) ? VFS_PERM_EXEC : 0;
	i |= (Mode & VFS_OPENFLAG_READ) ? VFS_PERM_READ : 0;
	i |= (Mode & VFS_OPENFLAG_WRITE) ? VFS_PERM_WRITE : 0;
	
	LOG("i = 0b%b", i);
	
	// Permissions Check
	if( !VFS_CheckACL(Node, i) ) {
		_CloseNode( Node );
		Log_Log("VFS", "VFS_int_CreateHandle: Permissions Failed");
		errno = EACCES;
		LEAVE_RET('i', -1);
	}

	if( MM_GetPhysAddr(Node->Type) == 0 ) {
		Log_Error("VFS", "Node %p from mount '%s' (%s) has a bad type (%p)",
			Node, Mount->MountPoint, Mount->Filesystem->Name, Node->Type);
		errno = EINTERNAL;
		LEAVE_RET('i', -1);
	}
	
	i = VFS_AllocHandle( !!(Mode & VFS_OPENFLAG_USER), Node, Mode );
	if( i < 0 ) {
		Log_Notice("VFS", "VFS_int_CreateHandle: Out of handles");
		errno = ENFILE;
		LEAVE_RET('i', -1);
	}

	VFS_GetHandle(i)->Mount = Mount;

	LEAVE_RET('x', i);
}

/**
 * \fn int VFS_Open(const char *Path, Uint Mode)
 * \brief Open a file
 */
int VFS_Open(const char *Path, Uint Flags)
{
	return VFS_OpenEx(Path, Flags, 0);
}

int VFS_OpenEx(const char *Path, Uint Flags, Uint Mode)
{
	tVFS_Node	*node;
	tVFS_Mount	*mnt;
	char	*absPath;
	
	ENTER("sPath xFlags oMode", Path, Flags);
	
	// Get absolute path
	absPath = VFS_GetAbsPath(Path);
	if(absPath == NULL) {
		Log_Warning("VFS", "VFS_Open: Path expansion failed '%s'", Path);
		LEAVE_RET('i', -1);
	}
	LOG("absPath = \"%s\"", absPath);
	
	// Parse path and get mount point
	node = VFS_ParsePath(absPath, NULL, &mnt);
	
	// Create file if requested and it doesn't exist
	if( !node && (Flags & VFS_OPENFLAG_CREATE) )
	{
		// TODO: Translate `Mode` into ACL and node flags
		Uint	new_flags = 0;
		
		// Split path at final separator
		char *file = strrchr(absPath, '/');
		*file = '\0';
		file ++;

		// Get parent node
		tVFS_Mount	*pmnt;
		tVFS_Node *pnode = VFS_ParsePath(absPath, NULL, &pmnt);
		if(!pnode) {
			LOG("Unable to open parent '%s'", absPath);
			free(absPath);
			errno = ENOENT;
			LEAVE_RET('i', -1);
		}

		// Check ACLs on the parent
		if( !VFS_CheckACL(pnode, VFS_PERM_EXEC|VFS_PERM_WRITE) ) {
			errno = EACCES;
			goto _pnode_err;
		}

		// Check that there's a MkNod method
		if( !pnode->Type || !pnode->Type->MkNod ) {
			Log_Warning("VFS", "VFS_Open - Directory has no MkNod method");
			errno = EINVAL;
			goto _pnode_err;
		}
		
		node = pnode->Type->MkNod(pnode, file, new_flags);
		if( !node ) {
			LOG("Cannot create node '%s' in '%s'", file, absPath);
			errno = ENOENT;
			goto _pnode_err;
		}
		// Set mountpoint (and increment open handle count)
		mnt = pmnt;
		_ReferenceMount(mnt, "Open - create");
		// Fall through on error check
		
		_CloseNode(pnode);
		_DereferenceMount(pmnt, "Open - create");
		goto _pnode_ok;

	_pnode_err:
		if( pnode ) {
			_CloseNode(pnode);
			_DereferenceMount(pmnt, "Open - create,fail");
			free(absPath);
		}
		LEAVE('i', -1);
		return -1;
	}
	_pnode_ok:
	
	// Free generated path
	free(absPath);
	
	// Check for error
	if(!node)
	{
		LOG("Cannot find node");
		errno = ENOENT;
		goto _error;
	}
	
	// Check for symlinks
	if( !(Flags & VFS_OPENFLAG_NOLINK) && (node->Flags & VFS_FFLAG_SYMLINK) )
	{
		char	tmppath[node->Size+1];
		if( node->Size > MAX_PATH_LEN ) {
			Log_Warning("VFS", "VFS_Open - Symlink is too long (%i)", node->Size);
			goto _error;
		}
		if( !node->Type || !node->Type->Read ) {
			Log_Warning("VFS", "VFS_Open - No read method on symlink");
			goto _error;
		}
		// Read symlink's path
		node->Type->Read( node, 0, node->Size, tmppath, 0 );
		tmppath[ node->Size ] = '\0';
		_CloseNode( node );
		_DereferenceMount(mnt, "Open - symlink");
		// Open the target
		node = VFS_ParsePath(tmppath, NULL, &mnt);
		if(!node) {
			LOG("Cannot find symlink target node (%s)", tmppath);
			errno = ENOENT;
			goto _error;
		}
	}

	 int	ret = VFS_int_CreateHandle(node, mnt, Flags);
	LEAVE_RET('x', ret);
_error:
	if( node )
	{
		_DereferenceMount(mnt, "Open - error");
		_CloseNode(node);
	}
	LEAVE_RET('i', -1);
}


/**
 * \brief Open a file from an open directory
 */
int VFS_OpenChild(int FD, const char *Name, Uint Mode)
{
	tVFS_Handle	*h;
	tVFS_Node	*node;
	
	ENTER("xFD sName xMode", FD, Name, Mode);

	// Get handle
	h = VFS_GetHandle(FD);
	if(h == NULL) {
		Log_Warning("VFS", "VFS_OpenChild - Invalid file handle 0x%x", FD);
		errno = EINVAL;
		LEAVE_RET('i', -1);
	}
	
	// Check for directory
	if( !(h->Node->Flags & VFS_FFLAG_DIRECTORY) ) {
		Log_Warning("VFS", "VFS_OpenChild - Passed handle is not a directory");
		errno = ENOTDIR;
		LEAVE_RET('i', -1);
	}

	// Sanity check
	if( !h->Node->Type || !h->Node->Type->FindDir ) {
		Log_Error("VFS", "VFS_OpenChild - Node does not have a type/is missing FindDir");
		errno = ENOTDIR;
		LEAVE_RET('i', -1);
	}
	
	// Find Child
	node = h->Node->Type->FindDir(h->Node, Name, 0);
	if(!node) {
		errno = ENOENT;
		LEAVE_RET('i', -1);
	}

	// Increment open handle count, no problems with the mount going away as `h` is already open on it
	_ReferenceMount(h->Mount, "OpenChild");

	LEAVE_RET('x', VFS_int_CreateHandle(node, h->Mount, Mode));
}

int VFS_OpenInode(Uint32 Mount, Uint64 Inode, int Mode)
{
	tVFS_Mount	*mnt;
	tVFS_Node	*node;

	ENTER("iMount XInode xMode", Mount, Inode, Mode);
	
	// Get mount point
	mnt = VFS_GetMountByIdent(Mount);
	if( !mnt ) {
		LOG("Mount point ident invalid");
		errno = ENOENT;
		LEAVE_RET('i', -1);
	}
	
	// Does the filesystem support this?
	if( !mnt->Filesystem->GetNodeFromINode ) {
		Log_Notice("VFS", "Filesystem '%s' does not support inode accesses",
			mnt->Filesystem->Name);
		errno = ENOENT;
		LEAVE_RET('i', -1);
	}

	// Get node
	node = mnt->Filesystem->GetNodeFromINode(mnt->RootNode, Inode);
	if( !node ) {
		LOG("Unable to find inode");
		errno = ENOENT;
		LEAVE_RET('i', -1);
	}
	
	LEAVE_RET('x', VFS_int_CreateHandle(node, mnt, Mode));
}

int VFS_Reopen(int FD, const char *Path, int Flags)
{
	tVFS_Handle	*h = VFS_GetHandle(FD);
	if(!h) {
		errno = EBADF;
		return -1;
	}

	int newf = VFS_Open(Path, Flags);
	if( newf == -1 ) {
		// errno = set by VFS_Open
		return -1;
	}
	
	_CloseNode(h->Node);
	_DereferenceMount(h->Mount, "Reopen");
	memcpy(h, VFS_GetHandle(newf), sizeof(*h));
	_ReferenceNode(h->Node);
	_ReferenceMount(h->Mount, "Reopen");
	
	VFS_Close(newf);	

	return FD;
}

/**
 * \fn void VFS_Close(int FD)
 * \brief Closes an open file handle
 */
void VFS_Close(int FD)
{
	tVFS_Handle	*h;
	
	// Get handle
	h = VFS_GetHandle(FD);
	if(h == NULL) {
		Log_Warning("VFS", "Invalid file handle passed to VFS_Close, 0x%x", FD);
		errno = EINVAL;
		return;
	}

	if( h->Node == NULL ) {
		Log_Warning("VFS", "Non-open handle passed to VFS_Close, 0x%x", FD);
		errno = EINVAL;
		return ;
	}	

	#if VALIDATE_VFS_FUNCTIPONS
	if(h->Node->Close && !MM_GetPhysAddr(h->Node->Close)) {
		Log_Warning("VFS", "Node %p's ->Close method is invalid (%p)",
			h->Node, h->Node->Close);
		errno = EINTERNAL;
		return ;
	}
	#endif
	
	LOG("Handle %x", FD);
	_CloseNode(h->Node);

	if( h->Mount ) {
		_DereferenceMount(h->Mount, "Close");
	}

	h->Node = NULL;
}

int VFS_DuplicateFD(int SrcFD, int DstFD)
{
	 int	isUser = !(SrcFD & VFS_KERNEL_FLAG);
	tVFS_Handle	*src = VFS_GetHandle(SrcFD);
	if( !src )	return -1;
	if( DstFD == -1 ) {
		DstFD = VFS_AllocHandle(isUser, src->Node, src->Mode);
	}
	else {
		// Can't overwrite
		if( VFS_GetHandle(DstFD) )
			return -1;
		VFS_SetHandle(DstFD, src->Node, src->Mode);
	}
	_ReferenceMount(src->Mount, "DuplicateFD");
	_ReferenceNode(src->Node);
	memcpy(VFS_GetHandle(DstFD), src, sizeof(tVFS_Handle));
	return DstFD;
}

/*
 * Update flags on a FD
 */
int VFS_SetFDFlags(int FD, int Mask, int Value)
{
	tVFS_Handle	*h = VFS_GetHandle(FD);
	if(!h) {
		errno = EBADF;
		return -1;
	}
	 int	ret = h->Mode;
	
	Value &= Mask;
	h->Mode &= ~Mask;
	h->Mode |= Value;
	return ret;
}

/**
 * \brief Change current working directory
 */
int VFS_ChDir(const char *Dest)
{
	char	*buf;
	 int	fd;
	tVFS_Handle	*h;
	
	// Create Absolute
	buf = VFS_GetAbsPath(Dest);
	if(buf == NULL) {
		Log_Notice("VFS", "VFS_ChDir: Path expansion failed");
		return -1;
	}
	
	// Check if path exists
	fd = VFS_Open(buf, VFS_OPENFLAG_EXEC);
	if(fd == -1) {
		Log_Notice("VFS", "VFS_ChDir: Path is invalid");
		return -1;
	}
	
	// Get node so we can check for directory
	h = VFS_GetHandle(fd);
	if( !(h->Node->Flags & VFS_FFLAG_DIRECTORY) ) {
		Log("VFS_ChDir: Path is not a directory");
		VFS_Close(fd);
		return -1;
	}
	
	// Close file
	VFS_Close(fd);
	
	{
		char	**cwdptr = Threads_GetCWD(NULL);
		// Free old working directory
		if( *cwdptr )	free( *cwdptr );
		// Set new
		*cwdptr = buf;
	}
	
	Log_Debug("VFS", "Updated CWD to '%s'", buf);
	
	return 1;
}

/**
 * \fn int VFS_ChRoot(char *New)
 * \brief Change current root directory
 */
int VFS_ChRoot(const char *New)
{
	char	*buf;
	 int	fd;
	tVFS_Handle	*h;
	
	if(New[0] == '/' && New[1] == '\0')
		return 1;	// What a useless thing to ask!
	
	// Create Absolute
	buf = VFS_GetAbsPath(New);
	if(buf == NULL) {
		LOG("Path expansion failed");
		return -1;
	}
	
	// Check if path exists
	fd = VFS_Open(buf, VFS_OPENFLAG_EXEC);
	if(fd == -1) {
		LOG("Path is invalid");
		return -1;
	}
	
	// Get node so we can check for directory
	h = VFS_GetHandle(fd);
	if( !(h->Node->Flags & VFS_FFLAG_DIRECTORY) ) {
		LOG("Path is not a directory");
		VFS_Close(fd);
		return -1;
	}
	
	// Close file
	VFS_Close(fd);

	// Update	
	{
		char	**chroot_ptr = Threads_GetChroot(NULL);
		if( *chroot_ptr )	free( *chroot_ptr );
		*chroot_ptr = buf;
	}
	
	LOG("Updated Root to '%s'", buf);
	
	return 1;
}

/*
 * Marshal a handle so that it can be transferred between processes
 */
Uint64 VFS_MarshalHandle(int FD)
{
	tVFS_Handle	*h = VFS_GetHandle(FD);
	if(!h || !h->Node) {
		errno = EBADF;
		return -1;
	}
	
	// Allocate marshal location
	 int	ret = -1;
	Mutex_Acquire(&glVFS_MarshalledHandles);
	for( int i = 0; i < MAX_MARSHALLED_HANDLES; i ++ )
	{
		tVFS_MarshalledHandle*	mh = &gaVFS_MarshalledHandles[i];
		if( mh->Handle.Node == NULL ) {
			mh->Handle.Node = h->Node;
			mh->AllocTime = now();
			ret = i;
		}
		if( now() - mh->AllocTime > 2000 ) {
			Log_Notice("VFS", "TODO: Expire marshalled handle");
		}
	}
	Mutex_Release(&glVFS_MarshalledHandles);
	if( ret < 0 ) {
		// TODO: Need to clean up lost handles to avoid DOS
		Log_Warning("VFS", "Out of marshaled handle slots");
		errno = EAGAIN;
		return -1;
	}
	
	// Populate
	tVFS_MarshalledHandle*	mh = &gaVFS_MarshalledHandles[ret];
	mh->Handle = *h;
	_ReferenceMount(h->Mount, "MarshalHandle");
	_ReferenceNode(h->Node);
	mh->Magic = rand();
	
	return (Uint64)mh->Magic << 32 | ret;
}

/*
 * Un-marshal a handle into the current process
 * NOTE: Does not support unmarshalling into kernel handle list
 */
int VFS_UnmarshalHandle(Uint64 Handle)
{
	Uint32	magic = Handle >> 32;
	 int	id = Handle & 0xFFFFFFFF;
	
	// Range check
	if( id >= MAX_MARSHALLED_HANDLES ) {
		LOG("ID too high (%i > %i)", id, MAX_MARSHALLED_HANDLES);
		errno = EINVAL;
		return -1;
	}
	
	
	// Check validity
	tVFS_MarshalledHandle	*mh = &gaVFS_MarshalledHandles[id];
	if( mh->Handle.Node == NULL ) {
		LOG("Target node is NULL");
		errno = EINVAL;
		return -1;
	}
	if( mh->Magic != magic ) {
		LOG("Magic mismatch (0x%08x != 0x%08x)", magic, mh->Magic);
		errno = EACCES;
		return -1;
	}
	
	Mutex_Acquire(&glVFS_MarshalledHandles);
	// - Create destination handle
	 int	ret = VFS_AllocHandle(true, mh->Handle.Node, mh->Handle.Mode);
	// - Clear allocation
	mh->Handle.Node = NULL;
	Mutex_Release(&glVFS_MarshalledHandles);
	if( ret == -1 ) {
		errno = ENFILE;
		return -1;
	}
	
	// No need to reference node/mount, new handle takes marshalled reference
	
	return ret;
}

// === EXPORTS ===
EXPORT(VFS_Open);
EXPORT(VFS_Close);

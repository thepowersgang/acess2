/*
 * AcessMicro VFS
 * - Open, Close and ChDir
 */
#define DEBUG	0
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"

// === CONSTANTS ===
#define	OPEN_MOUNT_ROOT	1
#define MAX_KERNEL_FILES	128
#define MAX_PATH_SLASHES	256

// === IMPORTS ===
extern tVFS_Node	gVFS_MemRoot;
extern tVFS_Mount	*gRootMount;

// === GLOBALS ===
tVFS_Handle	*gaUserHandles = (void*)MM_PPD_VFS;
tVFS_Handle	*gaKernelHandles = (void*)MM_KERNEL_VFS;

// === CODE ===
/**
 * \fn char *VFS_GetAbsPath(char *Path)
 * \brief Create an absolute path from a relative one
 */
char *VFS_GetAbsPath(char *Path)
{
	char	*ret;
	 int	pathLen = strlen(Path);
	 int	read, write;
	 int	pos, slashNum=0, baseLen;
	Uint	slashOffsets[MAX_PATH_SLASHES];
	char	*cwd = CFGPTR(CFG_VFS_CWD);
	 int	cwdLen;
	
	ENTER("sPath", Path);
	
	// Memory File
	if(Path[0] == '$') {
		ret = malloc(strlen(Path)+1);
		if(!ret) {
			Warning("VFS_GetAbsPath - malloc() returned NULL");
			return NULL;
		}
		strcpy(ret, Path);
		LEAVE('p', ret);
		return ret;
	}
	
	// Check if the path is already absolute
	if(Path[0] == '/') {
		ret = malloc(pathLen + 1);
		if(!ret) {
			Warning("VFS_GetAbsPath - malloc() returned NULL");
			return NULL;
		}
		strcpy(ret, Path);
		baseLen = 1;
	} else {
		cwdLen = strlen(cwd);
		// Prepend the current directory
		ret = malloc(cwdLen+pathLen+2);
		strcpy(ret, cwd);
		ret[cwdLen] = '/';
		strcpy(&ret[cwdLen+1], Path);
	}
	
	// Remove . and ..
	read = write = 1;	// Cwd has already been parsed
	for(; read < baseLen+pathLen; read = pos+1)
	{
		pos = strpos( &ret[read], '/' );
		// If we are in the last section, force a break at the end of the itteration
		if(pos == -1)	pos = baseLen+pathLen;
		else	pos += read;	// Else, Adjust to absolute
		
		//Log("pos-read = %i", pos-read);
		
		// Check Length
		if(pos - read <= 2)
		{
			//Log("&ret[read] = '%s'", &ret[read]);
			// Current Dir "."
			if(strncmp(&ret[read], ".", pos-read) == 0)	continue;
			// Parent ".."
			if(strncmp(&ret[read], "..", pos-read) == 0)
			{
				// If there is no higher, silently ignore
				if(slashNum < 2)	continue;
				// Reverse write pointer
				write = slashOffsets[ --slashNum - 1 ];
				continue;
			}
		}
		
		
		// Only copy if the positions differ
		if(read != write) {
			Log("write = %i, read = %i", write, read);
			memcpy( &ret[write], &ret[read], pos-read+1 );
		}
		write = pos+1;
		if(slashNum < MAX_PATH_SLASHES)
			slashOffsets[ slashNum++ ] = pos;
		else {
			LOG("Path '%s' has too many elements", Path);
			free(ret);
			LEAVE('n');
			return NULL;
		}
	}
	
	// `ret` should now be the absolute path
	LEAVE('s', ret);
	Log("VFS_GetAbsPath: RETURN '%s'", ret);
	return ret;
}

/**
 * \fn char *VFS_ParsePath(char *Path, char **TruePath)
 * \brief Parses a path, resolving sysmlinks and applying permissions
 */
tVFS_Node *VFS_ParsePath(char *Path, char **TruePath)
{
	tVFS_Mount	*mnt;
	tVFS_Mount	*longestMount = gRootMount;	// Root is first
	 int	cmp, retLength = 0;
	 int	ofs, nextSlash;
	tVFS_Node	*curNode, *tmpNode;
	char	*tmp;
	
	ENTER("sPath pTruePath", Path, TruePath);
	
	// Memory File
	if(Path[0] == '$') {
		if(TruePath) {
			*TruePath = malloc(strlen(Path)+1);
			strcpy(*TruePath, Path);
		}
		curNode = gVFS_MemRoot.FindDir(&gVFS_MemRoot, Path);
		LEAVE('p', curNode);
		return curNode;
	}
	// For root we always fast return
	
	if(Path[0] == '/' && Path[1] == '\0') {
		if(TruePath) {
			*TruePath = malloc( gRootMount->MountPointLen+1 );
			strcpy(*TruePath, gRootMount->MountPoint);
		}
		LEAVE('p', gRootMount->RootNode);
		return gRootMount->RootNode;
	}
	
	// Check if there is anything mounted
	if(!gMounts)	return NULL;
	
	// Find Mountpoint
	for(mnt = gMounts;
		mnt;
		mnt = mnt->Next)
	{
		// Quick Check
		if( Path[mnt->MountPointLen] != '/' && Path[mnt->MountPointLen] != '\0')
			continue;
		// Length Check - If the length is smaller than the longest match sofar
		if(mnt->MountPointLen < longestMount->MountPointLen)	continue;
		// String Compare
		cmp = strcmp(Path, mnt->MountPoint);
		
		#if OPEN_MOUNT_ROOT
		// Fast Break - Request Mount Root
		if(cmp == 0) {
			if(TruePath) {
				*TruePath = malloc( mnt->MountPointLen+1 );
				strcpy(*TruePath, mnt->MountPoint);
			}
			LEAVE('p', mnt->RootNode);
			return mnt->RootNode;
		}
		#endif
		// Not a match, continue
		if(cmp != '/')	continue;
		longestMount = mnt;
	}
	
	// Sanity Check
	/*if(!longestMount) {
		Log("VFS_GetTruePath - ERROR: No Root Node\n");
		return NULL;
	}*/
	
	// Save to shorter variable
	mnt = longestMount;
	
	LOG("mnt = {MountPoint:\"%s\"}", mnt->MountPoint);
	
	// Initialise String
	if(TruePath)
	{
		*TruePath = malloc( mnt->MountPointLen+1 );
		strcpy(*TruePath, mnt->MountPoint);
		retLength = mnt->MountPointLen;
	}
	
	curNode = mnt->RootNode;
	curNode->ReferenceCount ++;	
	// Parse Path
	ofs = mnt->MountPointLen+1;
	for(; (nextSlash = strpos(&Path[ofs], '/')) != -1; Path[nextSlash]='/',ofs = nextSlash + 1)
	{
		nextSlash += ofs;
		Path[nextSlash] = '\0';
	
		// Check for empty string
		if( Path[ofs] == '\0' )	continue;
	
		// Check permissions on root of filesystem
		if( !VFS_CheckACL(curNode, VFS_PERM_EXECUTE) ) {
			curNode->Close( curNode );
			if(TruePath) {
				free(*TruePath);
				*TruePath = NULL;
			}
			LEAVE('n');
			return NULL;
		}
		
		// Check if the node has a FindDir method
		if(!curNode->FindDir) {
			if(curNode->Close)	curNode->Close(curNode);
			if(TruePath) {
				free(*TruePath);
				*TruePath = NULL;
			}
			Path[nextSlash] = '/';
			LEAVE('n');
			return NULL;
		}
		LOG("FindDir(%p, '%s')", curNode, &Path[ofs]);
		// Get Child Node
		tmpNode = curNode->FindDir(curNode, &Path[ofs]);
		LOG("tmpNode = %p", tmpNode);
		if(curNode->Close)
			curNode->Close(curNode);
		curNode = tmpNode;
		
		// Error Check
		if(!curNode) {
			LOG("Node '%s' not found in dir '%s'", &Path[ofs], Path);
			if(TruePath) {
				free(*TruePath);
				*TruePath = NULL;
			}
			Path[nextSlash] = '/';
			LEAVE('n');
			return NULL;
		}
		
		// Handle Symbolic Links
		if(curNode->Flags & VFS_FFLAG_SYMLINK) {
			if(TruePath) {
				free(*TruePath);
				*TruePath = NULL;
			}
			tmp = malloc( curNode->Size + 1 );
			curNode->Read( curNode, 0, curNode->Size, tmp );
			tmp[ curNode->Size ] = '\0';
			
			// Parse Symlink Path
			curNode = VFS_ParsePath(tmp, TruePath);
			
			// Error Check
			if(!curNode) {
				free(tmp);	// Free temp string
				LEAVE('n');
				return NULL;
			}
			
			// Set Path Variable
			if(TruePath) {
				*TruePath = tmp;
				retLength = strlen(tmp);
			} else {
				free(tmp);	// Free temp string
			}
			
			continue;
		}
		
		// Handle Non-Directories
		if( !(curNode->Flags & VFS_FFLAG_DIRECTORY) )
		{
			Warning("VFS_ParsePath - File in directory context");
			if(TruePath)	free(*TruePath);
			LEAVE('n');
			return NULL;
		}
		
		// Check if path needs extending
		if(!TruePath)	continue;
		
		// Increase buffer space
		tmp = realloc( *TruePath, retLength + strlen(&Path[ofs]) + 1 + 1 );
		// Check if allocation succeeded
		if(!tmp) {
			Warning("VFS_ParsePath -  Unable to reallocate true path buffer");
			free(*TruePath);
			if(curNode->Close)	curNode->Close(curNode);
			LEAVE('n');
			return NULL;
		}
		*TruePath = tmp;
		// Append to path
		(*TruePath)[retLength] = '/';
		strcpy(*TruePath+retLength+1, &Path[ofs]);
		// - Extend Path
		retLength += strlen(&Path[ofs])+1;
	}
	
	// Get last node
	LOG("VFS_ParsePath: FindDir(%p, '%s')", curNode, &Path[ofs]);
	tmpNode = curNode->FindDir(curNode, &Path[ofs]);
	LOG("tmpNode = %p", tmpNode);
	if(curNode->Close)	curNode->Close(curNode);
	// Check if file was found
	if(!tmpNode) {
		LOG("Node '%s' not found in dir '%s'", &Path[ofs], Path);
		if(TruePath)	free(*TruePath);
		if(curNode->Close)	curNode->Close(curNode);
		LEAVE('n');
		return NULL;
	}
	
	if(TruePath)
	{
		// Increase buffer space
		tmp = realloc(*TruePath, retLength + strlen(&Path[ofs]) + 1 + 1);
		// Check if allocation succeeded
		if(!tmp) {
			Warning("VFS_ParsePath -  Unable to reallocate true path buffer");
			free(*TruePath);
			if(tmpNode->Close)	tmpNode->Close(curNode);
			LEAVE('n');
			return NULL;
		}
		*TruePath = tmp;
		// Append to path
		(*TruePath)[retLength] = '/';
		strcpy(*TruePath + retLength + 1, &Path[ofs]);
		// - Extend Path
		//retLength += strlen(tmpNode->Name) + 1;
	}
	
	LEAVE('p', tmpNode);
	return tmpNode;
}

/**
 * \fn int VFS_Open(char *Path, Uint Mode)
 * \brief Open a file
 */
int VFS_Open(char *Path, Uint Mode)
{
	tVFS_Node	*node;
	char	*absPath;
	 int	i;
	
	ENTER("sPath xMode", Path, Mode);
	
	// Get absolute path
	absPath = VFS_GetAbsPath(Path);
	LOG("absPath = \"%s\"", absPath);
	// Parse path and get mount point
	node = VFS_ParsePath(absPath, NULL);
	// Free generated path
	free(absPath);
	
	if(!node) {
		LOG("Cannot find node");
		LEAVE('i', -1);
		return -1;
	}
	
	// Check for symlinks
	if( !(Mode & VFS_OPENFLAG_NOLINK) && (node->Flags & VFS_FFLAG_SYMLINK) )
	{
		if( !node->Read ) {
			Warning("No read method on symlink");
			LEAVE('i', -1);
			return -1;
		}
		absPath = malloc(node->Size+1);	// Allocate Buffer
		node->Read( node, 0, node->Size, absPath );	// Read Path
		
		absPath[ node->Size ] = '\0';	// End String
		if(node->Close)	node->Close( node );	// Close old node
		node = VFS_ParsePath(absPath, NULL);	// Get new node
		free( absPath );	// Free allocated path
	}
	
	if(!node) {
		LEAVE('i', -1);
		return -1;
	}
	
	i = 0;
	i |= (Mode & VFS_OPENFLAG_EXEC) ? VFS_PERM_EXECUTE : 0;
	i |= (Mode & VFS_OPENFLAG_READ) ? VFS_PERM_READ : 0;
	i |= (Mode & VFS_OPENFLAG_WRITE) ? VFS_PERM_WRITE : 0;
	
	LOG("i = 0b%b", i);
	
	// Permissions Check
	if( !VFS_CheckACL(node, i) ) {
		node->Close( node );
		LEAVE('i', -1);
		return -1;
	}
	
	// Check for a user open
	if(Mode & VFS_OPENFLAG_USER)
	{
		// Allocate Buffer
		if( MM_GetPhysAddr( (Uint)gaUserHandles ) == 0 )
		{
			Uint	addr, size;
			size = CFGINT(CFG_VFS_MAXFILES) * sizeof(tVFS_Handle);
			for(addr = 0; addr < size; addr += 0x1000)
				MM_Allocate( (Uint)gaUserHandles + addr );
			memset( gaUserHandles, 0, size );
		}
		// Get a handle
		for(i=0;i<CFGINT(CFG_VFS_MAXFILES);i++)
		{
			if(gaUserHandles[i].Node)	continue;
			gaUserHandles[i].Node = node;
			gaUserHandles[i].Position = 0;
			gaUserHandles[i].Mode = Mode;
			LEAVE('i', i);
			return i;
		}
	}
	else
	{
		// Allocate space if not already
		if( MM_GetPhysAddr( (Uint)gaKernelHandles ) == 0 )
		{
			Uint	addr, size;
			size = MAX_KERNEL_FILES * sizeof(tVFS_Handle);
			for(addr = 0; addr < size; addr += 0x1000)
				MM_Allocate( (Uint)gaKernelHandles + addr );
			memset( gaKernelHandles, 0, size );
		}
		// Get a handle
		for(i=0;i<MAX_KERNEL_FILES;i++)
		{
			if(gaKernelHandles[i].Node)	continue;
			gaKernelHandles[i].Node = node;
			gaKernelHandles[i].Position = 0;
			gaKernelHandles[i].Mode = Mode;
			LEAVE('x', i|VFS_KERNEL_FLAG);
			return i|VFS_KERNEL_FLAG;
		}
	}
	
	LEAVE('i', -1);
	return -1;
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
		Warning("Invalid file handle passed to VFS_Close, 0x%x\n", FD);
		return;
	}
	
	if(h->Node->Close)
		h->Node->Close( h->Node );
	
	h->Node = NULL;
}

/**
 * \fn int VFS_ChDir(char *New)
 * \brief Change current working directory
 */
int VFS_ChDir(char *New)
{
	char	*buf;
	 int	fd;
	tVFS_Handle	*h;
	
	// Create Absolute
	buf = VFS_GetAbsPath(New);
	if(buf == NULL) {
		Log("VFS_ChDir: Path expansion failed");
		return -1;
	}
	
	// Check if path exists
	fd = VFS_Open(buf, VFS_OPENFLAG_EXEC);
	if(fd == -1) {
		Log("VFS_ChDir: Path is invalid");
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
	
	// Free old working directory
	if( CFGPTR(CFG_VFS_CWD) )	free( CFGPTR(CFG_VFS_CWD) );
	// Set new
	CFGPTR(CFG_VFS_CWD) = buf;
	
	Log("Updated CWD to '%s'", buf);
	
	return 1;
}

/**
 * \fn tVFS_Handle *VFS_GetHandle(int FD)
 * \brief Gets a pointer to the handle information structure
 */
tVFS_Handle *VFS_GetHandle(int FD)
{
	tVFS_Handle	*h;
	
	if(FD < 0)	return NULL;
	
	if(FD & VFS_KERNEL_FLAG) {
		FD &= (VFS_KERNEL_FLAG - 1);
		if(FD >= MAX_KERNEL_FILES)	return NULL;
		h = &gaKernelHandles[ FD ];
	} else {
		if(FD >= CFGINT(CFG_VFS_MAXFILES))	return NULL;
		h = &gaUserHandles[ FD ];
	}
	
	if(h->Node == NULL)	return NULL;
	return h;
}

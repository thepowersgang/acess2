/*
 * AcessMicro VFS
 * - Open, Close and ChDir
 */
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"
#include "vfs_ext.h"

#define DEBUG	0

#if DEBUG
#else
# undef ENTER
# undef LOG
# undef LEAVE
# define ENTER(...)
# define LOG(...)
# define LEAVE(...)
#endif

// === CONSTANTS ===
#define	OPEN_MOUNT_ROOT	1
#define MAX_KERNEL_FILES	128

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
	Uint	slashOffsets[256];
	char	*cwd = CFGPTR(CFG_VFS_CWD);
	 int	cwdLen;
	
	ENTER("sPath", Path);
	
	// Memory File
	if(Path[0] == '$') {
		ret = malloc(strlen(Path)+1);
		strcpy(ret, Path);
		LEAVE('p', ret);
		return ret;
	}
	
	// Check if the path is already absolute
	if(Path[0] == '/') {
		ret = malloc(pathLen + 1);
		strcpy(ret, Path);
		baseLen = 1;
	} else {
		cwdLen = strlen(cwd);
		// Prepend the current directory
		ret = malloc(cwdLen+pathLen+1);
		strcpy(ret, cwd);
		strcpy(&ret[cwdLen], Path);
	
		// Pre-fill the slash positions
		pos = 0;
		while( (pos = strpos( &ret[pos+1], '/' )) != -1 )
			slashOffsets[slashNum++] = pos;
			
		baseLen = cwdLen;
	}
	
	// Remove . and ..
	read = write = baseLen;	// Cwd has already been parsed
	for(; read < baseLen+pathLen; read = pos+1)
	{
		pos = strpos( &ret[read], '/' );
		// If we are in the last section, force a break at the end of the itteration
		if(pos == -1)	pos = baseLen+pathLen;
		else	pos += read;	// Else, Adjust to absolute
		
		// Check Length
		if(pos - read <= 2)
		{
			// Current Dir "."
			if(strncmp(&ret[read], ".", pos-read) == 0)	continue;
			// Parent ".."
			if(strncmp(&ret[read], "..", pos-read) == 0)
			{
				// If there is no higher, silently ignore
				if(!slashNum)	continue;
				// Reverse write pointer
				write = slashOffsets[ slashNum-- ];
				continue;
			}
		}
		
		
		// Only copy if the positions differ
		if(read != write) {
			memcpy( &ret[write], &ret[read], pos-read+1 );
		}
		write = pos+1;
		if(slashNum < 256)
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
			LOG("No read method on symlink");
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
 * \fn tVFS_Handle *VFS_GetHandle(int FD)
 * \brief Gets a pointer to the handle information structure
 */
tVFS_Handle *VFS_GetHandle(int FD)
{
	if(FD < 0)	return NULL;
	
	if(FD & VFS_KERNEL_FLAG) {
		FD &= (VFS_KERNEL_FLAG - 1);
		if(FD >= MAX_KERNEL_FILES)	return NULL;
		return &gaKernelHandles[ FD ];
	} else {
		if(FD >= CFGINT(CFG_VFS_MAXFILES))	return NULL;
		return &gaUserHandles[ FD ];
	}
}

/*
 * Acess2 VFS
 * - Directory Management Functions
 */
#define SANITY	1
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include <vfs_int.h>

// === IMPORTS ===
extern tVFS_Mount	*gRootMount;

// === PROTOTYPES ===
#if 0
 int	VFS_MkDir(const char *Path);
 int	VFS_MkNod(const char *Path, Uint Flags);
 int	VFS_Symlink(const char *Name, const char *Link);
#endif

// === CODE ===
/**
 * \fn int VFS_MkDir(char *Path)
 * \brief Create a new node
 * \param Path	Path of directory to create
 */
int VFS_MkDir(const char *Path)
{
	return VFS_MkNod(Path, VFS_FFLAG_DIRECTORY);
}

/**
 * \fn int VFS_MkNod(char *Path, Uint Flags)
 * \brief Create a new node in a directory
 * \param Path	Path of new node
 * \param Flags	Flags to apply to the node
 */
int VFS_MkNod(const char *Path, Uint Flags)
{
	tVFS_Mount	*mountpt;
	char	*absPath, *name;
	 int	pos = 0, oldpos = 0;
	 int	next = 0;
	tVFS_Node	*parent;
	tVFS_Node	*ret;
	
	ENTER("sPath xFlags", Path, Flags);
	
	absPath = VFS_GetAbsPath(Path);
	LOG("absPath = '%s'", absPath);
	
	while( (next = strpos(&absPath[pos+1], '/')) != -1 ) {
		LOG("next = %i", next);
		pos += next+1;
		LOG("pos = %i", pos);
		oldpos = pos;
	}
	absPath[oldpos] = '\0';	// Mutilate path
	name = &absPath[oldpos+1];
	
	LOG("absPath = '%s', name = '%s'", absPath, name);
	
	// Check for root
	if(absPath[0] == '\0')
		parent = VFS_ParsePath("/", NULL, &mountpt);
	else
		parent = VFS_ParsePath(absPath, NULL, &mountpt);
	
	LOG("parent = %p", parent);
	
	if(!parent) {
		errno = ENOENT;
		goto _error;
	}

	// Permissions Check
	if( !VFS_CheckACL(parent, VFS_PERM_EXEC|VFS_PERM_WRITE) ) {
		errno = EACCES;
		goto _error;
	}
	
	LOG("parent = %p", parent);
	
	if(!parent->Type || !parent->Type->MkNod) {
		Log_Warning("VFS", "VFS_MkNod - Directory has no MkNod method");
		errno = ENOTDIR;
		goto _error;
	}
	
	// Create node
	ret = parent->Type->MkNod(parent, name, Flags);
	_CloseNode(ret);
	
	// Free allocated string
	free(absPath);
	
	// Free Parent
	ASSERT(mountpt->OpenHandleCount>0);
	mountpt->OpenHandleCount --;
	_CloseNode(parent);

	// Return whatever the driver said	
	LEAVE('i', ret==NULL);
	return ret==NULL;

_error:
	if( parent ) {
		_CloseNode(parent);
		ASSERT(mountpt->OpenHandleCount>0);
		mountpt->OpenHandleCount --;
	}
	free(absPath);
	LEAVE('i', -1);
	return -1;
}

/**
 * \fn int VFS_Symlink(const char *Name, const char *Link)
 * \brief Creates a symlink called \a Name to \a Link
 * \param Name	Name of symbolic link
 * \param Link	Destination of symbolic link
 */
int VFS_Symlink(const char *Name, const char *Link)
{
	char	*realLink;
	 int	fp;
	
	ENTER("sName sLink", Name, Link);
	
	// Get absolue path name
	realLink = VFS_GetAbsPath( Link );
	if(!realLink) {
		Log_Warning("VFS", "Path '%s' is badly formed", Link);
		errno = EINVAL;
		LEAVE('i', -1);
		return -1;
	}

	LOG("realLink = '%s'", realLink);

	// Make node
	if( VFS_MkNod(Name, VFS_FFLAG_SYMLINK) != 0 ) {
		Log_Warning("VFS", "Unable to create link node '%s'", Name);
		free(realLink);
		// errno is set by VFS_MkNod
		LEAVE('i', -2);
		return -2;	// Make link node
	}
	
	// Write link address
	fp = VFS_Open(Name, VFS_OPENFLAG_WRITE|VFS_OPENFLAG_NOLINK);
	if( fp == -1 ) {
		Log_Warning("VFS", "Unable to open newly created symlink '%s'", Name);
		free(realLink);
		LEAVE('i', -3);
		return -3;
	}
	VFS_Write(fp, strlen(realLink), realLink);
	VFS_Close(fp);
	
	free(realLink);
	
	LEAVE('i', 1);
	return 1;
}

/**
 * \fn int VFS_ReadDir(int FD, char *Dest)
 * \brief Read from a directory
 */
int VFS_ReadDir(int FD, char *Dest)
{
	tVFS_Handle	*h = VFS_GetHandle(FD);
	 int	rv;
	
	//ENTER("ph pDest", h, Dest);
	
	if(!h || !h->Node->Type || !h->Node->Type->ReadDir) {
		//LEAVE('i', 0);
		return 0;
	}
	
	#if 0
	if(h->Node->Size != (Uint64)-1 && h->Position >= h->Node->Size) {
		//LEAVE('i', 0);
		return 0;
	}
	#endif
	
	do {
		rv = h->Node->Type->ReadDir(h->Node, h->Position, Dest);
		if(rv > 0)
			h->Position += rv;
		else
			h->Position ++;
	} while(rv > 0);
	
	if(rv < 0) {
		//LEAVE('i', 0);
		return 0;
	}
	
	//LEAVE('i', 1);
	return 1;
}

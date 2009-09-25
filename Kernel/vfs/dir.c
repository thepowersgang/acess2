/*
 */
#include "vfs.h"
#include "vfs_int.h"

// === IMPORTS ===
extern tVFS_Mount	*gRootMount;

// === PROTOTYPES ===
 int	VFS_MkDir(char *Path);
 int	VFS_MkNod(char *Path, Uint Flags);

// === CODE ===
/**
 * \fn int VFS_MkDir(char *Path)
 * \brief Create a new node
 * \param Path	Path of directory to create
 */
int VFS_MkDir(char *Path)
{
	return VFS_MkNod(Path, VFS_FFLAG_DIRECTORY);
}

/**
 * \fn int VFS_MkNod(char *Path, Uint Flags)
 * \brief Create a new node in a directory
 * \param Path	Path of new node
 * \param Flags	Flags to apply to the node
 */
int VFS_MkNod(char *Path, Uint Flags)
{
	char	*absPath, *name;
	 int	pos=0, oldpos = 0;
	tVFS_Node	*parent;
	 int	ret;
	
	Debug_Enter("VFS_MkNod", "sPath xFlags", Path, Flags);
	
	absPath = VFS_GetAbsPath(Path);
	
	while( (pos = strpos8(&absPath[pos+1], '/')) != -1 )	oldpos = pos;
	absPath[oldpos] = '\0';	// Mutilate path
	name = &absPath[oldpos+1];
	
	// Check for root
	if(absPath[0] == '\0')
		parent = VFS_ParsePath("/", NULL);
	else
		parent = VFS_ParsePath(absPath, NULL);
	
	if(!parent)	return -1;	// Error Check
	
	// Permissions Check
	if( !VFS_CheckACL(parent, VFS_PERM_EXECUTE|VFS_PERM_WRITE) ) {
		if(parent->Close)	parent->Close( parent );
		free(absPath);
		Debug_Leave("VFS_MkNod", 'i', -1);
		return -1;
	}
	
	Debug_Log("VFS_MkNod", "parent = %p\n", parent);
	
	if(parent->MkNod == NULL) {
		Warning("VFS_MkNod - Directory has no MkNod method");
		Debug_Leave("VFS_MkNod", 'i', -1);
		return -1;
	}
	
	// Create node
	ret = parent->MkNod(parent, name, Flags);
	
	// Free allocated string
	free(absPath);
	
	// Free Parent
	if(parent->Close)	parent->Close( parent );
	
	// Error Check
	if(ret == 0)	return -1;
	
	Debug_Leave("VFS_MkNod", 'i', 0);
	return 0;
}

/**
 * \fn int VFS_Symlink(char *Name, char *Link)
 * \brief Creates a symlink called \a Name to \a Link
 * \param Name	Name of symbolic link
 * \param Link	Destination of symbolic link
 */
int VFS_Symlink(char *Name, char *Link)
{
	char	*realLink;
	 int	fp;
	tVFS_Node	*destNode;
	
	//LogF("vfs_symlink: (name='%s', link='%s')\n", name, link);
	
	// Get absolue path name
	Link = VFS_GetAbsPath( Link );
	if(!Link) {
		Warning("Path '%s' is badly formed", Link);
		return -1;
	}
	
	// Get true path and node
	destNode = VFS_ParsePath( Link, &realLink );
	free(Link);
	
	// Check if destination exists
	if(!destNode) {
		Warning("File '%s' does not exist, symlink not created", Link);
		return -1;
	}
	
	// Derefence the destination
	if(destNode->Close)	destNode->Close(destNode);
	
	// Make node
	if( VFS_MkNod(Name, VFS_FFLAG_SYMLINK) != 0 ) {
		Warning("Unable to create link node '%s'", Name);
		return -2;	// Make link node
	}
	
	// Write link address
	fp = VFS_Open(Name, VFS_OPENFLAG_WRITE|VFS_OPENFLAG_NOLINK);
	VFS_Write(fp, strlen(realLink), realLink);
	VFS_Close(fp);
	
	free(realLink);
	
	return 1;
}

/**
 * \fn int VFS_ReadDir(int FD, char *Dest)
 * \brief Read from a directory
 */
int VFS_ReadDir(int FD, char *Dest)
{
	tVFS_Handle	*h = VFS_GetHandle(FD);
	char	*tmp;
	
	ENTER("ph pDest", h, Dest);
	
	if(!h || h->Node->ReadDir == NULL) {
		LEAVE('i', 0);
		return 0;
	}
	
	if(h->Position >= h->Node->Size) {
		LEAVE('i', 0);
		return 0;
	}
	
	tmp = h->Node->ReadDir(h->Node, h->Position);
	LOG("tmp = '%s'", tmp);
	
	if(!tmp) {
		LEAVE('i', 0);
		return 0;
	}
	
	h->Position ++;
	
	strcpy(Dest, tmp);
	
	if(IsHeap(tmp))	free(tmp);
	
	LEAVE('i', 1);
	return 1;
}

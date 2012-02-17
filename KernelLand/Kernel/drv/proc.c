/*
 * Acess2
 * - Kernel Status Driver
 */
#define DEBUG	1
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <fs_sysfs.h>

// === CONSTANTS ===
#define	VERSION	((0 << 8) | (1))	// 0.01

// === TYPES ===
typedef struct sSysFS_Ent
{
	struct sSysFS_Ent	*Next;
	struct sSysFS_Ent	*ListNext;
	struct sSysFS_Ent	*Parent;
	tVFS_Node	Node;
	char	Name[];
} tSysFS_Ent;

// === PROTOTYPES ===
 int	SysFS_Install(char **Arguments);
 int	SysFS_IOCtl(tVFS_Node *Node, int Id, void *Data);

#if 0
 int	SysFS_RegisterFile(const char *Path, const char *Data, int Length);
 int	SysFS_UpdateFile(int ID, const char *Data, int Length);
 int	SysFS_RemoveFile(int ID);
#endif

char	*SysFS_Comm_ReadDir(tVFS_Node *Node, int Id);
tVFS_Node	*SysFS_Comm_FindDir(tVFS_Node *Node, const char *Filename);
size_t	SysFS_Comm_ReadFile(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
void	SysFS_Comm_CloseFile(tVFS_Node *Node);

// === GLOBALS ===
extern tSysFS_Ent	gSysFS_Version;	// Defined Later
extern tSysFS_Ent	gSysFS_Root;	// Defined Later
MODULE_DEFINE(0, VERSION, SysFS, SysFS_Install, NULL, NULL);
tVFS_NodeType	gSysFS_FileNodeType = {
	.TypeName = "SysFS File",
	.Read = SysFS_Comm_ReadFile
	};
tVFS_NodeType	gSysFS_DirNodeType = {
	.TypeName = "SysFS Dir",
	.ReadDir = SysFS_Comm_ReadDir,
	.FindDir = SysFS_Comm_FindDir
	};
tSysFS_Ent	gSysFS_Version_Kernel = {
	NULL, NULL,	// Nexts
	&gSysFS_Version,	// Parent
	{
		.Inode = 1,	// File #1
		.ImplPtr = NULL,
		.ImplInt = (Uint)&gSysFS_Version_Kernel,	// Self-Link
		.Size = 0,
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRO,
		.Type = &gSysFS_FileNodeType
	},
	"Kernel"
};
tSysFS_Ent	gSysFS_Version = {
	NULL, NULL,
	&gSysFS_Root,
	{
		.Size = 1,
		.ImplPtr = &gSysFS_Version_Kernel,
		.ImplInt = (Uint)&gSysFS_Version,	// Self-Link
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.Flags = VFS_FFLAG_DIRECTORY,
		.Type = &gSysFS_DirNodeType
	},
	"Version"
};
// Root of the SysFS tree (just used to keep the code clean)
tSysFS_Ent	gSysFS_Root = {
	NULL, NULL,
	NULL,
	{
		.Size = 1,
		.ImplPtr = &gSysFS_Version,
		.ImplInt = (Uint)&gSysFS_Root	// Self-Link
	},
	"/"
};
tDevFS_Driver	gSysFS_DriverInfo = {
	NULL, "system",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ImplPtr = &gSysFS_Version,
	.Size = sizeof(gSysFS_Root)/sizeof(tSysFS_Ent),
	.Type = &gSysFS_DirNodeType
	}
};
 int	giSysFS_NextFileID = 2;
tSysFS_Ent	*gSysFS_FileList;

// === CODE ===
/**
 * \fn int SysFS_Install(char **Options)
 * \brief Installs the SysFS Driver
 */
int SysFS_Install(char **Options)
{
	{
		const char	*fmt = "Acess2 "EXPAND_STR(KERNEL_VERSION)" "EXPAND_STR(ARCHDIR)" build %i, hash %s";
		gSysFS_Version_Kernel.Node.Size = sprintf(NULL, fmt, BUILD_NUM, gsGitHash);
		gSysFS_Version_Kernel.Node.ImplPtr = malloc( gSysFS_Version_Kernel.Node.Size + 1 );
		sprintf(gSysFS_Version_Kernel.Node.ImplPtr, fmt, BUILD_NUM, gsGitHash);
	}

	DevFS_AddDevice( &gSysFS_DriverInfo );
	return MODULE_ERR_OK;
}

/**
 * \fn int SysFS_RegisterFile(char *Path, char *Data, int Length)
 * \brief Registers a file (buffer) for the user to be able to read from
 * \param Path	Path for the file to be accessable from (relative to SysFS root)
 * \param Data	Pointer to the data buffer (must be non-volatile)
 * \param Length	Length of the data buffer
 * \return The file's identifier
 */
int SysFS_RegisterFile(const char *Path, const char *Data, int Length)
{
	 int	start = 0;
	 int	tmp;
	tSysFS_Ent	*ent = NULL;
	tSysFS_Ent	*child, *prev;
	
	// Find parent directory
	while( (tmp = strpos(&Path[start], '/')) != -1 )
	{
		prev = NULL;
		
		if(ent)
			child = ent->Node.ImplPtr;
		else
			child = gSysFS_DriverInfo.RootNode.ImplPtr;
		for( ; child; prev = child, child = child->Next )
		{
			if( strncmp( &Path[start], child->Name, tmp+1 ) == '/' )
				break;
		}
		
		// Need a new directory?
		if( !child )
		{
			child = calloc( 1, sizeof(tSysFS_Ent)+tmp+1 );
			child->Next = NULL;
			memcpy(child->Name, &Path[start], tmp);
			child->Name[tmp] = '\0';
			child->Parent = ent;
			child->Node.Inode = 0;
			child->Node.ImplPtr = NULL;
			child->Node.ImplInt = (Uint)child;	// Uplink
			child->Node.NumACLs = 1;
			child->Node.ACLs = &gVFS_ACL_EveryoneRX;
			child->Node.Flags = VFS_FFLAG_DIRECTORY;
			child->Node.Type = &gSysFS_DirNodeType;
			if( !prev ) {
				if(ent)
					ent->Node.ImplPtr = child;
				else
					gSysFS_DriverInfo.RootNode.ImplPtr = child;
				// ^^^ Impossible (There is already /Version)
			}
			else
				prev->Next = child;
			if(ent)
				ent->Node.Size ++;
			else
				gSysFS_DriverInfo.RootNode.Size ++;
			Log_Log("SysFS", "Added directory '%s'", child->Name);
		}
		
		ent = child;
		
		start = tmp+1;
	}
	
	// ent: Parent tSysFS_Ent or NULL
	// start: beginning of last path element
	
	// Check if the name is taken
	prev = NULL;
	if(ent)
		child = ent->Node.ImplPtr;
	else
		child = gSysFS_DriverInfo.RootNode.ImplPtr;
	for( ; child; child = child->Next )
	{
		if( strcmp( &Path[start], child->Name ) == 0 )
			break;
	}
	if( child ) {
		Log_Warning("SysFS", "'%s' is taken (in '%s')\n", &Path[start], Path);
		return 0;
	}
	
	// Create new node
	child = calloc( 1, sizeof(tSysFS_Ent)+strlen(&Path[start])+1 );
	child->Next = NULL;
	strcpy(child->Name, &Path[start]);
	child->Parent = ent;
	
	child->Node.Inode = giSysFS_NextFileID++;
	child->Node.ImplPtr = (void*)Data;
	child->Node.ImplInt = (Uint)child;	// Uplink
	child->Node.Size = Length;
	child->Node.NumACLs = 1;
	child->Node.ACLs = &gVFS_ACL_EveryoneRO;
	child->Node.Type = &gSysFS_FileNodeType;
	
	// Add to parent's child list
	if(ent) {
		ent->Node.Size ++;
		child->Next = ent->Node.ImplPtr;
		ent->Node.ImplPtr = child;
	}
	else {
		gSysFS_DriverInfo.RootNode.Size ++;
		child->Next = gSysFS_DriverInfo.RootNode.ImplPtr;
		gSysFS_DriverInfo.RootNode.ImplPtr = child;
	}
	// Add to global file list
	child->ListNext = gSysFS_FileList;
	gSysFS_FileList = child;
	
	Log_Log("SysFS", "Added '%s' (%p)", Path, Data);
	
	return child->Node.Inode;
}

/**
 * \fn int SysFS_UpdateFile(int ID, char *Data, int Length)
 * \brief Updates a file
 * \param ID	Identifier returned by ::SysFS_RegisterFile
 * \param Data	Pointer to the data buffer
 * \param Length	Length of the data buffer
 * \return Boolean Success
 */
int SysFS_UpdateFile(int ID, const char *Data, int Length)
{
	tSysFS_Ent	*ent;
	
	for( ent = gSysFS_FileList; ent; ent = ent->Next )
	{
		// It's a reverse sorted list
		if(ent->Node.Inode < ID)	return 0;
		if(ent->Node.Inode == ID)
		{
			ent->Node.ImplPtr = (void*)Data;
			ent->Node.Size = Length;
			return 1;
		}
	}
	
	return 0;
}

/**
 * \fn int SysFS_RemoveFile(int ID)
 * \brief Removes a file from user access
 * \param ID	Identifier returned by ::SysFS_RegisterFile
 * \return Boolean Success
 * \note If a handle is still open to the file, it will be invalidated
 */
int SysFS_RemoveFile(int ID)
{
	tSysFS_Ent	*file;
	tSysFS_Ent	*ent, *parent, *prev;
	
	prev = NULL;
	for( ent = gSysFS_FileList; ent; prev = ent, ent = ent->Next )
	{
		// It's a reverse sorted list
		if(ent->Node.Inode < ID)	return 0;
		if(ent->Node.Inode == ID)	break;
	}
	
	if(!ent)	return 0;
	
	// Set up for next part
	file = ent;
	parent = file->Parent;
	
	// Remove from file list
	if(prev)
		prev->ListNext = file->ListNext;
	else
		gSysFS_FileList = file->ListNext;
	file->Node.Size = 0;
	file->Node.ImplPtr = NULL;
	
	// Search parent directory
	for( ent = parent->Node.ImplPtr; ent; prev = ent, ent = ent->Next )
	{
		if( ent == file )	break;
	}
	if(!ent) {
		Log_Warning("SysFS", "Bookkeeping Error: File in list, but not in directory");
		return 0;
	}
	
	// Remove from parent directory
	if(prev)
		prev->Next = ent->Next;
	else
		parent->Node.ImplPtr = ent->Next;
	
	// Free if not in use
	if(file->Node.ReferenceCount == 0)
		free(file);
	
	return 1;
}

/**
 * \fn char *SysFS_Comm_ReadDir(tVFS_Node *Node, int Pos)
 * \brief Reads from a SysFS directory
 */
char *SysFS_Comm_ReadDir(tVFS_Node *Node, int Pos)
{
	tSysFS_Ent	*child = (tSysFS_Ent*)Node->ImplPtr;
	if(Pos < 0 || Pos >= Node->Size)	return NULL;
	
	for( ; child; child = child->Next, Pos-- )
	{
		if( Pos == 0 )	return strdup(child->Name);
	}
	return NULL;
}

/**
 * \fn tVFS_Node *SysFS_Comm_FindDir(tVFS_Node *Node, const char *Filename)
 * \brief Find a file in a SysFS directory
 */
tVFS_Node *SysFS_Comm_FindDir(tVFS_Node *Node, const char *Filename)
{
	tSysFS_Ent	*child = (tSysFS_Ent*)Node->ImplPtr;
	
	for( ; child; child = child->Next )
	{
		if( strcmp(child->Name, Filename) == 0 )
			return &child->Node;
	}
	
	return NULL;
}

/**
 * \brief Read from an exposed buffer
 */
size_t SysFS_Comm_ReadFile(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	if( Offset > Node->Size )	return -1;
	if( Length > Node->Size )	Length = Node->Size;
	if( Offset + Length > Node->Size)	Length = Node->Size - Offset;
	
	if( Node->ImplPtr )
		memcpy(Buffer, (void*)((Uint)Node->ImplPtr+(Uint)Offset), Length);
	else
		return -1;
	
	return Length;
}

/**
 * \fn void SysFS_Comm_CloseFile(tVFS_Node *Node)
 * \brief Closes an open file
 * \param Node	Node to close
 */
void SysFS_Comm_CloseFile(tVFS_Node *Node)
{
	// Dereference
	Node->ReferenceCount --;
	if( Node->ReferenceCount > 0 )	return;
	
	// Check if it is still valid
	if( Node->ImplPtr )	return;
	
	// Delete
	free( (void*)Node->ImplInt );
}

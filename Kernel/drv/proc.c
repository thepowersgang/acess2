/*
 * Acess2
 * - Kernel Status Driver
 */
#define DEBUG	1
#include <common.h>
#include <modules.h>
#include <fs_devfs.h>
#include <fs_sysfs.h>

// === CONSTANTS ===
#define	VERSION	((0 << 8) | (1))	// 0.01
#define KERNEL_VERSION_STRING	("Acess2 " EXPAND_STR(KERNEL_VERSION) " build " EXPAND_STR(BUILD_NUM))

// === TYPES ===
typedef struct sSysFS_Ent
{
	struct sSysFS_Ent	*Next;
	struct sSysFS_Ent	*ListNext;
	struct sSysFS_Ent	*Parent;
	char	*Name;
	tVFS_Node	Node;
} tSysFS_Ent;

// === PROTOTYPES ===
 int	SysFS_Install(char **Arguments);
 int	SysFS_IOCtl(tVFS_Node *Node, int Id, void *Data);

 int	SysFS_RegisterFile(char *Path, char *Data, int Length);
 int	SysFS_UpdateFile(int ID, char *Data, int Length);
 int	SysFS_RemoveFile(int ID);

char	*SysFS_Comm_ReadDir(tVFS_Node *Node, int Id);
tVFS_Node	*SysFS_Comm_FindDir(tVFS_Node *Node, char *Filename);
Uint64	SysFS_Comm_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
void	SysFS_Comm_CloseFile(tVFS_Node *Node);

// === GLOBALS ===
extern tSysFS_Ent	gSysFS_Version;	// Defined Later
extern tSysFS_Ent	gSysFS_Root;	// Defined Later
MODULE_DEFINE(0, VERSION, SysFS, SysFS_Install, NULL, NULL);
tSysFS_Ent	gSysFS_Version_Kernel = {
	NULL, NULL,	// Nexts
	&gSysFS_Version,	// Parent
	"Kernel",
	{
		.Inode = 1,	// File #1
		.ImplPtr = KERNEL_VERSION_STRING,
		.ImplInt = (Uint)&gSysFS_Version_Kernel,	// Self-Link
		.Size = sizeof(KERNEL_VERSION_STRING)-1,
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRO,
		.Read = SysFS_Comm_ReadFile
	}
};
tSysFS_Ent	gSysFS_Version = {
	NULL, NULL,
	&gSysFS_Root,
	"Version",
	{
		.Size = 1,
		.ImplPtr = &gSysFS_Version_Kernel,
		.ImplInt = (Uint)&gSysFS_Version,	// Self-Link
		.NumACLs = 1,
		.ACLs = &gVFS_ACL_EveryoneRX,
		.Flags = VFS_FFLAG_DIRECTORY,
		.ReadDir = SysFS_Comm_ReadDir,
		.FindDir = SysFS_Comm_FindDir
	}
};
// Root of the SysFS tree (just used for clean code)
tSysFS_Ent	gSysFS_Root = {
	NULL, NULL,
	NULL,
	"/",
	{
		.Size = 1,
		.ImplPtr = &gSysFS_Version,
		.ImplInt = (Uint)&gSysFS_Root	// Self-Link
	//	.NumACLs = 1,
	//	.ACLs = &gVFS_ACL_EveryoneRX,
	//	.Flags = VFS_FFLAG_DIRECTORY,
	//	.ReadDir = SysFS_Comm_ReadDir,
	//	.FindDir = SysFS_Comm_FindDir
	}
};
tDevFS_Driver	gSysFS_DriverInfo = {
	NULL, "system",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ImplPtr = &gSysFS_Version,
	.Size = sizeof(gSysFS_Root)/sizeof(tSysFS_Ent),
	.ReadDir = SysFS_Comm_ReadDir,
	.FindDir = SysFS_Comm_FindDir,
	.IOCtl = NULL
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
	DevFS_AddDevice( &gSysFS_DriverInfo );
	return 0;
}

/**
 * \fn int SysFS_RegisterFile(char *Path, char *Data, int Length)
 * \brief Registers a file (buffer) for the user to be able to read from
 * \param Path	Path for the file to be accessable from (relative to SysFS root)
 * \param Data	Pointer to the data buffer (must be non-volatile)
 * \param Length	Length of the data buffer
 * \return The file's identifier
 */
int SysFS_RegisterFile(char *Path, char *Data, int Length)
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
			child = calloc( 1, sizeof(tSysFS_Ent) );
			child->Next = NULL;
			child->Name = malloc(tmp+1);
			memcpy(child->Name, &Path[start], tmp);
			child->Name[tmp] = '\0';
			child->Parent = ent;
			child->Node.Inode = 0;
			child->Node.ImplPtr = NULL;
			child->Node.ImplInt = (Uint)child;	// Uplink
			child->Node.NumACLs = 1;
			child->Node.ACLs = &gVFS_ACL_EveryoneRX;
			child->Node.Flags = VFS_FFLAG_DIRECTORY;
			child->Node.ReadDir = SysFS_Comm_ReadDir;
			child->Node.FindDir = SysFS_Comm_FindDir;
			if( !prev ) {
				//if(ent)
					ent->Node.ImplPtr = child;
				//else
				//	gSysFS_DriverInfo.RootNode.ImplPtr = child;
				// ^^^ Impossible (There is already /Version
			}
			else
				prev->Next = child;
			if(ent)
				ent->Node.Size ++;
			else
				gSysFS_DriverInfo.RootNode.Size ++;
			LOG("Added directory '%s'\n", child->Name);
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
	for( child = ent->Node.ImplPtr; child; prev = child, child = child->Next )
	{
		if( strcmp( &Path[start], child->Name ) == 0 )
			break;
	}
	if( child ) {
		Warning("[SYSFS] '%s' is taken (in '%s')\n", &Path[start], Path);
		return 0;
	}
	
	// Create new node
	child = calloc( 1, sizeof(tSysFS_Ent) );
	child->Next = NULL;
	child->Name = strdup(&Path[start]);
	child->Parent = ent;
	
	child->Node.Inode = giSysFS_NextFileID++;
	child->Node.ImplPtr = Data;
	child->Node.ImplInt = (Uint)child;	// Uplink
	child->Node.Size = Length;
	child->Node.NumACLs = 1;
	child->Node.ACLs = &gVFS_ACL_EveryoneRO;
	child->Node.Read = SysFS_Comm_ReadFile;
	child->Node.Close = SysFS_Comm_CloseFile;
	
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
	
	Log("[SYSFS] Added '%s' (%p)", Path, Data);
	
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
int SysFS_UpdateFile(int ID, char *Data, int Length)
{
	tSysFS_Ent	*ent;
	
	for( ent = gSysFS_FileList; ent; ent = ent->Next )
	{
		// It's a reverse sorted list
		if(ent->Node.Inode < ID)	return 0;
		if(ent->Node.Inode == ID)
		{
			ent->Node.ImplPtr = Data;
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
	prev->ListNext = file->ListNext;
	file->Node.Size = 0;
	file->Node.ImplPtr = NULL;
	
	// Search parent directory
	for( ent = parent->Node.ImplPtr; ent; prev = ent, ent = ent->Next )
	{
		if( ent == file )	break;
	}
	if(!ent) {
		Warning("[SYSFS] Bookkeeping Error: File in list, but not in directory");
		return 0;
	}
	
	// Remove from parent directory
	prev->Next = ent->Next;
	
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
 * \fn tVFS_Node *SysFS_Comm_FindDir(tVFS_Node *Node, char *Filename)
 * \brief Find a file in a SysFS directory
 */
tVFS_Node *SysFS_Comm_FindDir(tVFS_Node *Node, char *Filename)
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
 * \fn Uint64 SysFS_Comm_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read from an exposed buffer
 */
Uint64 SysFS_Comm_ReadFile(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
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

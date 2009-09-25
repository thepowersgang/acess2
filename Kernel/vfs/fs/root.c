/* 
 * AcessMicro VFS
 * - Root Filesystem Driver
 */
#include <vfs.h>
#include <vfs_ramfs.h>

// === CONSTANTS ===
#define MAX_FILES	64

// === PROTOTYPES ===
tVFS_Node	*Root_InitDevice(char *Device, char *Options);
 int	Root_MkNod(tVFS_Node *Node, char *Name, Uint Flags);
tVFS_Node	*Root_FindDir(tVFS_Node *Node, char *Name);
char	*Root_ReadDir(tVFS_Node *Node, int Pos);
Uint64	Root_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Root_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
tRamFS_File	*Root_int_AllocFile();

// === GLOBALS ===
tVFS_Driver	gRootFS_Info = {
	"rootfs", 0, Root_InitDevice,
	NULL
};
tRamFS_File	RootFS_Files[MAX_FILES];
tVFS_ACL	RootFS_ACLs[3] = {
	{{0,0}, {0,VFS_PERM_ALL}},	// Owner (Root)
	{{1,0}, {0,VFS_PERM_ALL}},	// Group (Root)
	{{0,-1}, {0,VFS_PERM_ALL}}	// World (Nobody)
};

// === CODE ===
/**
 * \fn tVFS_Node *Root_InitDevice(char *Device, char *Options)
 * \brief Initialise the root filesystem
 */
tVFS_Node *Root_InitDevice(char *Device, char *Options)
{
	tRamFS_File	*root;
	if(strcmp(Device, "root") != 0) {
		return NULL;
	}
	
	// Create Root Node
	root = &RootFS_Files[0];
	
	root->Node.ImplPtr = root;
	
	root->Node.CTime
		= root->Node.MTime
		= root->Node.ATime = now();
	root->Node.NumACLs = 3;
	root->Node.ACLs = RootFS_ACLs;
	
	//root->Node.Close = Root_CloseFile;	// Not Needed (It's a RAM Disk!)
	//root->Node.Relink = Root_RelinkRoot;	// Not Needed (Why relink the root of the tree)
	root->Node.FindDir = Root_FindDir;
	root->Node.ReadDir = Root_ReadDir;
	root->Node.MkNod = Root_MkNod;
	
	return &root->Node;
}

/**
 * \fn int Root_MkNod(tVFS_Node *Node, char *Name, Uint Flags)
 * \brief Create an entry in the root directory
 */
int Root_MkNod(tVFS_Node *Node, char *Name, Uint Flags)
{
	tRamFS_File	*parent = Node->ImplPtr;
	tRamFS_File	*child = parent->Data.FirstChild;
	tRamFS_File	*prev = (tRamFS_File *) &parent->Data.FirstChild;
	
	Log("Root_MkNod: (Node=%p, Name='%s', Flags=0x%x)", Node, Name, Flags);
	
	// Find last child, while we're at it, check for duplication
	for( ; child; prev = child, child = child->Next )
	{
		if(strcmp(child->Name, Name) == 0)	return 0;
	}
	
	child = Root_int_AllocFile();
	memset(child, 0, sizeof(tRamFS_File));
	
	child->Name = malloc(strlen(Name)+1);
	strcpy(child->Name, Name);
	
	child->Parent = parent;
	child->Next = NULL;
	child->Data.FirstChild = NULL;
	
	child->Node.ImplPtr = child;
	child->Node.Flags = Flags;
	child->Node.NumACLs = 0;
	child->Node.Size = 0;
	
	if(Flags & VFS_FFLAG_DIRECTORY)
	{
		child->Node.ReadDir = Root_ReadDir;
		child->Node.FindDir = Root_FindDir;
		child->Node.MkNod = Root_MkNod;
	} else {
		child->Node.Read = Root_Read;
		child->Node.Write = Root_Write;
	}
	
	prev->Next = child;
	
	parent->Node.Size ++;
	
	return 1;
}

/**
 * \fn tVFS_Node *Root_FindDir(tVFS_Node *Node, char *Name)
 * \brief Find an entry in the filesystem
 */
tVFS_Node *Root_FindDir(tVFS_Node *Node, char *Name)
{
	tRamFS_File	*parent = Node->ImplPtr;
	tRamFS_File	*child = parent->Data.FirstChild;
	
	//Log("Root_FindDir: (Node=%p, Name='%s')", Node, Name);
	
	for(;child;child = child->Next)
	{
		//Log(" Root_FindDir: strcmp('%s', '%s')", child->Node.Name, Name);
		if(strcmp(child->Name, Name) == 0)	return &child->Node;
	}
	
	return NULL;
}

/**
 * \fn char *Root_ReadDir(tVFS_Node *Node, int Pos)
 * \brief Get an entry from the filesystem
 */
char *Root_ReadDir(tVFS_Node *Node, int Pos)
{
	tRamFS_File	*parent = Node->ImplPtr;
	tRamFS_File	*child = parent->Data.FirstChild;
	
	for( ; child && Pos--; child = child->Next ) ;
	
	if(Pos)	return child->Name;
	
	return child->Name;
}

/**
 * \fn Uint64 Root_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read from a file in the root directory
 */
Uint64 Root_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tRamFS_File	*file = Node->ImplPtr;
	
	if(Offset > Node->Size)	return 0;
	if(Length > Node->Size)	return 0;
	
	if(Offset+Length > Node->Size)
		Length = Node->Size - Offset;
	
	memcpy(Buffer, file->Data.Bytes+Offset, Length);
	
	return Length;
}

/**
 * \fn Uint64 Root_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write to a file in the root directory
 */
Uint64 Root_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tRamFS_File	*file = Node->ImplPtr;
	
	// Check if buffer needs to be expanded
	if(Offset + Length > Node->Size)
	{
		void *tmp = realloc( file->Data.Bytes, Offset + Length );
		if(tmp == NULL)	{
			Warning("Root_Write - Increasing buffer size failed\n");
			return -1;
		}
		file->Data.Bytes = tmp;
		Node->Size = Offset + Length;
		Log(" Root_Write: Expanded buffer to %i bytes\n", Node->Size);
	}
	
	memcpy(file->Data.Bytes+Offset, Buffer, Length);
	
	return Length;
}

/**
 * \fn tRamFS_File *Root_int_AllocFile()
 * \brief Allocates a file from the pool
 */
tRamFS_File *Root_int_AllocFile()
{
	 int	i;
	for( i = 0; i < MAX_FILES; i ++ )
	{
		if( RootFS_Files[i].Name == NULL )
		{
			return &RootFS_Files[i];
		}
	}
	return NULL;
}

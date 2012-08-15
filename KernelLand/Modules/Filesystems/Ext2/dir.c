/*
 * Acess2 Ext2 Driver
 * - By John Hodge (thePowersGang)
 *
 * dir.c
 * - Directory Handling
 */
#define DEBUG	0
#define VERBOSE	0
#include "ext2_common.h"

// === MACROS ===
#define BLOCK_DIR_OFS(_data, _block)	((Uint16*)(_data)[(_block)])

// === PROTOTYPES ===
char	*Ext2_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Ext2_FindDir(tVFS_Node *Node, const char *FileName);
 int	Ext2_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	Ext2_Unlink(tVFS_Node *Node, const char *OldName);
 int	Ext2_Link(tVFS_Node *Parent, const char *Name, tVFS_Node *Node);
// --- Helpers ---
tVFS_Node	*Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeId);

// === GLOBALS ===
tVFS_NodeType	gExt2_DirType = {
	.TypeName = "ext2-dir",
	.ReadDir = Ext2_ReadDir,
	.FindDir = Ext2_FindDir,
	.MkNod = Ext2_MkNod,
	.Unlink = Ext2_Unlink,
	.Link = Ext2_Link,
	.Close = Ext2_CloseFile
	};
tVFS_NodeType	gExt2_FileType = {
	.TypeName = "ext2-file",
	.Read = Ext2_Read,
	.Write = Ext2_Write,
	.Close = Ext2_CloseFile
	};

// === CODE ===
/**
 * \brief Reads a directory entry
 * \param Node	Directory node
 * \param Pos	Position of desired element
 */
char *Ext2_ReadDir(tVFS_Node *Node, int Pos)
{
	tExt2_Inode	inode;
	tExt2_DirEnt	dirent;
	Uint64	Base;	// Block's Base Address
	 int	block = 0;
	Uint	ofs = 0;
	 int	entNum = 0;
	tExt2_Disk	*disk = Node->ImplPtr;
	Uint	size;
	
	ENTER("pNode iPos", Node, Pos);
	
	// Read directory's inode
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	size = inode.i_size;
	
	LOG("inode={.i_block[0]= 0x%x, .i_size=0x%x}", inode.i_block[0], inode.i_size);
	
	// Find Entry
	// Get First Block
	// - Do this ourselves as it is a simple operation
	Base = inode.i_block[0] * disk->BlockSize;
	// Scan directory
	while(Pos -- && size > 0 && size <= inode.i_size)
	{
		VFS_ReadAt( disk->FD, Base+ofs, sizeof(tExt2_DirEnt), &dirent);
		ofs += dirent.rec_len;
		size -= dirent.rec_len;
		entNum ++;
		
		if(ofs >= disk->BlockSize) {
			block ++;
			if( ofs > disk->BlockSize ) {
				Log_Warning("EXT2", "Directory Entry %i of inode %i extends over a block boundary, ignoring",
					entNum-1, Node->Inode);
			}
			ofs = 0;
			Base = Ext2_int_GetBlockAddr( disk, inode.i_block, block );
			if( Base == 0 ) {
				size = 0;
				break;
			}
		}
	}
	
	// Check for the end of the list
	if(size <= 0 || size > inode.i_size) {
		LEAVE('n');
		return NULL;
	}
	
	// Read Entry
	VFS_ReadAt( disk->FD, Base+ofs, sizeof(tExt2_DirEnt), &dirent );
	LOG("dirent={.rec_len=%i,.inode=0x%x,.name_len=%i}",
		dirent.rec_len, dirent.inode, dirent.name_len);
	dirent.name[ dirent.name_len ] = '\0';	// Cap off string
	
	if( dirent.name_len == 0 ) {
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;
	}
	
	// Ignore . and .. (these are done in the VFS)
	if( (dirent.name[0] == '.' && dirent.name[1] == '\0')
	||  (dirent.name[0] == '.' && dirent.name[1] == '.' && dirent.name[2]=='\0')) {
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;	// Skip
	}
	
	LEAVE('s', dirent.name);
	// Create new node
	return strdup(dirent.name);
}

/**
 * \brief Gets information about a file
 * \param Node	Parent Node
 * \param Filename	Name of wanted file
 * \return VFS Node of file
 */
tVFS_Node *Ext2_FindDir(tVFS_Node *Node, const char *Filename)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	tExt2_DirEnt	dirent;
	Uint64	Base;	// Block's Base Address
	 int	block = 0;
	Uint	ofs = 0;
	 int	entNum = 0;
	Uint	size;
	 int	filenameLen = strlen(Filename);
	
	// Read directory's inode
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	size = inode.i_size;
	
	// Get First Block
	// - Do this ourselves as it is a simple operation
	Base = inode.i_block[0] * disk->BlockSize;
	// Find File
	while(size > 0)
	{
		VFS_ReadAt( disk->FD, Base+ofs, sizeof(tExt2_DirEnt), &dirent);
		// TODO: Possible overrun if name_len == 255?
		dirent.name[ dirent.name_len ] = '\0';	// Cap off string
		// If it matches, create a node and return it
		if(dirent.name_len == filenameLen && strcmp(dirent.name, Filename) == 0)
			return Ext2_int_CreateNode( disk, dirent.inode );
		// Increment pointers
		ofs += dirent.rec_len;
		size -= dirent.rec_len;
		entNum ++;
		
		// Check for end of block
		if(ofs >= disk->BlockSize) {
			block ++;
			if( ofs > disk->BlockSize ) {
				Log_Warning("EXT2", "Directory Entry %i of inode %i extends over a block boundary, ignoring",
					entNum-1, Node->Inode);
			}
			ofs = 0;
			Base = Ext2_int_GetBlockAddr( disk, inode.i_block, block );
		}
	}
	
	return NULL;
}

/**
 * \fn int Ext2_MkNod(tVFS_Node *Parent, const char *Name, Uint Flags)
 * \brief Create a new node
 */
int Ext2_MkNod(tVFS_Node *Parent, const char *Name, Uint Flags)
{
	ENTER("pParent sName xFlags", Parent, Name, Flags);
	
	Uint64 inodeNum = Ext2_int_AllocateInode(Parent->ImplPtr, Parent->Inode);
	if( inodeNum == 0 ) {
		return -1;
	}
	tVFS_Node *child = Ext2_int_CreateNode(Parent->ImplPtr, inodeNum);
	if( !child ) {
		Ext2_int_DereferenceInode(Parent->ImplPtr, inodeNum);
		return -1;
	}

	child->Flags = Flags & (VFS_FFLAG_DIRECTORY|VFS_FFLAG_SYMLINK|VFS_FFLAG_READONLY);
	child->UID = Threads_GetUID();
	child->GID = Threads_GetGID();
	child->CTime =
		child->MTime =
		child->ATime =
		now();
	child->ImplInt = 0;	// ImplInt is the link count
	// TODO: Set up ACLs

	int rv = Ext2_Link(Parent, Name, child);
	child->Type->Close(child);
	LEAVE('i', rv);
	return rv;
}

/**
 * \brief Rename a file
 * \param Node	This (directory) node
 * \param OldName	Old name of file
 * \param NewName	New name for file
 * \return Boolean Failure - See ::tVFS_Node.Unlink for info
 */
int Ext2_Unlink(tVFS_Node *Node, const char *OldName)
{
	return 1;
}

/**
 * \brief Links an existing node to a new name
 * \param Parent	Parent (directory) node
 * \param Name	New name for the node
 * \param Node	Node to link
 * \return Boolean Failure - See ::tVFS_Node.Link for info
 */
int Ext2_Link(tVFS_Node *Node, const char *Name, tVFS_Node *Child)
{	
	#if 0
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	tExt2_DirEnt	dirent;
	tExt2_DirEnt	newEntry;
	Uint64	Base;	// Block's Base Address
	 int	block = 0, ofs = 0;
	Uint	size;
	void	*blockData;
	 int	bestMatch = -1, bestSize, bestBlock, bestOfs;
	 int	nEntries;
	
	blockData = malloc(disk->BlockSize);
	
	// Read child inode (get's the file type)
	Ext2_int_ReadInode(disk, Child->Inode, &inode);
	
	// Create a stub entry
	newEntry.inode = Child->Inode;
	newEntry.name_len = strlen(Name);
	newEntry.rec_len = (newEntry.name_len+3+8)&~3;
	newEntry.type = inode.i_mode >> 12;
	memcpy(newEntry.name, Name, newEntry.name_len);
	
	// Read directory's inode
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	size = inode.i_size;
	
	// Get a lock on the inode
	Ext2_int_LockInode(disk, Node->Inode);
	
	// Get First Block
	// - Do this ourselves as it is a simple operation
	base = inode.i_block[0] * disk->BlockSize;
	VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
	block = 0;
	// Find File
	while(size > 0)
	{
		dirent = blockData + ofs;
		// Sanity Check the entry
		if(ofs + dirent->rec_len > disk->BlockSize) {
			Log_Warning("EXT2",
				"Directory entry %i of inode 0x%x extends over a block boundary",
				nEntries, (Uint)Node->Inode);
		}
		else {
		
			// Free entry
			if(dirent->type == 0) {
				if( dirent->rec_len >= newEntry.rec_len
				 && (bestMatch == -1 || bestSize > dirent->rec_len) )
				{
					bestMatch = nEntries;
					bestSize = dirent->rec_len;
					bestBlock = block;
					bestOfs = ofs;
				}
			}
			// Non free - check name to avoid duplicates
			else {
				if(strncmp(Name, dirent->name, dirent->name_len) == 0) {
					Ext2_int_UnlockInode(disk, Node->Inode);
					return 1;	// ERR_???
				}
			}
		}
		
		// Increment the pointer
		nEntries ++;
		ofs += dirent->rec_len;
		if( ofs >= disk->BlockSize ) {
			// Read the next block if needed
			BLOCK_DIR_OFS(Node->Data, block) = nEntries;
			block ++;
			ofs = 0;
			base = Ext2_int_GetBlockAddr(disk, inode.i_block, block);
			VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
		}
	}
	
	// Check if a free slot was found
	if( bestMatch >= 0 ) {
		// Read-Modify-Write
		bestBlock = Ext2_int_GetBlockAddr(disk, inode.i_block, bestBlock);
		if( block > 0 )
			bestMatch = BLOCK_DIR_OFS(Node->Data, bestBlock);
		VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
		dirent = blockData + bestOfs;
		memcpy(dirent, newEntry, newEntry.rec_len);
		VFS_WriteAt( disk->FD, base, disk->BlockSize, blockData );
	}
	else {
		// Allocate block, Write
		block = Ext2_int_AllocateBlock(Disk, block);
		Log_Warning("EXT2", "");
	}

	Ext2_int_UnlockInode(disk, Node->Inode);
	return 0;
	#else
	return 1;
	#endif
}

// ---- INTERNAL FUNCTIONS ----
/**
 * \fn vfs_node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID)
 * \brief Create a new VFS Node
 */
tVFS_Node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID)
{
	tExt2_Inode	inode;
	tVFS_Node	retNode;
	tVFS_Node	*tmpNode;
	
	if( !Ext2_int_ReadInode(Disk, InodeID, &inode) )
		return NULL;
	
	if( (tmpNode = Inode_GetCache(Disk->CacheID, InodeID)) )
		return tmpNode;

	memset(&retNode, 0, sizeof(retNode));	
	
	// Set identifiers
	retNode.Inode = InodeID;
	retNode.ImplPtr = Disk;
	retNode.ImplInt = inode.i_links_count;
	
	// Set file length
	retNode.Size = inode.i_size;
	
	// Set Access Permissions
	retNode.UID = inode.i_uid;
	retNode.GID = inode.i_gid;
	retNode.NumACLs = 3;
	retNode.ACLs = VFS_UnixToAcessACL(inode.i_mode & 0777, inode.i_uid, inode.i_gid);
	
	//  Set Function Pointers
	retNode.Type = &gExt2_FileType;
	
	switch(inode.i_mode & EXT2_S_IFMT)
	{
	// Symbolic Link
	case EXT2_S_IFLNK:
		retNode.Flags = VFS_FFLAG_SYMLINK;
		break;
	// Regular File
	case EXT2_S_IFREG:
		retNode.Flags = 0;
		retNode.Size |= (Uint64)inode.i_dir_acl << 32;
		break;
	// Directory
	case EXT2_S_IFDIR:
		retNode.Type = &gExt2_DirType;
		retNode.Flags = VFS_FFLAG_DIRECTORY;
		retNode.Data = calloc( sizeof(Uint16), DivUp(retNode.Size, Disk->BlockSize) );
		break;
	// Unknown, Write protect it to be safe 
	default:
		retNode.Flags = VFS_FFLAG_READONLY;
		break;
	}
	
	// Set Timestamps
	retNode.ATime = inode.i_atime * 1000;
	retNode.MTime = inode.i_mtime * 1000;
	retNode.CTime = inode.i_ctime * 1000;
	
	// Save in node cache and return saved node
	return Inode_CacheNode(Disk->CacheID, &retNode);
}

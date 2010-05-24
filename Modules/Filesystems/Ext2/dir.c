/*
 * Acess OS
 * Ext2 Driver Version 1
 */
/**
 * \file dir.c
 * \brief Second Extended Filesystem Driver
 * \todo Implement file full write support
 */
#define DEBUG	1
#define VERBOSE	0
#include "ext2_common.h"


// === PROTOTYPES ===
char	*Ext2_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Ext2_FindDir(tVFS_Node *Node, char *FileName);
 int	Ext2_MkNod(tVFS_Node *Node, char *Name, Uint Flags);
tVFS_Node	*Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeId, char *Name);

// === CODE ===
/**
 * \brief Reads a directory entry
 * \param Node	Directory node
 * \param Pos	Position of desired element
 */
char *Ext2_ReadDir(tVFS_Node *Node, int Pos)
{
	tExt2_Inode	inode;
	char	namebuf[EXT2_NAME_LEN+1];
	tExt2_DirEnt	dirent;
	Uint64	Base;	// Block's Base Address
	 int	block = 0, ofs = 0;
	 int	entNum = 0;
	tExt2_Disk	*disk = Node->ImplPtr;
	Uint	size;
	
	ENTER("pNode iPos", Node, Pos);
	
	// Read directory's inode
	//Ext2_int_GetInode(Node, &inode);
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	size = inode.i_size;
	
	LOG("inode.i_block[0] = 0x%x", inode.i_block[0]);
	
	// Find Entry
	// Get First Block
	// - Do this ourselves as it is a simple operation
	Base = inode.i_block[0] * disk->BlockSize;
	// Scan directory
	while(Pos -- && size > 0)
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
		}
	}
	
	// Check for the end of the list
	if(size <= 0) {
		LEAVE('n');
		return NULL;
	}
	
	// Read Entry
	VFS_ReadAt( disk->FD, Base+ofs, sizeof(tExt2_DirEnt), &dirent );
	//LOG("dirent.inode = %i", dirent.inode);
	//LOG("dirent.rec_len = %i", dirent.rec_len);
	//LOG("dirent.name_len = %i", dirent.name_len);
	VFS_ReadAt( disk->FD, Base+ofs+sizeof(tExt2_DirEnt), dirent.name_len, namebuf );
	namebuf[ dirent.name_len ] = '\0';	// Cap off string
	
	
	// Ignore . and .. (these are done in the VFS)
	if( (namebuf[0] == '.' && namebuf[1] == '\0')
	||  (namebuf[0] == '.' && namebuf[1] == '.' && namebuf[2]=='\0')) {
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;	// Skip
	}
	
	LEAVE('s', namebuf);
	// Create new node
	return strdup(namebuf);
}

/**
 * \brief Gets information about a file
 * \param Node	Parent Node
 * \param Filename	Name of wanted file
 * \return VFS Node of file
 */
tVFS_Node *Ext2_FindDir(tVFS_Node *Node, char *Filename)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	char	namebuf[EXT2_NAME_LEN+1];
	tExt2_DirEnt	dirent;
	Uint64	Base;	// Block's Base Address
	 int	block = 0, ofs = 0;
	 int	entNum = 0;
	Uint	size;
	
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
		VFS_ReadAt( disk->FD, Base+ofs+sizeof(tExt2_DirEnt), dirent.name_len, namebuf );
		namebuf[ dirent.name_len ] = '\0';	// Cap off string
		// If it matches, create a node and return it
		if(strcmp(namebuf, Filename) == 0)
			return Ext2_int_CreateNode( disk, dirent.inode, namebuf );
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
 * \fn int Ext2_MkNod(tVFS_Node *Parent, char *Name, Uint Flags)
 * \brief Create a new node
 */
int Ext2_MkNod(tVFS_Node *Parent, char *Name, Uint Flags)
{
	#if 0
	tVFS_Node	*child;
	child = Ext2_int_AllocateNode(Parent, Flags);
	return Ext2_Link(Parent, child, Name);
	#else
	return 0;
	#endif
}

/**
 * \brief Links an existing node to a new name
 * \param Parent	Parent (directory) node
 * \param Node	Node to link
 * \param Name	New name for the node
 * \return Boolean Failure - See ::tVFS_Node.Link for info
 */
int Ext2_Link(tVFS_Node *Parent, tVFS_Node *Node, char *Name)
{
	return 1;
}

// ---- INTERNAL FUNCTIONS ----
/**
 * \fn vfs_node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID, char *Name)
 * \brief Create a new VFS Node
 */
tVFS_Node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID, char *Name)
{
	tExt2_Inode	inode;
	tVFS_Node	retNode;
	tVFS_Node	*tmpNode;
	
	if( !Ext2_int_ReadInode(Disk, InodeID, &inode) )
		return NULL;
	
	if( (tmpNode = Inode_GetCache(Disk->CacheID, InodeID)) )
		return tmpNode;
	
	
	// Set identifiers
	retNode.Inode = InodeID;
	retNode.ImplPtr = Disk;
	
	// Set file length
	retNode.Size = inode.i_size;
	
	// Set Access Permissions
	retNode.UID = inode.i_uid;
	retNode.GID = inode.i_gid;
	retNode.NumACLs = 3;
	retNode.ACLs = VFS_UnixToAcessACL(inode.i_mode & 0777, inode.i_uid, inode.i_gid);
	
	//  Set Function Pointers
	retNode.Read = Ext2_Read;
	retNode.Write = Ext2_Write;
	retNode.Close = Ext2_CloseFile;
	
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
		retNode.ReadDir = Ext2_ReadDir;
		retNode.FindDir = Ext2_FindDir;
		retNode.MkNod = Ext2_MkNod;
		//retNode.Relink = Ext2_Relink;
		retNode.Flags = VFS_FFLAG_DIRECTORY;
		break;
	// Unknown, Write protect and hide it to be safe 
	default:
		retNode.Flags = VFS_FFLAG_READONLY;//|VFS_FFLAG_HIDDEN;
		break;
	}
	
	// Check if the file should be hidden
	//if(Name[0] == '.')	retNode.Flags |= VFS_FFLAG_HIDDEN;
	
	// Set Timestamps
	retNode.ATime = inode.i_atime * 1000;
	retNode.MTime = inode.i_mtime * 1000;
	retNode.CTime = inode.i_ctime * 1000;
	
	// Save in node cache and return saved node
	return Inode_CacheNode(Disk->CacheID, &retNode);
}

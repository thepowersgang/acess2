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
#define BLOCK_DIR_OFS(_data, _block)	(((Uint16*)(_data))[(_block)])

// === PROTOTYPES ===
 int	Ext2_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*Ext2_FindDir(tVFS_Node *Node, const char *FileName, Uint Flags);
tVFS_Node	*Ext2_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	Ext2_Unlink(tVFS_Node *Node, const char *OldName);
 int	Ext2_Link(tVFS_Node *Parent, const char *Name, tVFS_Node *Node);

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
int Ext2_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
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
		LEAVE('i', -ENOENT);
		return -ENOENT;
	}
	
	// Read Entry
	VFS_ReadAt( disk->FD, Base+ofs, sizeof(tExt2_DirEnt), &dirent );
	LOG("dirent={.rec_len=%i,.inode=0x%x,.name_len=%i}",
		dirent.rec_len, dirent.inode, dirent.name_len);
	dirent.name[ dirent.name_len ] = '\0';	// Cap off string
	
	if( dirent.name_len == 0 ) {
		LEAVE('i', 1);
		return 1;
	}
	
	// Ignore . and .. (these are done in the VFS)
	if( (dirent.name[0] == '.' && dirent.name[1] == '\0')
	||  (dirent.name[0] == '.' && dirent.name[1] == '.' && dirent.name[2]=='\0')) {
		LEAVE('i', 1);
		return 1;	// Skip
	}
	
	LOG("Name '%s'", dirent.name);
	strncpy(Dest, dirent.name, FILENAME_MAX);
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Gets information about a file
 * \param Node	Parent Node
 * \param Filename	Name of wanted file
 * \return VFS Node of file
 */
tVFS_Node *Ext2_FindDir(tVFS_Node *Node, const char *Filename, Uint Flags)
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
		// If it matches, create a node and return it
		if(dirent.name_len == filenameLen && strncmp(dirent.name, Filename, filenameLen) == 0)
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
tVFS_Node *Ext2_MkNod(tVFS_Node *Parent, const char *Name, Uint Flags)
{
	ENTER("pParent sName xFlags", Parent, Name, Flags);
	
	Uint64 inodeNum = Ext2_int_AllocateInode(Parent->ImplPtr, Parent->Inode);
	if( inodeNum == 0 ) {
		LOG("Inode allocation failed");
		LEAVE_RET('n', NULL);
	}
	tVFS_Node *child = Ext2_int_CreateNode(Parent->ImplPtr, inodeNum);
	if( !child ) {
		Ext2_int_DereferenceInode(Parent->ImplPtr, inodeNum);
		Log_Warning("Ext2", "Ext2_MkNod - Node creation failed");
		LEAVE_RET('n', NULL);
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
	if( rv ) {
		Ext2_CloseFile(child);
		return NULL;
	}
	LEAVE_RET('p', child);
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
	Log_Warning("Ext2", "TODO: Impliment Ext2_Unlink");
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
	tExt2_Disk	*disk = Node->ImplPtr;
	tExt2_Inode	inode;
	tExt2_DirEnt	*dirent;
	tExt2_DirEnt	newEntry;
	Uint64	base;	// Block's Base Address
	 int	block = 0, ofs = 0;
	Uint	size;
	 int	bestMatch = -1;
	 int	bestSize=0, bestBlock=0, bestOfs=0, bestNeedsSplit=0;
	 int	nEntries;

	ENTER("pNode sName pChild",
		Node, Name, Child);
	
	void *blockData = malloc(disk->BlockSize);
	
	// Read child inode (get's the file type)
	Ext2_int_ReadInode(disk, Child->Inode, &inode);
	
	// Create a stub entry
	newEntry.inode = Child->Inode;
	newEntry.name_len = strlen(Name);
	newEntry.rec_len = ((newEntry.name_len+3)&~3) + EXT2_DIRENT_SIZE;
	newEntry.type = inode.i_mode >> 12;
	memcpy(newEntry.name, Name, newEntry.name_len);
	
	// Read directory's inode
	Ext2_int_ReadInode(disk, Node->Inode, &inode);
	size = inode.i_size;
	
	// Get a lock on the inode
	//Ext2_int_LockInode(disk, Node->Inode);
	Mutex_Acquire(&Node->Lock);

//	if( !Node->Data ) {
//	}

	// Get First Block
	// - Do this ourselves as it is a simple operation
	base = inode.i_block[0] * disk->BlockSize;
	VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
	block = 0;
	nEntries = 0;
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
		else
		{
			LOG("Entry %i: %x %i bytes", nEntries, dirent->type, dirent->rec_len);
			// Free entry
			if(dirent->type == 0)
			{
				if( dirent->rec_len >= newEntry.rec_len
				 && (bestMatch == -1 || bestSize > dirent->rec_len) )
				{
					bestMatch = nEntries;
					bestSize = dirent->rec_len;
					bestBlock = block;
					bestOfs = ofs;
					bestNeedsSplit = 0;
				}
			}
			// Non free - check name to avoid duplicates
			else
			{
				LOG(" name='%.*s'", dirent->name_len, dirent->name);
				if(strncmp(Name, dirent->name, dirent->name_len) == 0) {
					//Ext2_int_UnlockInode(disk, Node->Inode);
					goto _err;
				}
				
				 int	spare_space = dirent->rec_len - (dirent->name_len + EXT2_DIRENT_SIZE);
				if( spare_space > newEntry.rec_len
				 && (bestMatch == -1 || bestSize > spare_space) )
				{
					bestMatch = nEntries;
					bestSize = spare_space;
					bestBlock = block;
					bestOfs = ofs;
					bestNeedsSplit = 1;
				}
			}
		}
		
		// Increment the pointer
		nEntries ++;
		ofs += dirent->rec_len;
		size -= dirent->rec_len;
		if( size > 0 && ofs >= disk->BlockSize ) {
			// Read the next block if needed
		//	BLOCK_DIR_OFS(Node->Data, block) = nEntries;
			block ++;
			ofs = 0;
			base = Ext2_int_GetBlockAddr(disk, inode.i_block, block);
			VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
		}
	}
	
	LOG("bestMatch = %i", bestMatch);
	// If EOF was reached with no space, check if we can fit one on the end
	if( bestMatch < 0 && ofs + newEntry.rec_len < disk->BlockSize ) {
		Node->Size += newEntry.rec_len;
		Node->Flags |= VFS_FFLAG_DIRTY;
		bestBlock = block;
		bestOfs = ofs;
		bestSize = newEntry.rec_len;
		bestNeedsSplit = 0;
	}
	// Check if a free slot was found
	if( bestMatch >= 0 )
	{
		// Read-Modify-Write
		base = Ext2_int_GetBlockAddr(disk, inode.i_block, bestBlock);
		VFS_ReadAt( disk->FD, base, disk->BlockSize, blockData );
		dirent = blockData + bestOfs;
		// Shorten a pre-existing entry
		if(bestNeedsSplit)
		{
			dirent->rec_len = EXT2_DIRENT_SIZE + dirent->name_len;
			bestOfs += dirent->rec_len;
			//bestSize -= dirent->rec_len; // (not needed, bestSize is the spare space after)
			dirent = blockData + bestOfs;
		}
		// Insert new file entry
		memcpy(dirent, &newEntry, newEntry.rec_len);
		// Create a new blank entry
		if( bestSize != newEntry.rec_len )
		{
			bestOfs += newEntry.rec_len;
			dirent = blockData + bestOfs;

			dirent->rec_len = bestSize - newEntry.rec_len;			
			dirent->type = 0;
		}
		// Save changes
		VFS_WriteAt( disk->FD, base, disk->BlockSize, blockData );
	}
	else {
		// Allocate block, Write
		Uint32 newblock = Ext2_int_AllocateBlock(disk, base / disk->BlockSize);
		Ext2_int_AppendBlock(disk, &inode, newblock);
		base = newblock * disk->BlockSize;
		Node->Size += newEntry.rec_len;
		Node->Flags |= VFS_FFLAG_DIRTY;
		memcpy(blockData, &newEntry, newEntry.rec_len);
		memset(blockData + newEntry.rec_len, 0, disk->BlockSize - newEntry.rec_len);
		VFS_WriteAt( disk->FD, base, disk->BlockSize, blockData );
	}

	Child->ImplInt ++;
	Child->Flags |= VFS_FFLAG_DIRTY;

	//Ext2_int_UnlockInode(disk, Node->Inode);
	free(blockData);
	Mutex_Release(&Node->Lock);
	LEAVE('i', 0);
	return 0;
_err:
	free(blockData);
	Mutex_Release(&Node->Lock);
	LEAVE('i', 1);
	return 1;	// ERR_???
}


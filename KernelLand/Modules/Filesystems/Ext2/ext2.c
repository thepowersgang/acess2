/*
 * Acess2 Ext2 Driver
 * - By John Hodge (thePowersGang)
 *
 * ext2.c
 * - Driver core
 */
#define DEBUG	0
#define VERSION	VER2(0,90)
#include "ext2_common.h"
#include <modules.h>

#define MIN_BLOCKS_PER_GROUP	2
#define MAX_BLOCK_LOG_SIZE	10	// 1024 << 10 = 1MiB

// === PROTOTYPES ===
 int	Ext2_Install(char **Arguments);
 int	Ext2_Cleanup(void);
// - Interface Functions
 int    Ext2_Detect(int FD);
tVFS_Node	*Ext2_InitDevice(const char *Device, const char **Options);
void	Ext2_Unmount(tVFS_Node *Node);
void	Ext2_CloseFile(tVFS_Node *Node);
tVFS_Node	*Ext2_GetNodeFromINode(tVFS_Node *RootNode, Uint64 Inode);
// - Internal Helpers
 int	Ext2_int_GetInode(tVFS_Node *Node, tExt2_Inode *Inode);
void	Ext2_int_DumpInode(tExt2_Disk *Disk, Uint32 InodeID, tExt2_Inode *Inode);
Uint64	Ext2_int_GetBlockAddr(tExt2_Disk *Disk, Uint32 *Blocks, int BlockNum);
Uint32	Ext2_int_AllocateInode(tExt2_Disk *Disk, Uint32 Parent);
void	Ext2_int_DereferenceInode(tExt2_Disk *Disk, Uint32 Inode);
void	Ext2_int_UpdateSuperblock(tExt2_Disk *Disk);

// === SEMI-GLOBALS ===
MODULE_DEFINE(0, VERSION, FS_Ext2, Ext2_Install, Ext2_Cleanup);
tExt2_Disk	gExt2_disks[6];
 int	giExt2_count = 0;
tVFS_Driver	gExt2_FSInfo = {
	.Name = "ext2",
	.Detect = Ext2_Detect,
	.InitDevice = Ext2_InitDevice,
	.Unmount = Ext2_Unmount,
	.GetNodeFromINode = Ext2_GetNodeFromINode
	};

// === CODE ===
/**
 * \fn int Ext2_Install(char **Arguments)
 * \brief Install the Ext2 Filesystem Driver
 */
int Ext2_Install(char **Arguments)
{
	VFS_AddDriver( &gExt2_FSInfo );
	return MODULE_ERR_OK;
}

/**
 * \brief Clean up driver state before unload
 */
int Ext2_Cleanup(void)
{
	return 0;
}

/**
 * Detect if a volume is Ext2 formatted
 */
int Ext2_Detect(int FD)
{
	tExt2_SuperBlock	sb;
	size_t	len;
	
	len = VFS_ReadAt(FD, 1024, 1024, &sb);

	if( len != 1024 ) {
		Log_Debug("Ext2", "_Detect: Read failed? (0x%x != 1024)", len);
		return 0;
	}
	
	switch(sb.s_magic)
	{
	case 0xEF53:
		return 2;
	default:
		Log_Debug("Ext2", "_Detect: s_magic = 0x%x", sb.s_magic);
		return 0;
	}
}

/**
 \brief Initializes a device to be read by by the driver
 \param Device	String - Device to read from
 \param Options	NULL Terminated array of option strings
 \return Root Node
*/
tVFS_Node *Ext2_InitDevice(const char *Device, const char **Options)
{
	tExt2_Disk	*disk = NULL;
	 int	fd;
	 int	groupCount;
	tExt2_SuperBlock	sb;
	
	ENTER("sDevice pOptions", Device, Options);
	
	// Open Disk
	fd = VFS_Open(Device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);		//Open Device
	if(fd == -1) {
		Log_Warning("EXT2", "Unable to open '%s'", Device);
		LEAVE('n');
		return NULL;
	}
	
	// Read Superblock at offset 1024
	VFS_ReadAt(fd, 1024, 1024, &sb);	// Read Superblock
	
	// Sanity Check Magic value
	if(sb.s_magic != 0xEF53) {
		Log_Warning("EXT2", "Volume '%s' is not an EXT2 volume (0x%x != 0xEF53)",
			Device, sb.s_magic);
		goto _error;
	}

	if( sb.s_blocks_per_group < MIN_BLOCKS_PER_GROUP ) {
		Log_Warning("Ext2", "Blocks per group is too small (%i < %i)",
			sb.s_blocks_per_group, MIN_BLOCKS_PER_GROUP);
		goto _error;
	}	

	// Get Group count
	groupCount = DivUp(sb.s_blocks_count, sb.s_blocks_per_group);
	LOG("groupCount = %i", groupCount);
	
	// Allocate Disk Information
	disk = malloc(sizeof(tExt2_Disk) + sizeof(tExt2_Group)*groupCount);
	if(!disk) {
		Log_Warning("EXT2", "Unable to allocate disk structure");
		goto _error;
	}
	disk->FD = fd;
	memcpy(&disk->SuperBlock, &sb, 1024);
	disk->GroupCount = groupCount;
	
	// Get an inode cache handle
	disk->CacheID = Inode_GetHandle(NULL);
	
	// Get Block Size
	if( sb.s_log_block_size > MAX_BLOCK_LOG_SIZE ) {
		Log_Warning("Ext2", "Block size (log2) too large (%i > %i)",
			sb.s_log_block_size, MAX_BLOCK_LOG_SIZE);
		goto _error;
	}
	disk->BlockSize = 1024 << sb.s_log_block_size;
	LOG("Disk->BlockSie = 0x%x (1024 << %i)", disk->BlockSize, sb.s_log_block_size);
	
	// Read Group Information
	LOG("sb,s_first_data_block = %x", sb.s_first_data_block);
	VFS_ReadAt(
		disk->FD,
		sb.s_first_data_block * disk->BlockSize + 1024,
		sizeof(tExt2_Group)*groupCount,
		disk->Groups
		);
	
	LOG("Block Group 0");
	LOG(".bg_block_bitmap = 0x%x", disk->Groups[0].bg_block_bitmap);
	LOG(".bg_inode_bitmap = 0x%x", disk->Groups[0].bg_inode_bitmap);
	LOG(".bg_inode_table = 0x%x", disk->Groups[0].bg_inode_table);
	LOG("Block Group 1");
	LOG(".bg_block_bitmap = 0x%x", disk->Groups[1].bg_block_bitmap);
	LOG(".bg_inode_bitmap = 0x%x", disk->Groups[1].bg_inode_bitmap);
	LOG(".bg_inode_table = 0x%x", disk->Groups[1].bg_inode_table);
	
	// Get root Inode
	Ext2_int_ReadInode(disk, 2, &disk->RootInode);
	
	// Create Root Node
	memset(&disk->RootNode, 0, sizeof(tVFS_Node));
	disk->RootNode.Inode = 2;	// Root inode ID
	disk->RootNode.ImplPtr = disk;	// Save disk pointer
	disk->RootNode.Size = -1;	// Fill in later (on readdir)
	disk->RootNode.Flags = VFS_FFLAG_DIRECTORY;

	disk->RootNode.Type = &gExt2_DirType;
	
	// Complete root node
	disk->RootNode.UID = disk->RootInode.i_uid;
	disk->RootNode.GID = disk->RootInode.i_gid;
	disk->RootNode.NumACLs = 1;
	disk->RootNode.ACLs = &gVFS_ACL_EveryoneRW;
	
	#if DEBUG
	LOG("inode.i_size = 0x%x", disk->RootInode.i_size);
	LOG("inode.i_block[0] = 0x%x", disk->RootInode.i_block[0]);
	#endif
	
	LEAVE('p', &disk->RootNode);
	return &disk->RootNode;
_error:
	if( disk )
		free(disk);
	VFS_Close(fd);
	LEAVE('n');
	return NULL;
}

/**
 * \fn void Ext2_Unmount(tVFS_Node *Node)
 * \brief Close a mounted device
 */
void Ext2_Unmount(tVFS_Node *Node)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	
	VFS_Close( disk->FD );
	Inode_ClearCache( disk->CacheID );
	memset(disk, 0, sizeof(tExt2_Disk)+disk->GroupCount*sizeof(tExt2_Group));
	free(disk);
}

/**
 * \fn void Ext2_CloseFile(tVFS_Node *Node)
 * \brief Close a file (Remove it from the cache)
 */
void Ext2_CloseFile(tVFS_Node *Node)
{
	tExt2_Disk	*disk = Node->ImplPtr;
	ENTER("pNode", Node);

	if( Mutex_Acquire(&Node->Lock) != 0 )
	{
		LEAVE('-');
		return ;
	}

	if( Node->Flags & VFS_FFLAG_DIRTY )
	{
		// Commit changes
		Ext2_int_WritebackNode(disk, Node);
		Node->Flags &= ~VFS_FFLAG_DIRTY;
	}

	int was_not_referenced = (Node->ImplInt == 0);
	tVFS_ACL	*acls = Node->ACLs;
	if( Inode_UncacheNode(disk->CacheID, Node->Inode) == 1 )
	{
		if( was_not_referenced )
		{
			LOG("Removng inode");
			// Remove inode
			Log_Warning("Ext2", "TODO: Remove inode when not referenced (%x)", (Uint32)Node->Inode);
		}
		if( acls != &gVFS_ACL_EveryoneRW ) {
			free(acls);
		}
		LOG("Node cleaned");
	}
	else {
		LOG("Still referenced, releasing lock");
		Mutex_Release(&Node->Lock);
	}
	LEAVE('-');
	return ;
}

tVFS_Node *Ext2_GetNodeFromINode(tVFS_Node *RootNode, Uint64 Inode)
{
	return Ext2_int_CreateNode(RootNode->ImplPtr, Inode);
}

//==================================
//=       INTERNAL FUNCTIONS       =
//==================================
/**
 * \fn int Ext2_int_ReadInode(tExt2_Disk *Disk, Uint InodeId, tExt2_Inode *Inode)
 * \brief Read an inode into memory
 */
int Ext2_int_ReadInode(tExt2_Disk *Disk, Uint32 InodeId, tExt2_Inode *Inode)
{
	 int	group, subId;
	
	ENTER("pDisk iInodeId pInode", Disk, InodeId, Inode);
	
	if(InodeId == 0)	return 0;
	
	InodeId --;	// Inodes are numbered starting at 1
	
	group = InodeId / Disk->SuperBlock.s_inodes_per_group;
	subId = InodeId % Disk->SuperBlock.s_inodes_per_group;
	
	LOG("group=%i, subId = %i", group, subId);
	
	// Read Inode
	VFS_ReadAt(Disk->FD,
		Disk->Groups[group].bg_inode_table * Disk->BlockSize + sizeof(tExt2_Inode)*subId,
		sizeof(tExt2_Inode),
		Inode);
	
	LEAVE('i', 1);
	return 1;
}

/**
 * \brief Write a modified inode out to disk
 */
int Ext2_int_WriteInode(tExt2_Disk *Disk, Uint32 InodeId, tExt2_Inode *Inode)
{
	 int	group, subId;
	ENTER("pDisk iInodeId pInode", Disk, InodeId, Inode);
	
	if(InodeId == 0) {
		LEAVE('i', 0);
		return 0;
	}

	Ext2_int_DumpInode(Disk, InodeId, Inode);	

	InodeId --;	// Inodes are numbered starting at 1
	
	group = InodeId / Disk->SuperBlock.s_inodes_per_group;
	subId = InodeId % Disk->SuperBlock.s_inodes_per_group;
	
	LOG("group=%i, subId = %i", group, subId);
	
	// Write Inode
	VFS_WriteAt(Disk->FD,
		Disk->Groups[group].bg_inode_table * Disk->BlockSize + sizeof(tExt2_Inode)*subId,
		sizeof(tExt2_Inode),
		Inode
		);
	
	LEAVE('i', 1);
	return 1;
}

/**
 * \fn vfs_node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID)
 * \brief Create a new VFS Node
 */
tVFS_Node *Ext2_int_CreateNode(tExt2_Disk *Disk, Uint InodeID)
{
	struct {
		tVFS_Node	retNode;
		tExt2_Inode	inode;
	} data;
	tVFS_Node	*node = &data.retNode;
	tExt2_Inode	*in = &data.inode;
	
	if( !Ext2_int_ReadInode(Disk, InodeID, &data.inode) )
		return NULL;
	
	if( (node = Inode_GetCache(Disk->CacheID, InodeID)) )
		return node;
	node = &data.retNode;

	memset(node, 0, sizeof(*node));
	
	// Set identifiers
	node->Inode = InodeID;
	node->ImplPtr = Disk;
	node->ImplInt = in->i_links_count;
	if( in->i_links_count == 0 ) {
		Log_Notice("Ext2", "Inode %p:%x is not referenced, bug?", Disk, InodeID);
	}
	
	// Set file length
	node->Size = in->i_size;
	
	// Set Access Permissions
	node->UID = in->i_uid;
	node->GID = in->i_gid;
	node->NumACLs = 3;
	node->ACLs = VFS_UnixToAcessACL(in->i_mode & 0777, in->i_uid, in->i_gid);
	
	//  Set Function Pointers
	node->Type = &gExt2_FileType;
	
	switch(in->i_mode & EXT2_S_IFMT)
	{
	// Symbolic Link
	case EXT2_S_IFLNK:
		node->Flags = VFS_FFLAG_SYMLINK;
		break;
	// Regular File
	case EXT2_S_IFREG:
		node->Flags = 0;
		node->Size |= (Uint64)in->i_dir_acl << 32;
		break;
	// Directory
	case EXT2_S_IFDIR:
		node->Type = &gExt2_DirType;
		node->Flags = VFS_FFLAG_DIRECTORY;
		node->Data = calloc( sizeof(Uint16), DivUp(node->Size, Disk->BlockSize) );
		break;
	// Unknown, Write protect it to be safe 
	default:
		node->Flags = VFS_FFLAG_READONLY;
		break;
	}
	
	// Set Timestamps
	node->ATime = in->i_atime * 1000;
	node->MTime = in->i_mtime * 1000;
	node->CTime = in->i_ctime * 1000;
	
	// Save in node cache and return saved node
	return Inode_CacheNodeEx(Disk->CacheID, &data.retNode, sizeof(data));
}

int Ext2_int_WritebackNode(tExt2_Disk *Disk, tVFS_Node *Node)
{
	tExt2_Inode	*inode = (void*)(Node+1);

	if( Disk != Node->ImplPtr ) {
		Log_Error("Ext2", "Ext2_int_WritebackNode - Disk != Node->ImplPtr");
		return -1;
	}

	if( Node->Flags & VFS_FFLAG_SYMLINK ) {
		inode->i_mode = EXT2_S_IFLNK;
	}
	else if( Node->Flags & VFS_FFLAG_DIRECTORY ) {
		inode->i_mode = EXT2_S_IFDIR;
	}
	else if( Node->Flags & VFS_FFLAG_READONLY ) {
		Log_Notice("Ext2", "Not writing back readonly inode %p:%x", Disk, Node->Inode);
		return 1;
	}
	else {
		inode->i_mode = EXT2_S_IFREG;
		inode->i_dir_acl = Node->Size >> 32;
	}

	inode->i_size = Node->Size & 0xFFFFFFFF;
	inode->i_links_count = Node->ImplInt;

	inode->i_uid = Node->UID;
	inode->i_gid = Node->GID;

	inode->i_atime = Node->ATime / 1000;
	inode->i_mtime = Node->MTime / 1000;
	inode->i_ctime = Node->CTime / 1000;

	// TODO: Compact ACLs into unix mode
	Log_Warning("Ext2", "TODO: Support converting Acess ACLs into unix modes");
	inode->i_mode |= 777;

	Ext2_int_WriteInode(Disk, Node->Inode, inode);

	return 0;
}

void Ext2_int_DumpInode(tExt2_Disk *Disk, Uint32 InodeID, tExt2_Inode *Inode)
{
	LOG("%p[Inode %i] = {", Disk, InodeID);
	LOG(" .i_mode = 0%04o", Inode->i_mode);
	LOG(" .i_uid:i_gid = %i:%i", Inode->i_uid, Inode->i_gid);
	LOG(" .i_size = 0x%x", Inode->i_size);
	LOG(" .i_block[0:3] = {0x%x,0x%x,0x%x,0x%x}",
		Inode->i_block[0], Inode->i_block[1], Inode->i_block[2], Inode->i_block[3]);
	LOG(" .i_block[4:7] = {0x%x,0x%x,0x%x,0x%x}",
		Inode->i_block[4], Inode->i_block[5], Inode->i_block[6], Inode->i_block[7]);
	LOG(" .i_block[8:11] = {0x%x,0x%x,0x%x,0x%x}",
		Inode->i_block[8], Inode->i_block[6], Inode->i_block[10], Inode->i_block[11]);
	LOG(" .i_block[12:14] = {0x%x,0x%x,0x%x}",
		Inode->i_block[12], Inode->i_block[13], Inode->i_block[14]);
	LOG("}");
}

/**
 * \fn Uint64 Ext2_int_GetBlockAddr(tExt2_Disk *Disk, Uint32 *Blocks, int BlockNum)
 * \brief Get the address of a block from an inode's list
 * \param Disk	Disk information structure
 * \param Blocks	Pointer to an inode's block list
 * \param BlockNum	Block index in list
 */
Uint64 Ext2_int_GetBlockAddr(tExt2_Disk *Disk, Uint32 *Blocks, int BlockNum)
{
	Uint32	*iBlocks;
	 int	dwPerBlock = Disk->BlockSize / 4;
	
	// Direct Blocks
	if(BlockNum < 12)
		return (Uint64)Blocks[BlockNum] * Disk->BlockSize;
	
	// Single Indirect Blocks
	iBlocks = malloc( Disk->BlockSize );
	VFS_ReadAt(Disk->FD, (Uint64)Blocks[12]*Disk->BlockSize, Disk->BlockSize, iBlocks);
	
	BlockNum -= 12;
	if(BlockNum < dwPerBlock)
	{
		BlockNum = iBlocks[BlockNum];
		free(iBlocks);
		return (Uint64)BlockNum * Disk->BlockSize;
	}
	
	BlockNum -= dwPerBlock;
	// Double Indirect Blocks
	if(BlockNum < dwPerBlock*dwPerBlock)
	{
		VFS_ReadAt(Disk->FD, (Uint64)Blocks[13]*Disk->BlockSize, Disk->BlockSize, iBlocks);
		VFS_ReadAt(Disk->FD, (Uint64)iBlocks[BlockNum/dwPerBlock]*Disk->BlockSize, Disk->BlockSize, iBlocks);
		BlockNum = iBlocks[BlockNum%dwPerBlock];
		free(iBlocks);
		return (Uint64)BlockNum * Disk->BlockSize;
	}
	
	BlockNum -= dwPerBlock*dwPerBlock;
	// Triple Indirect Blocks
	VFS_ReadAt(Disk->FD, (Uint64)Blocks[14]*Disk->BlockSize, Disk->BlockSize, iBlocks);
	VFS_ReadAt(Disk->FD, (Uint64)iBlocks[BlockNum/(dwPerBlock*dwPerBlock)]*Disk->BlockSize, Disk->BlockSize, iBlocks);
	VFS_ReadAt(Disk->FD, (Uint64)iBlocks[(BlockNum/dwPerBlock)%dwPerBlock]*Disk->BlockSize, Disk->BlockSize, iBlocks);
	BlockNum = iBlocks[BlockNum%dwPerBlock];
	free(iBlocks);
	return (Uint64)BlockNum * Disk->BlockSize;
}

/**
 * \fn Uint32 Ext2_int_AllocateInode(tExt2_Disk *Disk, Uint32 Parent)
 * \brief Allocate an inode (from the current group preferably)
 * \param Disk	EXT2 Disk Information Structure
 * \param Parent	Inode ID of the parent (used to locate the child nearby)
 */
Uint32 Ext2_int_AllocateInode(tExt2_Disk *Disk, Uint32 Parent)
{
	Uint	start_group = (Parent - 1) / Disk->SuperBlock.s_inodes_per_group;
	Uint	group = start_group;

	if( Disk->SuperBlock.s_free_inodes_count == 0 ) 
	{
		Log_Notice("Ext2", "Ext2_int_AllocateInode - Out of inodes on %p", Disk);
		return 0;
	}

	while( group < Disk->GroupCount && Disk->Groups[group].bg_free_inodes_count == 0 )
		group ++;
	if( group == Disk->GroupCount )
	{
		group = 0;
		while( group < start_group && Disk->Groups[group].bg_free_inodes_count == 0 )
			group ++;
	}
	
	if( Disk->Groups[group].bg_free_inodes_count == 0 )
	{
		Log_Notice("Ext2", "Ext2_int_AllocateInode - Out of inodes on %p, but superblock says some free", Disk);
		return 0;
	}

	// Load bitmap for group
	//  (s_inodes_per_group / 8) bytes worth
	// - Allocate a buffer the size of a sector/block
	// - Read in part of the bitmap
	// - Search for a free inode
	tExt2_Group	*bg = &Disk->Groups[group];
	 int	ofs = 0;
	do {
		const int sector_size = 512;
		Uint8 buf[sector_size];
		VFS_ReadAt(Disk->FD, Disk->BlockSize*bg->bg_inode_bitmap+ofs, sector_size, buf);

		int byte, bit;
		for( byte = 0; byte < sector_size && buf[byte] == 0xFF; byte ++ )
			;
		if( byte < sector_size )
		{
			for( bit = 0; bit < 8 && buf[byte] & (1 << bit); bit ++)
				;
			ASSERT(bit != 8);
			buf[byte] |= 1 << bit;
			VFS_WriteAt(Disk->FD, Disk->BlockSize*bg->bg_inode_bitmap+ofs, sector_size, buf);

			bg->bg_free_inodes_count --;
			Disk->SuperBlock.s_free_inodes_count --;

			Uint32	ret = group * Disk->SuperBlock.s_inodes_per_group + byte * 8 + bit + 1;
			Log_Debug("Ext2", "Ext2_int_AllocateInode - Allocated 0x%x", ret);
			return ret;
		}

		ofs += sector_size;
	} while(ofs < Disk->SuperBlock.s_inodes_per_group / 8);

	Log_Notice("Ext2", "Ext2_int_AllocateInode - Out of inodes in group %p:%i but header reported free",
		Disk, group);

	return 0;
}

/**
 * \brief Reduce the reference count on an inode
 */
void Ext2_int_DereferenceInode(tExt2_Disk *Disk, Uint32 Inode)
{
	Log_Warning("Ext2", "TODO: Impliment Ext2_int_DereferenceInode");
}

/**
 * \fn void Ext2_int_UpdateSuperblock(tExt2_Disk *Disk)
 * \brief Updates the superblock
 */
void Ext2_int_UpdateSuperblock(tExt2_Disk *Disk)
{
	 int	bpg = Disk->SuperBlock.s_blocks_per_group;
	 int	ngrp = Disk->SuperBlock.s_blocks_count / bpg;
	 int	i;
	 
	// Update Primary
	VFS_WriteAt(Disk->FD, 1024, 1024, &Disk->SuperBlock);
	
	// - Update block groups while we're at it
	VFS_WriteAt(
		Disk->FD,
		Disk->SuperBlock.s_first_data_block * Disk->BlockSize + 1024,
		sizeof(tExt2_Group)*Disk->GroupCount,
		Disk->Groups
		);
	
	// Secondaries
	// at Block Group 1, 3^n, 5^n, 7^n
	
	// 1
	if(ngrp <= 1)	return;
	VFS_WriteAt(Disk->FD, 1*bpg*Disk->BlockSize, 1024, &Disk->SuperBlock);
	
	#define INT_MAX	(((long long int)1<<(sizeof(int)*8))-1)
	
	// Powers of 3
	for( i = 3; i < ngrp && i < INT_MAX/3; i *= 3 )
		VFS_WriteAt(Disk->FD, i*bpg*Disk->BlockSize, 1024, &Disk->SuperBlock);
	
	// Powers of 5
	for( i = 5; i < ngrp && i < INT_MAX/5; i *= 5 )
		VFS_WriteAt(Disk->FD, i*bpg*Disk->BlockSize, 1024, &Disk->SuperBlock);
	
	// Powers of 7
	for( i = 7; i < ngrp && i < INT_MAX/7; i *= 7 )
		VFS_WriteAt(Disk->FD, i*bpg*Disk->BlockSize, 1024, &Disk->SuperBlock);
}

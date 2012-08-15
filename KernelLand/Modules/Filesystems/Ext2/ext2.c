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

// === IMPORTS ===
extern tVFS_NodeType	gExt2_DirType;

// === PROTOTYPES ===
 int	Ext2_Install(char **Arguments);
 int	Ext2_Cleanup(void);
// - Interface Functions
 int    Ext2_Detect(int FD);
tVFS_Node	*Ext2_InitDevice(const char *Device, const char **Options);
void	Ext2_Unmount(tVFS_Node *Node);
void	Ext2_CloseFile(tVFS_Node *Node);
// - Internal Helpers
 int	Ext2_int_GetInode(tVFS_Node *Node, tExt2_Inode *Inode);
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
	.GetNodeFromINode = NULL
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
	tExt2_Disk	*disk;
	 int	fd;
	 int	groupCount;
	tExt2_SuperBlock	sb;
	tExt2_Inode	inode;
	
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
		VFS_Close(fd);
		LEAVE('n');
		return NULL;
	}
	
	// Get Group count
	groupCount = DivUp(sb.s_blocks_count, sb.s_blocks_per_group);
	LOG("groupCount = %i", groupCount);
	
	// Allocate Disk Information
	disk = malloc(sizeof(tExt2_Disk) + sizeof(tExt2_Group)*groupCount);
	if(!disk) {
		Log_Warning("EXT2", "Unable to allocate disk structure");
		VFS_Close(fd);
		LEAVE('n');
		return NULL;
	}
	disk->FD = fd;
	memcpy(&disk->SuperBlock, &sb, 1024);
	disk->GroupCount = groupCount;
	
	// Get an inode cache handle
	disk->CacheID = Inode_GetHandle();
	
	// Get Block Size
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
	Ext2_int_ReadInode(disk, 2, &inode);
	
	// Create Root Node
	memset(&disk->RootNode, 0, sizeof(tVFS_Node));
	disk->RootNode.Inode = 2;	// Root inode ID
	disk->RootNode.ImplPtr = disk;	// Save disk pointer
	disk->RootNode.Size = -1;	// Fill in later (on readdir)
	disk->RootNode.Flags = VFS_FFLAG_DIRECTORY;

	disk->RootNode.Type = &gExt2_DirType;
	
	// Complete root node
	disk->RootNode.UID = inode.i_uid;
	disk->RootNode.GID = inode.i_gid;
	disk->RootNode.NumACLs = 1;
	disk->RootNode.ACLs = &gVFS_ACL_EveryoneRW;
	
	#if DEBUG
	LOG("inode.i_size = 0x%x", inode.i_size);
	LOG("inode.i_block[0] = 0x%x", inode.i_block[0]);
	#endif
	
	LEAVE('p', &disk->RootNode);
	return &disk->RootNode;
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
		Log_Warning("Ext2", "TODO: Commit node changes");
	}

	int was_not_referenced = (Node->ImplInt == 0);
	tVFS_ACL	*acls = Node->ACLs;
	if( Inode_UncacheNode(disk->CacheID, Node->Inode) == 1 )
	{
		if( was_not_referenced )
		{
			LOG("Removng inode");
			// Remove inode
			Log_Warning("Ext2", "TODO: Remove inode when not deleted");
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
//	Uint	block = (Parent - 1) / Disk->SuperBlock.s_inodes_per_group;
	Log_Warning("EXT2", "Ext2_int_AllocateInode is unimplemented");
	return 0;
}

/**
 * \brief Reduce the reference count on an inode
 */
void Ext2_int_DereferenceInode(tExt2_Disk *Disk, Uint32 Inode)
{
	
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

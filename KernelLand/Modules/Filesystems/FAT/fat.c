/*
 * Acess 2
 * FAT12/16/32 Driver Version (Incl LFN)
 * 
 * NOTE: This driver will only support _reading_ long file names, not
 * writing. I don't even know why I'm adding write-support. FAT sucks.
 * 
 * Known Bugs:
 * - LFN Is buggy in FAT_ReadDir
 * 
 * Notes:
 * - There's hard-coded 512 byte sectors everywhere, that needs to be
 *   cleaned.
 * - Thread safety is out the window with the write and LFN code
 */
/**
 * \todo Implement changing of the parent directory when a file is written to
 * \todo Implement file creation / deletion
 */
#define DEBUG	1
#define VERBOSE	1

#include <acess.h>
#include <modules.h>
#include "common.h"

// === PROTOTYPES ===
// --- Driver Core
 int	FAT_Install(char **Arguments);
tVFS_Node	*FAT_InitDevice(const char *device, const char **options);
void	FAT_Unmount(tVFS_Node *Node);
// --- Helpers
 int	FAT_int_GetAddress(tVFS_Node *Node, Uint64 Offset, Uint64 *Addr, Uint32 *Cluster);
// --- File IO
size_t	FAT_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
#if SUPPORT_WRITE
size_t	FAT_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
#endif
void	FAT_CloseFile(tVFS_Node *node);


// === Options ===
 int	giFAT_MaxCachedClusters = 1024*512/4;

// === SEMI-GLOBALS ===
MODULE_DEFINE(0, VER2(1,1) /*v1.01*/, VFAT, FAT_Install, NULL, NULL);
tFAT_VolInfo	gFAT_Disks[8];
 int	giFAT_PartCount = 0;
tVFS_Driver	gFAT_FSInfo = {"fat", 0, FAT_InitDevice, FAT_Unmount, FAT_GetNodeFromINode, NULL};
tVFS_NodeType	gFAT_DirType = {
	.TypeName = "FAT-Dir",
	.ReadDir = FAT_ReadDir,
	.FindDir = FAT_FindDir,
	#if SUPPORT_WRITE
	.MkNod = FAT_Mknod,
	.Link = FAT_Link,
	.Unlink = FAT_Unlink,
	#endif
	.Close = FAT_CloseFile
	};
tVFS_NodeType	gFAT_FileType = {
	.TypeName = "FAT-File",
	.Read = FAT_Read,
	#if SUPPORT_WRITE
	.Write = FAT_Write,
	#endif
	.Close = FAT_CloseFile
	};

// === CODE ===
/**
 * \fn int FAT_Install(char **Arguments)
 * \brief Install the FAT Driver
 */
int FAT_Install(char **Arguments)
{
	VFS_AddDriver( &gFAT_FSInfo );
	return MODULE_ERR_OK;
}

/**
 * \brief Reads the boot sector of a disk and prepares the structures for it
 */
tVFS_Node *FAT_InitDevice(const char *Device, const char **Options)
{
	fat_bootsect *bs;
	 int	i;
	Uint32	FATSz, RootDirSectors, TotSec;
	tVFS_Node	*node = NULL;
	tFAT_VolInfo	*diskInfo = &gFAT_Disks[giFAT_PartCount];

	memset(diskInfo, 0, sizeof(*diskInfo));
	
	// Temporary Pointer
	bs = &diskInfo->bootsect;
	
	// Open device and read boot sector
	diskInfo->fileHandle = VFS_Open(Device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	if(diskInfo->fileHandle == -1) {
		Log_Notice("FAT", "Unable to open device '%s'", Device);
		return NULL;
	}
	
	VFS_ReadAt(diskInfo->fileHandle, 0, 512, bs);
	
	if(bs->bps == 0 || bs->spc == 0) {
		Log_Notice("FAT", "Error in FAT Boot Sector (zero BPS/SPC)");
		return NULL;
	}
	
	// FAT Type Determining
	// - From Microsoft FAT Specifcation
	RootDirSectors = ((bs->files_in_root*32) + (bs->bps - 1)) / bs->bps;
	
	if(bs->fatSz16 != 0)
		FATSz = bs->fatSz16;
	else
		FATSz = bs->spec.fat32.fatSz32;
	
	if(bs->totalSect16 != 0)
		TotSec = bs->totalSect16;
	else
		TotSec = bs->totalSect32;
	
	diskInfo->ClusterCount = (TotSec - (bs->resvSectCount + (bs->fatCount * FATSz) + RootDirSectors)) / bs->spc;
	
	if(diskInfo->ClusterCount < FAT16_MIN_SECTORS)
		diskInfo->type = FAT12;
	else if(diskInfo->ClusterCount < FAT32_MIN_CLUSTERS)
		diskInfo->type = FAT16;
	else
		diskInfo->type = FAT32;
	
	#if VERBOSE
	{
		char	*sFatType, *sSize;
		Uint	iSize = diskInfo->ClusterCount * bs->spc * bs->bps / 1024;
		
		switch(diskInfo->type)
		{
		case FAT12:	sFatType = "FAT12";	break;
		case FAT16:	sFatType = "FAT16";	break;
		case FAT32:	sFatType = "FAT32";	break;
		default:	sFatType = "UNKNOWN";	break;
		}
		if(iSize <= 2*1024) {
			sSize = "KiB";
		}
		else if(iSize <= 2*1024*1024) {
			sSize = "MiB";
			iSize >>= 10;
		}
		else {
			sSize = "GiB";
			iSize >>= 20;
		}
		Log_Notice("FAT", "'%s' %s, %i %s", Device, sFatType, iSize, sSize);
	}
	#endif
	
	// Get Name
	if(diskInfo->type == FAT32) {
		for(i=0;i<11;i++)
			diskInfo->name[i] = (bs->spec.fat32.label[i] == ' ' ? '\0' : bs->spec.fat32.label[i]);
	}
	else {
		for(i=0;i<11;i++)
			diskInfo->name[i] = (bs->spec.fat16.label[i] == ' ' ? '\0' : bs->spec.fat16.label[i]);
	}
	diskInfo->name[11] = '\0';
	
	// Compute Root directory offset
	if(diskInfo->type == FAT32)
		diskInfo->rootOffset = bs->spec.fat32.rootClust;
	else
		diskInfo->rootOffset = (FATSz * bs->fatCount) / bs->spc;
	
	diskInfo->firstDataSect = bs->resvSectCount + (bs->fatCount * FATSz) + RootDirSectors;
	
	//Allow for Caching the FAT
	#if CACHE_FAT
	if( diskInfo->ClusterCount <= giFAT_MaxCachedClusters )
	{
		Uint32	Ofs;
		diskInfo->FATCache = (Uint32*)malloc(sizeof(Uint32)*diskInfo->ClusterCount);
		if(diskInfo->FATCache == NULL) {
			Log_Warning("FAT", "Heap Exhausted");
			return NULL;
		}
		Ofs = bs->resvSectCount*512;
		if(diskInfo->type == FAT12)
		{
			Uint32	val;
			 int	j;
			char	buf[1536];
			for(i = 0; i < diskInfo->ClusterCount/2; i++) {
				j = i & 511;	//%512
				if( j == 0 ) {
					VFS_ReadAt(diskInfo->fileHandle, Ofs, 3*512, buf);
					Ofs += 3*512;
				}
				val = *((int*)(buf+j*3));
				diskInfo->FATCache[i*2] = val & 0xFFF;
				diskInfo->FATCache[i*2+1] = (val>>12) & 0xFFF;
			}
		}
		else if(diskInfo->type == FAT16)
		{
			Uint16	buf[256];
			for(i=0;i<diskInfo->ClusterCount;i++) {
				if( (i & 255) == 0 ) {
					VFS_ReadAt(diskInfo->fileHandle, Ofs, 512, buf);
					Ofs += 512;
				}
				diskInfo->FATCache[i] = buf[i&255];
			}
		}
		else if(diskInfo->type == FAT32)
		{
			Uint32	buf[128];
			for(i=0;i<diskInfo->ClusterCount;i++) {
				if( (i & 127) == 0 ) {
					VFS_ReadAt(diskInfo->fileHandle, Ofs, 512, buf);
					Ofs += 512;
				}
				diskInfo->FATCache[i] = buf[i&127];
			}
		}
		LOG("FAT Fully Cached");
	}
	#endif /*CACHE_FAT*/
	
	diskInfo->BytesPerCluster = bs->spc * bs->bps;
	
	// == VFS Interface
	node = &diskInfo->rootNode;
	//node->Size = bs->files_in_root;
	node->Size = -1;
	node->Inode = diskInfo->rootOffset;	// 0:31 - Cluster, 32:63 - Parent Directory Cluster
	node->ImplPtr = diskInfo;	// Disk info pointer
	node->ImplInt = 0;	// 0:15 - Directory Index, 16: Dirty Flag, 17: Deletion Flag
	
	node->ReferenceCount = 1;
	
	node->UID = 0;	node->GID = 0;
	node->NumACLs = 1;
	node->ACLs = &gVFS_ACL_EveryoneRWX;
	node->Flags = VFS_FFLAG_DIRECTORY;
	node->CTime = node->MTime = node->ATime = now();

	node->Type = &gFAT_DirType;	
	
	giFAT_PartCount ++;
	return node;
}

/**
 * \brief Closes a mount and marks it as free
 * \param Node	Mount Root
 * 
 * \todo Remove FAT Cache
 * \todo Clear LFN Cache
 * \todo Check that all files are closed and flushed
 */
void FAT_Unmount(tVFS_Node *Node)
{
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	// Close Disk Handle
	VFS_Close( disk->fileHandle );
	// Clear Node Cache
	FAT_int_ClearNodeCache(disk);
	// Mark as unused
	disk->fileHandle = -2;
	return;
}

/**
 * \brief Converts an offset in a file into a disk address
 * \param Node	File (or directory) node
 * \param Offset	Offset in the file
 * \param Addr	Return Address
 * \param Cluster	Set to the current cluster (or the last one if \a Offset
 *                  is past EOC) - Not touched if the node is the root
 *                  directory.
 * \return Zero on success, non-zero on error
 */
int FAT_int_GetAddress(tVFS_Node *Node, Uint64 Offset, Uint64 *Addr, Uint32 *Cluster)
{
	Uint32	cluster, base_cluster;
	Uint64	addr;
	 int	skip;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode XOffset", Node, Offset);
	
	cluster = base_cluster = Node->Inode & 0xFFFFFFF;	// Cluster ID
	LOG("base cluster = 0x%07x", cluster);
	
	// Do Cluster Skip
	// - Pre FAT32 had a reserved area for the root.
	if( disk->type == FAT32 || cluster != disk->rootOffset )
	{
		skip = Offset / disk->BytesPerCluster;
		LOG("skip = %i", skip);
		// Skip previous clusters
		for(; skip-- ; )
		{
			if(Cluster)	*Cluster = cluster;
			cluster = FAT_int_GetFatValue(disk, cluster);
			// Check for end of cluster chain
			if(cluster == 0xFFFFFFFF) {	LEAVE('i', 1);	return 1;}
		}
		if(Cluster)	*Cluster = cluster;
	}
	else {
		// TODO: Bounds checking on root
		LOG("Root cluster count %i", disk->bootsect.files_in_root*32/disk->BytesPerCluster);
		// Increment by clusters in offset
		cluster += Offset / disk->BytesPerCluster;
	}
	
	LOG("cluster = 0x%07x", cluster);
	
	// Bounds Checking (Used to spot corruption)
	if(cluster > disk->ClusterCount + 2)
	{
		Log_Warning("FAT", "Cluster ID is over cluster count (0x%x>0x%x)",
			cluster, disk->ClusterCount+2);
		LEAVE('i', 1);
		return 1;
	}
	
	// Compute Offsets
	// - Pre FAT32 cluster base (in sectors)
	if( base_cluster == disk->rootOffset && disk->type != FAT32 ) {
		addr = disk->bootsect.resvSectCount * disk->bootsect.bps;
		addr += cluster * disk->BytesPerCluster;
	}
	else {
		addr = disk->firstDataSect * disk->bootsect.bps;
		addr += (cluster - 2) * disk->BytesPerCluster;
	}
	// In-cluster offset
	addr += Offset % disk->BytesPerCluster;
	
	LOG("addr = 0x%08x", addr);
	*Addr = addr;
	LEAVE('i', 0);
	return 0;
}

/* ====================
 *       File IO
 * ====================
 */
/**
 * \brief Reads data from a specified file
 */
size_t FAT_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	 int	preSkip, count;
	Uint64	final_bytes;
	 int	i, cluster, pos;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	char	tmpBuf[disk->BytesPerCluster];
	 int	bpc = disk->BytesPerCluster;
	
	ENTER("pNode Xoffset xlength pbuffer", Node, Offset, Length, Buffer);
	
	// Sanity Check offset
	if(Offset > Node->Size) {
		LOG("Seek past EOF (%i > %i)", Offset, Node->Size);
		LEAVE('i', 0);
		return 0;
	}
	
	// Cluster is stored in the low 32-bits of the Inode field
	cluster = Node->Inode & 0xFFFFFFFF;
	
	// Clamp Size
	if(Offset + Length > Node->Size) {
		LOG("Reading past EOF (%lli + %lli > %lli), clamped to %lli",
			Offset, Length, Node->Size, Node->Size - Offset);
		Length = Node->Size - Offset;
	}
	
	// Skip previous clusters
	preSkip = Offset / bpc;
	Offset %= bpc;
	LOG("preSkip = %i, Offset = %i", preSkip, (int)Offset);
	for(i = preSkip; i--; )
	{
		cluster = FAT_int_GetFatValue(disk, cluster);
		if(cluster == -1) {
			Log_Warning("FAT", "Offset is past end of cluster chain mark");
			LEAVE('i', 0);
			return 0;
		}
	}

	// Reading from within one cluster
	if((int)Offset + (int)Length <= bpc)
	{
		LOG("single cluster only");
		FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
		memcpy( Buffer, (void*)( tmpBuf + Offset%bpc ), Length );
		LEAVE('X', Length);
		return Length;
	}
	
	// Align read to a cluster
	if( Offset > 0 )
	{
		pos = bpc - Offset;
		FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
		memcpy( Buffer, (void*)( tmpBuf + Offset ), pos );
		LOG("pos = %i, Reading the rest of the clusters");
		// Get next cluster in the chain
		cluster = FAT_int_GetFatValue(disk, cluster);
		if(cluster == -1) {
			Log_Warning("FAT", "Read past End of Cluster Chain (Align)");
			LEAVE('X', pos);
			return pos;
		}
	}
	else
		pos = 0;

	// Get Count of Clusters to read
//	count = DivMod64U(Length - pos, bpc, &final_bytes);
	count = (Length - pos) / bpc;
	final_bytes = (Length - pos) % bpc;
	LOG("Offset = %i, Length = %i, count = %i, final_bytes = %i", (int)Offset, (int)Length, count, final_bytes);
	
	// Read the rest of the cluster data
	for( ; count; count -- )
	{
		if(cluster == -1) {
			Log_Warning("FAT", "Read past End of Cluster Chain (Bulk)");
			LEAVE('X', pos);
			return pos;
		}
		// Read cluster
		FAT_int_ReadCluster(disk, cluster, bpc, (void*)(Buffer+pos));
		pos += bpc;
		// Get next cluster in the chain
		cluster = FAT_int_GetFatValue(disk, cluster);
	}

	if( final_bytes > 0 )
	{
		if(cluster == -1) {
			Log_Warning("FAT", "Read past End of Cluster Chain (Final)");
			LEAVE('X', pos);
			return pos;
		}
		// Read final cluster
		FAT_int_ReadCluster( disk, cluster, bpc, tmpBuf );
		memcpy( (void*)(Buffer+pos), tmpBuf, Length-pos );
	}
		
	#if DEBUG
	//Debug_HexDump("FAT_Read", Buffer, Length);
	#endif
	
	LEAVE('X', Length);
	return Length;
}

#if SUPPORT_WRITE
/**
 * \brief Write to a file
 * \param Node	File Node
 * \param Offset	Offset within file
 * \param Length	Size of data to write
 * \param Buffer	Data source
 */
size_t FAT_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	tFAT_VolInfo	*disk = Node->ImplPtr;
	char	tmpBuf[disk->BytesPerCluster];
	 int	remLength = Length;
	Uint32	cluster, tmpCluster;
	 int	bNewCluster = 0;
	
	if(Offset > Node->Size)	return 0;
	
	// Seek Clusters
	cluster = Node->Inode & 0xFFFFFFFF;
	while( Offset > disk->BytesPerCluster )
	{
		cluster = FAT_int_GetFatValue( disk, cluster );
		if(cluster == -1) {
			Log_Warning("FAT", "EOC Unexpectedly Reached");
			return 0;
		}
		Offset -= disk->BytesPerCluster;
	}
	if( Offset == disk->BytesPerCluster )
	{
		Uint32	tmp = FAT_int_AllocateCluster(disk, cluster);
		if(!tmp)	return 0;
		cluster = tmp;
		Offset -= disk->BytesPerCluster;
	}
	
	if( Offset + Length < disk->BytesPerCluster )
	{
		char	tmpBuf[disk->BytesPerCluster];
		
		// Read-Modify-Write
		FAT_int_ReadCluster( disk, cluster, disk->BytesPerCluster, tmpBuf );
		memcpy(	tmpBuf + Offset, Buffer, Length );
		FAT_int_WriteCluster( disk, cluster, tmpBuf );
		
		return Length;
	}
	
	// Clean up changes within a cluster
	if( Offset )
	{	
		// Read-Modify-Write
		FAT_int_ReadCluster( disk, cluster, disk->BytesPerCluster, tmpBuf );
		memcpy(	tmpBuf + Offset, Buffer, disk->BytesPerCluster - Offset );
		FAT_int_WriteCluster( disk, cluster, tmpBuf );
		
		remLength -= disk->BytesPerCluster - Offset;
		Buffer += disk->BytesPerCluster - Offset;
		
		// Get next cluster (allocating if needed)
		tmpCluster = FAT_int_GetFatValue(disk, cluster);
		if(tmpCluster == -1) {
			tmpCluster = FAT_int_AllocateCluster(disk, cluster);
			if( tmpCluster == 0 ) {
				return Length - remLength;
			}
		}
		cluster = tmpCluster;
	}
	
	while( remLength > disk->BytesPerCluster )
	{
		FAT_int_WriteCluster( disk, cluster, Buffer );
		Buffer += disk->BytesPerCluster;
		
		// Get next cluster (allocating if needed)
		tmpCluster = FAT_int_GetFatValue(disk, cluster);
		if(tmpCluster == -1) {
			bNewCluster = 1;
			tmpCluster = FAT_int_AllocateCluster(disk, cluster);
			if( tmpCluster == 0 ) {
				return Length - remLength;
			}
		}
		cluster = tmpCluster;
	}
	
	// Finish off
	if( bNewCluster )
		memset(tmpBuf, 0, disk->BytesPerCluster);
	else
		FAT_int_ReadCluster( disk, cluster, disk->BytesPerCluster, tmpBuf );
	memcpy(	tmpBuf, Buffer, remLength );
	FAT_int_WriteCluster( disk, cluster, tmpBuf );
	free( tmpBuf );
	
	return Length;
}
#endif

/**
 * \fn void FAT_CloseFile(tVFS_Node *Node)
 * \brief Close an open file
 */
void FAT_CloseFile(tVFS_Node *Node)
{
	tFAT_VolInfo	*disk = Node->ImplPtr;
	if(Node == NULL)	return ;
	
	#if SUPPORT_WRITE
	// Update the node if it's dirty (don't bother if it's marked for
	// deletion)
	if( (Node->ImplInt & FAT_FLAG_DIRTY) && !(Node->ImplInt & FAT_FLAG_DELETE) )
	{
		fat_filetable	ft;
		tVFS_Node	*dirnode;

		dirnode = FAT_int_CreateIncompleteDirNode(disk, Node->Inode >> 32);
		if( !dirnode ) {
			Log_Error("FAT", "Can't get node for directory cluster #0x%x", Node->Inode>>32);
			return ;
		}

		int id = FAT_int_GetEntryByCluster(dirnode, Node->Inode & 0xFFFFFFFF, &ft);
		ft.size = Node->Size;
		// TODO: update adate, mtime, mdate
		FAT_int_WriteDirEntry(dirnode, id, &ft);
		
		dirnode->Type->Close(dirnode);
		
		Node->ImplInt &= ~FAT_FLAG_DIRTY;
	}
	#endif
	
	// TODO: Make this more thread safe somehow, probably by moving the
	// Inode_UncacheNode higher up and saving the cluster value somewhere
	if( Node->ReferenceCount == 1 )
	{		
		#if SUPPORT_WRITE
		// Delete File
		if( Node->ImplInt & FAT_FLAG_DELETE ) {
			// Since the node is marked, we only need to remove it's data
			Uint32	cluster = Node->Inode & 0xFFFFFFFF;
			while( cluster != -1 )
				cluster = FAT_int_FreeCluster(Node->ImplPtr, cluster);
		}
		#endif
	}
	
	FAT_int_DerefNode(Node);
	return ;
}

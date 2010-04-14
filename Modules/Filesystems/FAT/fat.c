/*
 * Acess 2
 * FAT12/16/32 Driver Version (Incl LFN)
 * 
 * NOTE: This driver will only support _reading_ long file names, not
 * writing. I don't even know why i'm adding write-support. FAT sucks.
 * 
 * Known Bugs:
 * - LFN Is buggy in FAT_ReadDir
 */
/**
 * \todo Implement changing of the parent directory when a file is written to
 * \todo Implement file creation / deletion
 */
#define DEBUG	1
#define VERBOSE	1

#define CACHE_FAT	1	//!< Caches the FAT in memory
#define USE_LFN		0	//!< Enables the use of Long File Names
#define	SUPPORT_WRITE	0

#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include "fs_fat.h"

#define FAT_FLAG_DIRTY	0x10000
#define FAT_FLAG_DELETE	0x20000

// === TYPES ===
#if USE_LFN
typedef struct s_lfncache
{
	Uint	Inode;
	tFAT_VolInfo	*Disk;
	 int	id;
	char	Name[256];
	struct s_lfncache	*Next;
}	t_lfncache;
#endif

// === PROTOTYPES ===
 int	FAT_Install(char **Arguments);
tVFS_Node	*FAT_InitDevice(char *device, char **options);
void	FAT_Unmount(tVFS_Node *Node);

Uint32	FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 Cluster);
Uint32	FAT_int_AllocateCluster(tFAT_VolInfo *Disk, Uint32 Previous);

void	FAT_int_ReadCluster(tFAT_VolInfo *Disk, Uint32 Cluster, int Length, void *Buffer);
Uint64	FAT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
#if SUPPORT_WRITE
void	FAT_int_WriteCluster(tFAT_VolInfo *Disk, Uint32 Cluster, void *Buffer);
Uint64	FAT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
#endif

char	*FAT_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*FAT_FindDir(tVFS_Node *Node, char *Name);
 int	FAT_Mknod(tVFS_Node *Node, char *Name, Uint Flags);
 int	FAT_Relink(tVFS_Node *node, char *OldName, char *NewName);
void	FAT_CloseFile(tVFS_Node *node);

// === Options ===
 int	giFAT_MaxCachedClusters = 1024*512/4;

// === SEMI-GLOBALS ===
MODULE_DEFINE(0, (0<<8)|50 /*v0.50*/, VFAT, FAT_Install, NULL, NULL);
tFAT_VolInfo	gFAT_Disks[8];
 int	giFAT_PartCount = 0;
#if USE_LFN
t_lfncache	*fat_lfncache;
#endif
tVFS_Driver	gFAT_FSInfo = {"fat", 0, FAT_InitDevice, FAT_Unmount, NULL};

// === CODE ===
/**
 * \fn int FAT_Install(char **Arguments)
 * \brief 
 */
int FAT_Install(char **Arguments)
{
	VFS_AddDriver( &gFAT_FSInfo );
	return MODULE_ERR_OK;
}

/**
 * \fn tVFS_Node *FAT_InitDevice(char *Device, char **Options)
 * \brief Reads the boot sector of a disk and prepares the structures for it
 */
tVFS_Node *FAT_InitDevice(char *Device, char **Options)
{
	fat_bootsect *bs;
	 int	i;
	Uint32	FATSz, RootDirSectors, TotSec;
	tVFS_Node	*node = NULL;
	tFAT_VolInfo	*diskInfo = &gFAT_Disks[giFAT_PartCount];
	
	// Temporary Pointer
	bs = &diskInfo->bootsect;
	
	//Open device and read boot sector
	diskInfo->fileHandle = VFS_Open(Device, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	if(diskInfo->fileHandle == -1) {
		Log_Notice("FAT", "Unable to open device '%s'", Device);
		return NULL;
	}
	
	VFS_ReadAt(diskInfo->fileHandle, 0, 512, bs);
	
	if(bs->bps == 0 || bs->spc == 0) {
		Log_Notice("FAT", "Error in FAT Boot Sector");
		return NULL;
	}
	
	// FAT Type Determining
	// - From Microsoft FAT Specifcation
	RootDirSectors = ((bs->files_in_root*32) + (bs->bps - 1)) / bs->bps;
	
	if(bs->fatSz16 != 0)		FATSz = bs->fatSz16;
	else					FATSz = bs->spec.fat32.fatSz32;
	
	if(bs->totalSect16 != 0)		TotSec = bs->totalSect16;
	else						TotSec = bs->totalSect32;
	
	diskInfo->ClusterCount = (TotSec - (bs->resvSectCount + (bs->fatCount * FATSz) + RootDirSectors)) / bs->spc;
	
	if(diskInfo->ClusterCount < 4085)
		diskInfo->type = FAT12;
	else if(diskInfo->ClusterCount < 65525)
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
	
	// Initalise inode cache for filesystem
	diskInfo->inodeHandle = Inode_GetHandle();
	LOG("Inode Cache handle is %i", diskInfo->inodeHandle);
	
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
	
	node->Read = node->Write = NULL;
	node->ReadDir = FAT_ReadDir;
	node->FindDir = FAT_FindDir;
	#if SUPPORT_WRITE
	node->Relink = FAT_Relink;
	node->MkNod = FAT_Mknod;
	#else
	node->Relink = NULL;
	node->MkNod = NULL;
	#endif
	//node->Close = FAT_Unmount;
	
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
	Inode_ClearCache(disk->inodeHandle);
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
	Uint32	cluster;
	Uint64	addr;
	 int	skip;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode XOffset", Node, Offset);
	
	cluster = Node->Inode & 0xFFFFFFFF;	// Cluster ID
	LOG("cluster = %08x", cluster);
	
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
			if(cluster == -1) {	LEAVE('i', 1);	return 1;}
		}
		if(Cluster)	*Cluster = cluster;
	}
	
	LOG("cluster = %08x", cluster);
	
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
	if( cluster == disk->rootOffset && disk->type != FAT32 ) {
		addr = disk->bootsect.resvSectCount * disk->bootsect.bps;
		addr += cluster * disk->BytesPerCluster;
	}
	else {
		addr = disk->firstDataSect * disk->bootsect.bps;
		addr += (cluster - 2) * disk->BytesPerCluster;
	}
	addr += Offset % disk->BytesPerCluster;
	
	LOG("addr = 0x%08x", addr);
	*Addr = addr;
	LEAVE('i', 0);
	return 0;
}

/*
 * ====================
 *   FAT Manipulation
 * ====================
 */
/**
 * \fn Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
 * \brief Fetches a value from the FAT
 */
Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
{
	Uint32	val = 0;
	Uint32	ofs;
	ENTER("pDisk xCluster", Disk, cluster);
	LOCK( &Disk->lFAT );
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		val = Disk->FATCache[cluster];
		if(Disk->type == FAT12 && val == EOC_FAT12)	val = -1;
		if(Disk->type == FAT16 && val == EOC_FAT16)	val = -1;
		if(Disk->type == FAT32 && val == EOC_FAT32)	val = -1;
	}
	else
	{
	#endif
		ofs = Disk->bootsect.resvSectCount*512;
		if(Disk->type == FAT12) {
			VFS_ReadAt(Disk->fileHandle, ofs+(cluster>>1)*3, 3, &val);
			val = (cluster&1 ? val&0xFFF : val>>12);
			if(val == EOC_FAT12)	val = -1;
		} else if(Disk->type == FAT16) {
			VFS_ReadAt(Disk->fileHandle, ofs+cluster*2, 2, &val);
			if(val == EOC_FAT16)	val = -1;
		} else {
			VFS_ReadAt(Disk->fileHandle, ofs+cluster*4, 4, &val);
			if(val == EOC_FAT32)	val = -1;
		}
	#if CACHE_FAT
	}
	#endif /*CACHE_FAT*/
	RELEASE( &Disk->lFAT );
	LEAVE('x', val);
	return val;
}

#if SUPPORT_WRITE
/**
 * \brief Allocate a new cluster
 */
Uint32 FAT_int_AllocateCluster(tFAT_VolInfo *Disk, Uint32 Previous)
{
	Uint32	ret = Previous;
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		Uint32	eoc;
		
		LOCK(Disk->lFAT);
		for(ret = Previous; ret < Disk->ClusterCount; ret++)
		{
			if(Disk->FATCache[ret] == 0)
				goto append;
		}
		for(ret = 0; ret < Previous; ret++)
		{
			if(Disk->FATCache[ret] == 0)
				goto append;
		}
		
		RELEASE(Disk->lFAT);
		return 0;
	
	append:
		switch(Disk->type)
		{
		case FAT12:	eoc = EOC_FAT12;	break;
		case FAT16:	eoc = EOC_FAT16;	break;
		case FAT32:	eoc = EOC_FAT32;	break;
		default:	return 0;
		}
		
		Disk->FATCache[ret] = eoc;
		Disk->FATCache[Previous] = ret;
		
		RELEASE(Disk->lFAT);
		return ret;
	}
	else
	{
	#endif
		Uint32	val;
		Uint32	ofs = Disk->bootsect.resvSectCount*512;
		Log_Warning("FAT", "TODO: Implement cluster allocation with non cached FAT");
		return 0;
		
		switch(Disk->type)
		{
		case FAT12:
			VFS_ReadAt(Disk->fileHandle, ofs+(Previous>>1)*3, 3, &val);
			if( Previous & 1 ) {
				val &= 0xFFF000;
				val |= ret;
			}
			else {
				val &= 0xFFF;
				val |= ret<<12;
			}
			VFS_WriteAt(Disk->fileHandle, ofs+(Previous>>1)*3, 3, &val);
			
			VFS_ReadAt(Disk->fileHandle, ofs+(ret>>1)*3, 3, &val);
			if( Cluster & 1 ) {
				val &= 0xFFF000;
				val |= eoc;
			}
			else {
				val &= 0x000FFF;
				val |= eoc<<12;
			}
			VFS_WriteAt(Disk->fileHandle, ofs+(ret>>1)*3, 3, &val);
			break;
		case FAT16:
			VFS_ReadAt(Disk->fileHandle, ofs+Previous*2, 2, &ret);
			VFS_WriteAt(Disk->fileHandle, ofs+ret*2, 2, &eoc);
			break;
		case FAT32:
			VFS_ReadAt(Disk->fileHandle, ofs+Previous*4, 4, &ret);
			VFS_WriteAt(Disk->fileHandle, ofs+ret*4, 4, &eoc);
			break;
		}
		return ret;
	#if CACHE_FAT
	}
	#endif
}

/**
 * \brief Free's a cluster
 * \return The original contents of the cluster
 */
Uint32 FAT_int_FreeCluster(tFAT_VolInfo *Disk, Uint32 Cluster)
{
	Uint32	ret;
	#if CACHE_FAT
	if( Disk->ClusterCount <= giFAT_MaxCachedClusters )
	{
		LOCK(Disk->lFAT);
		
		ret = Disk->FATCache[Cluster];
		Disk->FATCache[Cluster] = 0;
		
		RELEASE(Disk->lFAT);
	}
	else
	{
	#endif
		Uint32	val;
		Uint32	ofs = Disk->bootsect.resvSectCount*512;
		LOCK(Disk->lFAT);
		switch(Disk->type)
		{
		case FAT12:
			VFS_ReadAt(Disk->fileHandle, ofs+(Cluster>>1)*3, 3, &val);
			if( Cluster & 1 ) {
				ret = val & 0xFFF0000;
				val &= 0xFFF;
			}
			else {
				ret = val & 0xFFF;
				val &= 0xFFF000;
			}
			VFS_WriteAt(Disk->fileHandle, ofs+(Previous>>1)*3, 3, &val);
			break;
		case FAT16:
			VFS_ReadAt(Disk->fileHandle, ofs+Previous*2, 2, &ret);
			val = 0;
			VFS_WriteAt(Disk->fileHandle, ofs+Cluster*2, 2, &val);
			break;
		case FAT32:
			VFS_ReadAt(Disk->fileHandle, ofs+Previous*4, 4, &ret);
			val = 0;
			VFS_WriteAt(Disk->fileHandle, ofs+Cluster*2, 2, &val);
			break;
		}
		RELEASE(Disk->lFAT);
	#if CACHE_FAT
	}
	#endif
	if(Disk->type == FAT12 && ret == EOC_FAT12)	ret = -1;
	if(Disk->type == FAT16 && ret == EOC_FAT16)	ret = -1;
	if(Disk->type == FAT32 && ret == EOC_FAT32)	ret = -1;
	return ret;
}
#endif

/*
 * ====================
 *      Cluster IO
 * ====================
 */
/**
 * \brief Read a cluster
 */
void FAT_int_ReadCluster(tFAT_VolInfo *Disk, Uint32 Cluster, int Length, void *Buffer)
{
	ENTER("pDisk xCluster iLength pBuffer", Disk, Cluster, Length, Buffer);
	//Log("Cluster = %i (0x%x)", Cluster, Cluster);
	VFS_ReadAt(
		Disk->fileHandle,
		(Disk->firstDataSect + (Cluster-2)*Disk->bootsect.spc )
			* Disk->bootsect.bps,
		Length,
		Buffer
		);
	LEAVE('-');
}

/* ====================
 *       File IO
 * ====================
 */
/**
 * \fn Uint64 FAT_Read(tVFS_Node *node, Uint64 offset, Uint64 length, void *buffer)
 * \brief Reads data from a specified file
 */
Uint64 FAT_Read(tVFS_Node *Node, Uint64 offset, Uint64 length, void *buffer)
{
	 int	preSkip, count;
	 int	i, cluster, pos;
	 int	bpc;
	void	*tmpBuf;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode Xoffset Xlength pbuffer", Node, offset, length, buffer);
	
	// Calculate and Allocate Bytes Per Cluster
	bpc = disk->BytesPerCluster;
	tmpBuf = (void*) malloc(bpc);
	if( !tmpBuf ) 	return 0;
	
	// Cluster is stored in Inode Field
	cluster = Node->Inode & 0xFFFFFFFF;
	
	// Sanity Check offset
	if(offset > Node->Size) {
		LOG("Reading past EOF (%i > %i)", offset, Node->Size);
		LEAVE('i', 0);
		return 0;
	}
	// Clamp Size
	if(offset + length > Node->Size) {
		LOG("Reading past EOF (%lli + %lli > %lli), clamped to %lli",
			offset, length, Node->Size, Node->Size - offset);
		length = Node->Size - offset;
	}
	
	// Single Cluster including offset
	if(length + offset < bpc)
	{
		FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
		memcpy( buffer, (void*)( tmpBuf + offset%bpc ), length );
		free(tmpBuf);
		LEAVE('i', 1);
		return length;
	}
	
	#if 0
	if( FAT_int_GetAddress(Node, offset, &addr) )
	{
		Log_Warning("FAT", "Offset is past end of cluster chain mark");
		LEAVE('i', 0);
		return 0;
	}
	#endif
	
	preSkip = offset / bpc;
	
	//Skip previous clusters
	for(i=preSkip;i--;)	{
		cluster = FAT_int_GetFatValue(disk, cluster);
		if(cluster == -1) {
			Log_Warning("FAT", "Offset is past end of cluster chain mark");
			LEAVE('i', 0);
			return 0;
		}
	}
	
	// Get Count of Clusters to read
	count = ((offset%bpc+length) / bpc) + 1;
	
	// Get buffer Position after 1st cluster
	pos = bpc - offset%bpc;
	
	// Read 1st Cluster
	FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
	memcpy(
		buffer,
		(void*)( tmpBuf + (bpc-pos) ),
		(pos < length ? pos : length)
		);
	
	if (count == 1) {
		free(tmpBuf);
		LEAVE('i', 1);
		return length;
	}
	
	cluster = FAT_int_GetFatValue(disk, cluster);
	
	#if DEBUG
	LOG("pos = %i", pos);
	LOG("Reading the rest of the clusters");
	#endif
	
	
	// Read the rest of the cluster data
	for( i = 1; i < count-1; i++ )
	{
		FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
		memcpy((void*)(buffer+pos), tmpBuf, bpc);
		pos += bpc;
		cluster = FAT_int_GetFatValue(disk, cluster);
		if(cluster == -1) {
			Warning("FAT_Read - Read past End of Cluster Chain");
			free(tmpBuf);
			LEAVE('i', 0);
			return 0;
		}
	}
	
	FAT_int_ReadCluster(disk, cluster, bpc, tmpBuf);
	memcpy((void*)(buffer+pos), tmpBuf, length-pos);
	
	#if DEBUG
	LOG("Free tmpBuf(0x%x) and Return", tmpBuf);
	#endif
	
	free(tmpBuf);
	LEAVE('X', length);
	return length;
}

#if SUPPORT_WRITE
/**
 * \brief Write a cluster to disk
 */
void FAT_int_WriteCluster(tFAT_VolInfo *Disk, Uint32 Cluster, void *Buffer)
{
	ENTER("pDisk xCluster pBuffer", Disk, Cluster, Buffer);
	VFS_ReadAt(
		Disk->fileHandle,
		(Disk->firstDataSect + (Cluster-2)*Disk->bootsect.spc )
			* Disk->bootsect.bps,
		Disk->BytesPerCluster,
		Buffer
		);
	LEAVE('-');
}

/**
 * \brief Write to a file
 * \param Node	File Node
 * \param Offset	Offset within file
 * \param Length	Size of data to write
 * \param Buffer	Data source
 */
Uint64 FAT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tFAT_VolInfo	*disk = Node->ImplPtr;
	void	*tmpBuf;
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
		tmpBuf = malloc( disk->BytesPerCluster );
		
		// Read-Modify-Write
		FAT_int_ReadCluster( disk, cluster, disk->BytesPerCluster, tmpBuf );
		memcpy(	tmpBuf + Offset, Buffer, Length );
		FAT_int_WriteCluster( disk, cluster, tmpBuf );
		
		free(tmpBuf);
		return Length;
	}
	
	// Clean up changes within a cluster
	if( Offset )
	{
		tmpBuf = malloc( disk->BytesPerCluster );
		
		// Read-Modify-Write
		FAT_int_ReadCluster( disk, cluster, disk->BytesPerCluster, tmpBuf );
		memcpy(	tmpBuf + Offset, Buffer, disk->BytesPerCluster - Offset );
		FAT_int_WriteCluster( disk, cluster, tmpBuf );
		
		free(tmpBuf);
		
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
	tmpBuf = malloc( disk->BytesPerCluster );
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

/* ====================
 *  File Names & Nodes
 * ====================
 */
/**
 * \fn void FAT_int_ProperFilename(char *dest, char *src)
 * \brief Converts a FAT directory entry name into a proper filename
 */
void FAT_int_ProperFilename(char *dest, char *src)
{
	 int	a, b;
	
	for( a = 0; a < 8; a++) {
		if(src[a] == ' ')	break;
		dest[a] = src[a];
	}
	b = a;
	a = 8;
	if(src[8] != ' ')
		dest[b++] = '.';
	for( ; a < 11; a++, b++)	{
		if(src[a] == ' ')	break;
		dest[b] = src[a];
	}
	dest[b] = '\0';
	#if DEBUG
	//LOG("dest='%s'", dest);
	#endif
}

/**
 * \fn char *FAT_int_CreateName(fat_filetable *ft, char *LongFileName)
 * \brief Converts either a LFN or a 8.3 Name into a proper name
 */
char *FAT_int_CreateName(fat_filetable *ft, char *LongFileName)
{
	char	*ret;
	ENTER("pft sLongFileName", ft, LongFileName);
	#if USE_LFN
	if(LongFileName && LongFileName[0] != '\0')
	{	
		ret = strdup(LongFileName);
	}
	else
	{
	#endif
		ret = (char*) malloc(13);
		memset(ret, 13, '\0');
		FAT_int_ProperFilename(ret, ft->name);
	#if USE_LFN
	}
	#endif
	LEAVE('s', ret);
	return ret;
}

/**
 * \fn tVFS_Node *FAT_int_CreateNode(tVFS_Node *parent, fat_filetable *ft)
 * \brief Creates a tVFS_Node structure for a given file entry
 */
tVFS_Node *FAT_int_CreateNode(tVFS_Node *Parent, fat_filetable *Entry, int Pos)
{
	tVFS_Node	node = {0};
	tVFS_Node	*ret;
	tFAT_VolInfo	*disk = Parent->ImplPtr;
	
	ENTER("pParent pFT", Parent, Entry);
	
	// Set Other Data
	node.Inode = Entry->cluster | (Entry->clusterHi<<16) | (Parent->Inode << 32);
	LOG("node.Inode = %llx", node.Inode);
	node.ImplInt = Pos & 0xFFFF;
	node.ImplPtr = disk;
	node.Size = Entry->size;
	LOG("Entry->size = %i", Entry->size);
	node.UID = 0;	node.GID = 0;
	node.NumACLs = 1;
	
	node.Flags = 0;
	if(Entry->attrib & ATTR_DIRECTORY)	node.Flags |= VFS_FFLAG_DIRECTORY;
	if(Entry->attrib & ATTR_READONLY) {
		node.Flags |= VFS_FFLAG_READONLY;
		node.ACLs = &gVFS_ACL_EveryoneRX;	// R-XR-XR-X
	}
	else {
		node.ACLs = &gVFS_ACL_EveryoneRWX;	// RWXRWXRWX
	}
	
	node.ATime = timestamp(0,0,0,
			((Entry->adate&0x1F) - 1),	// Days
			((Entry->adate&0x1E0) - 1),	// Months
			1980+((Entry->adate&0xFF00)>>8)	// Years
			);
	
	node.CTime = Entry->ctimems * 10;	// Miliseconds
	node.CTime += timestamp(
			((Entry->ctime&0x1F)<<1),	// Seconds
			((Entry->ctime&0x3F0)>>5),	// Minutes
			((Entry->ctime&0xF800)>>11),	// Hours
			((Entry->cdate&0x1F)-1),		// Days
			((Entry->cdate&0x1E0)-1),		// Months
			1980+((Entry->cdate&0xFF00)>>8)	// Years
			);
			
	node.MTime = timestamp(
			((Entry->mtime&0x1F)<<1),	// Seconds
			((Entry->mtime&0x3F0)>>5),	// Minutes
			((Entry->mtime&0xF800)>>11),	// Hours
			((Entry->mdate&0x1F)-1),		// Days
			((Entry->mdate&0x1E0)-1),		// Months
			1980+((Entry->mdate&0xFF00)>>8)	// Years
			);
	
	if(node.Flags & VFS_FFLAG_DIRECTORY) {
		//Log_Debug("FAT", "Directory %08x has size 0x%x", node.Inode, node.Size);
		node.ReadDir = FAT_ReadDir;
		node.FindDir = FAT_FindDir;
		#if SUPPORT_WRITE
		node.MkNod = FAT_Mknod;
		node.Relink = FAT_Relink;
		#endif
		node.Size = -1;
	} else {
		node.Read = FAT_Read;
		#if SUPPORT_WRITE
		node.Write = FAT_Write;
		#endif
	}
	node.Close = FAT_CloseFile;
	
	ret = Inode_CacheNode(disk->inodeHandle, &node);
	LEAVE('p', ret);
	return ret;
}

/* ====================
 *     Directory IO
 * ====================
 */

/**
 * \brief Reads a sector from the disk
 */
int FAT_int_ReadDirSector(tVFS_Node *Node, int Sector, fat_filetable *Buffer)
{
	Uint64	addr;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode iSector pEntry", Node, Sector, Buffer);
	
	if(FAT_int_GetAddress(Node, Sector * 512, &addr, NULL))
	{
		LEAVE('i', 1);
		return 1;
	}
	
	LOG("addr = 0x%llx", addr);
	// Read Sector
	if(VFS_ReadAt(disk->fileHandle, addr, 512, Buffer) != 512)
	{
		LEAVE('i', 1);
		return 1;
	}
	
	LEAVE('i', 0);
	return 0;
}

#if SUPPORT_WRITE
/**
 * \brief Writes an entry to the disk
 * \todo Support expanding a directory
 * \return Zero on success, non-zero on error
 */
int FAT_int_WriteDirEntry(tVFS_Node *Node, int ID, fat_filetable *Entry)
{
	Uint64	addr = 0;
	 int	tmp;
	Uint32	cluster = 0;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode iID pEntry", Node, ID, Entry);
	
	tmp = FAT_int_GetAddress(Node, ID * sizeof(fat_filetable), &addr, &cluster);
	if( tmp )
	{
		//TODO: Allocate a cluster
		cluster = FAT_int_AllocateCluster(Node->ImplPtr, cluster);
		if(cluster == -1) {
			Log_Warning("FAT", "Unable to allocate an other cluster for %p", Node);
			LEAVE('i', 1);
			return 1;
		}
		FAT_int_GetAddress(Node, ID * sizeof(fat_filetable), &addr, &cluster);
	}
	

	LOG("addr = 0x%llx", addr);
	
	// Read Sector
	VFS_WriteAt(disk->fileHandle, addr, sizeof(fat_filetable), Entry);	// Read Dir Data
	
	LEAVE('i', 0);
	return 0;
}
#endif

#if USE_LFN
// I should probably more tightly associate the LFN cache with the node
// somehow, maybe by adding a field to tVFS_Node before locking it
// Maybe .Cache or something like that (something that is free'd by the
// Inode_UncacheNode function)
	
/**
 * \fn char *FAT_int_GetLFN(tVFS_Node *node)
 * \brief Return pointer to LFN cache entry
 */
char *FAT_int_GetLFN(tVFS_Node *node)
{
	t_lfncache	*tmp;
	tmp = fat_lfncache;
	while(tmp)
	{
		if(tmp->Inode == node->Inode && tmp->Disk == node->ImplPtr)
			return tmp->Name;
		tmp = tmp->Next;
	}
	tmp = malloc(sizeof(t_lfncache));
	tmp->Inode = node->Inode;
	tmp->Disk = node->ImplPtr;
	memset(tmp->Name, 0, 256);
	
	tmp->Next = fat_lfncache;
	fat_lfncache = tmp;
	
	return tmp->Name;
}

/**
 * \fn void FAT_int_DelLFN(tVFS_Node *node)
 * \brief Delete a LFN cache entry
 */
void FAT_int_DelLFN(tVFS_Node *node)
{
	t_lfncache	*tmp;
	
	if(!fat_lfncache)	return;
	
	if(!fat_lfncache->Next)
	{
		tmp = fat_lfncache;
		fat_lfncache = tmp->Next;
		free(tmp);
		return;
	}
	tmp = fat_lfncache;
	while(tmp && tmp->Next)
	{
		if(tmp->Inode == node->Inode && tmp->Disk == node->ImplPtr)
		{
			free(tmp->Next);
			tmp->Next = tmp->Next->Next;
			return;
		}
		tmp = tmp->Next;
	}
}
#endif

/**
 * \fn char *FAT_ReadDir(tVFS_Node *Node, int ID)
 * \param Node	Node structure of directory
 * \param ID	Directory position
 */
char *FAT_ReadDir(tVFS_Node *Node, int ID)
{
	fat_filetable	fileinfo[16];	//Sizeof=32, 16 per sector
	 int	a=0;
	char	*ret;
	#if USE_LFN
	char	*lfn = NULL;
	#endif
	
	ENTER("pNode iID", Node, ID);
	
	if(FAT_int_ReadDirSector(Node, ID/16, fileinfo))
	{
		LOG("End of chain, end of dir");
		LEAVE('n');
		return NULL;
	}
	
	// Offset in sector
	a = ID % 16;

	LOG("fileinfo[%i].name[0] = 0x%x", a, (Uint8)fileinfo[a].name[0]);
	
	// Check if this is the last entry
	if( fileinfo[a].name[0] == '\0' ) {
		Node->Size = ID;
		LOG("End of list");
		LEAVE('n');
		return NULL;	// break
	}
	
	// Check for empty entry
	if( (Uint8)fileinfo[a].name[0] == 0xE5 ) {
		LOG("Empty Entry");
		//LEAVE('p', VFS_SKIP);
		//return VFS_SKIP;	// Skip
		LEAVE('n');
		return NULL;	// Skip
	}
	
	#if USE_LFN
	// Get Long File Name Cache
	lfn = FAT_int_GetLFN(Node);
	if(fileinfo[a].attrib == ATTR_LFN)
	{
		fat_longfilename	*lfnInfo;
		 int	len;
		
		lfnInfo = (fat_longfilename *) &fileinfo[a];
		if(lfnInfo->id & 0x40)	memset(lfn, 0, 256);
		// Get the current length
		len = strlen(lfn);
		
		// Sanity Check (FAT implementations should not allow >255 bytes)
		if(len + 13 > 255)	return VFS_SKIP;
		// Rebase all bytes
		for(a=len+1;a--;)	lfn[a+13] = lfn[a];
		
		// Append new bytes
		lfn[ 0] = lfnInfo->name1[0];	lfn[ 1] = lfnInfo->name1[1];
		lfn[ 2] = lfnInfo->name1[2];	lfn[ 3] = lfnInfo->name1[3];
		lfn[ 4] = lfnInfo->name1[4];	
		lfn[ 5] = lfnInfo->name2[0];	lfn[ 6] = lfnInfo->name2[1];
		lfn[ 7] = lfnInfo->name2[2];	lfn[ 8] = lfnInfo->name2[3];
		lfn[ 9] = lfnInfo->name2[4];	lfn[10] = lfnInfo->name2[5];
		lfn[11] = lfnInfo->name3[0];	lfn[12] = lfnInfo->name3[1];
		LOG("lfn = '%s'", lfn);
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;
	}
	#endif
	
	//Check if it is a volume entry
	if(fileinfo[a].attrib & 0x08) {
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;
	}
	// Ignore . and ..
	if(fileinfo[a].name[0] == '.') {
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;
	}	
	
	LOG("name='%c%c%c%c%c%c%c%c.%c%c%c'",
		fileinfo[a].name[0], fileinfo[a].name[1], fileinfo[a].name[2], fileinfo[a].name[3],
		fileinfo[a].name[4], fileinfo[a].name[5], fileinfo[a].name[6], fileinfo[a].name[7],
		fileinfo[a].name[8], fileinfo[a].name[9], fileinfo[a].name[10] );
	
	#if USE_LFN
	ret = FAT_int_CreateName(&fileinfo[a], lfn);
	lfn[0] = '\0';
	#else
	ret = FAT_int_CreateName(&fileinfo[a], NULL);
	#endif
	
	LEAVE('s', ret);
	return ret;
}

/**
 * \fn tVFS_Node *FAT_FindDir(tVFS_Node *node, char *name)
 * \brief Finds an entry in the current directory
 */
tVFS_Node *FAT_FindDir(tVFS_Node *Node, char *Name)
{
	fat_filetable	fileinfo[16];
	char	tmpName[13];
	#if USE_LFN
	fat_longfilename	*lfnInfo;
	char	lfn[256];
	 int	lfnPos=255, lfnId = -1;
	#endif
	 int	i;
	tVFS_Node	*tmpNode;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	Uint32	cluster;
	
	ENTER("pNode sname", Node, Name);
	
	// Fast Returns
	if(!Name || Name[0] == '\0') {
		LEAVE('n');
		return NULL;
	}
	
	for( i = 0; ; i++ )
	{
		if((i & 0xF) == 0) {
			if(FAT_int_ReadDirSector(Node, i/16, fileinfo))
			{
				LEAVE('n');
				return NULL;
			}
		}
		
		//Check if the files are free
		if(fileinfo[i&0xF].name[0] == '\0')	break;	// End of List marker
		if(fileinfo[i&0xF].name[0] == '\xE5')	continue;	// Free entry
		
		
		#if USE_LFN
		// Long File Name Entry
		if(fileinfo[i&0xF].attrib == ATTR_LFN)
		{
			lfnInfo = (fat_longfilename *) &fileinfo[i&0xF];
			if(lfnInfo->id & 0x40) {
				memset(lfn, 0, 256);
				lfnPos = 255;
			}
			lfn[lfnPos--] = lfnInfo->name3[1];	lfn[lfnPos--] = lfnInfo->name3[0];
			lfn[lfnPos--] = lfnInfo->name2[5];	lfn[lfnPos--] = lfnInfo->name2[4];
			lfn[lfnPos--] = lfnInfo->name2[3];	lfn[lfnPos--] = lfnInfo->name2[2];
			lfn[lfnPos--] = lfnInfo->name2[1];	lfn[lfnPos--] = lfnInfo->name2[0];
			lfn[lfnPos--] = lfnInfo->name1[4];	lfn[lfnPos--] = lfnInfo->name1[3];
			lfn[lfnPos--] = lfnInfo->name1[2];	lfn[lfnPos--] = lfnInfo->name1[1];
			lfn[lfnPos--] = lfnInfo->name1[0];
			if((lfnInfo->id&0x3F) == 1)
			{
				memcpy(lfn, lfn+lfnPos+1, 256-lfnPos);
				lfnId = i+1;
			}
		}
		else
		{
			// Remove LFN if it does not apply
			if(lfnId != i)	lfn[0] = '\0';
		#endif
			// Get Real Filename
			FAT_int_ProperFilename(tmpName, fileinfo[i&0xF].name);
			LOG("tmpName = '%s'", tmpName);
		
			// Only the long name is case sensitive, 8.3 is not
			#if USE_LFN
			if(strucmp(tmpName, Name) == 0 || strcmp(lfn, Name) == 0)
			#else
			if(strucmp(tmpName, Name) == 0)
			#endif
			{
				cluster = fileinfo[i&0xF].cluster | (fileinfo[i&0xF].clusterHi << 16);
				tmpNode = Inode_GetCache(disk->inodeHandle, cluster);
				if(tmpNode == NULL)	// Node is not cached
				{
					tmpNode = FAT_int_CreateNode(Node, &fileinfo[i&0xF], i);
				}
				LEAVE('p', tmpNode);
				return tmpNode;
			}
		#if USE_LFN
		}
		#endif
	}
	
	LEAVE('n');
	return NULL;
}

#if SUPPORT_WRITE
/**
 * \fn int FAT_Mknod(tVFS_Node *Node, char *Name, Uint Flags)
 * \brief Create a new node
 */
int FAT_Mknod(tVFS_Node *Node, char *Name, Uint Flags)
{
	return 0;
}

/**
 * \fn int FAT_Relink(tVFS_Node *Node, char *OldName, char *NewName)
 * \brief Rename / Delete a file
 */
int FAT_Relink(tVFS_Node *Node, char *OldName, char *NewName)
{
	tVFS_Node	*child;
	fat_filetable	ft = {0};
	 int	ret;
	
	child = FAT_FindDir(Node, OldName);
	if(!child)	return ENOTFOUND;
	
	// Delete?
	if( NewName == NULL )
	{
		child->ImplInt |= FAT_FLAG_DELETE;	// Mark for deletion on close
		
		// Delete from the directory
		ft.name[0] = '\xE9';
		FAT_int_WriteDirEntry(Node, child->ImplInt & 0xFFFF, &ft);
		
		// Return success
		ret = EOK;
	}
	// Rename
	else
	{
		Log_Warning("FAT", "Renaming no yet supported %p ('%s' => '%s')",
			Node, OldName, NewName);
		ret = ENOTIMPL;
	}
	
	// Close child
	child->Close( child );
	return ret;
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
	if( Node->ImplInt & FAT_FLAG_DIRTY && !(Node->ImplInt & FAT_FLAG_DELETE) )
	{
		tFAT_VolInfo	buf[16];
		tFAT_VolInfo	*ft = &buf[ (Node->ImplInt & 0xFFFF) % 16 ];
		
		FAT_int_ReadDirSector(Node, (Node->ImplInt & 0xFFFF)/16, buf);
		ft->size = Node->Size;
		// TODO: update adate, mtime, mdate
		FAT_int_WriteDirEntry(Node, Node->ImplInt & 0xFFFF, ft);
		
		Node->ImplInt &= ~FAT_FLAG_DIRTY;
	}
	#endif
	
	// TODO: Make this more thread safe somehow, probably by moving the
	// Inode_UncacheNode higher up and saving the cluster value somewhere
	if( Node->ReferenceCount == 1 )
	{
		// Delete LFN Cache
		#if USE_LFN
		if(	Node->Flags & VFS_FFLAG_DIRECTORY)
			FAT_int_DelLFN(Node);
		#endif
		
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
	
	Inode_UncacheNode(disk->inodeHandle, Node->Inode);
	return ;
}

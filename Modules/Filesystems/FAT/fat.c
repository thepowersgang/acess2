/*
 * Acess 2
 * FAT12/16/32 Driver Version (Incl LFN)
 */
#define DEBUG	0
#define VERBOSE	1

#define CACHE_FAT	1	//!< Caches the FAT in memory
#define USE_LFN		1	//!< Enables the use of Long File Names

#include <acess.h>
#include <modules.h>
#include <vfs.h>
#include "fs_fat.h"


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
void	FAT_int_WriteCluster(tFAT_VolInfo *Disk, Uint32 Cluster, void *Buffer);

Uint64	FAT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	FAT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
char	*FAT_ReadDir(tVFS_Node *Node, int ID);
tVFS_Node	*FAT_FindDir(tVFS_Node *Node, char *Name);
 int	FAT_Mknod(tVFS_Node *Node, char *Name, Uint Flags);
 int	FAT_Relink(tVFS_Node *node, char *OldName, char *NewName);
void	FAT_CloseFile(tVFS_Node *node);

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
		Log_Warning("FAT", "Unable to open device '%s'", Device);
		return NULL;
	}
	
	VFS_ReadAt(diskInfo->fileHandle, 0, 512, bs);
	
	if(bs->bps == 0 || bs->spc == 0) {
		Log_Warning("FAT", "Error in FAT Boot Sector\n");
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
		Log_Log("FAT", "'%s' %s, %i %s", Device, sFatType, iSize, sSize);
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
	{
	Uint32	Ofs;
	diskInfo->FATCache = (Uint32*)malloc(sizeof(Uint32)*diskInfo->ClusterCount);
	if(diskInfo->FATCache == NULL) {
		Log_Warning("FAT", "Heap Exhausted\n");
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
	node->Size = bs->files_in_root;
	node->Inode = diskInfo->rootOffset;	// 0:31 - Cluster, 32:63 - Parent Directory Cluster
	node->ImplPtr = diskInfo;	// Disk info pointer
	node->ImplInt = 0;	// 0:15 - Directory Index, 16: Dirty Flag
	
	node->ReferenceCount = 1;
	
	node->UID = 0;	node->GID = 0;
	node->NumACLs = 1;
	node->ACLs = &gVFS_ACL_EveryoneRWX;
	node->Flags = VFS_FFLAG_DIRECTORY;
	node->CTime = node->MTime = node->ATime = now();
	
	node->Read = node->Write = NULL;
	node->ReadDir = FAT_ReadDir;
	node->FindDir = FAT_FindDir;
	node->Relink = FAT_Relink;
	node->MkNod = FAT_Mknod;
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

/*
 * === FILE IO ===
 */
/**
 * \fn Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
 * \brief Fetches a value from the FAT
 */
Uint32 FAT_int_GetFatValue(tFAT_VolInfo *Disk, Uint32 cluster)
{
	Uint32	val = 0;
	#if !CACHE_FAT
	Uint32	ofs = Disk->bootsect.resvSectCount*512;
	#endif
	ENTER("pDisk xCluster", Disk, cluster);
	LOCK( &Disk->lFAT );
	#if CACHE_FAT
	val = Disk->FATCache[cluster];
	if(Disk->type == FAT12 && val == EOC_FAT12)	val = -1;
	if(Disk->type == FAT16 && val == EOC_FAT16)	val = -1;
	if(Disk->type == FAT32 && val == EOC_FAT32)	val = -1;
	#else
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
	#endif /*CACHE_FAT*/
	RELEASE( &Disk->lFAT );
	LEAVE('x', val);
	return val;
}

/**
 * \brief Allocate a new cluster
 */
Uint32 FAT_int_AllocateCluster(tFAT_VolInfo *Disk, Uint32 Previous)
{
	Uint32	ret = Previous;
	#if CACHE_FAT
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
	#else
	Uint32	val;
	//Uint8	buf[512];
	Log_Warning("FAT", "TODO: Implement cluster allocation with non cached FAT");
	return 0;
	
	if(Disk->type == FAT12) {
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
		
		VFS_ReadAt(Disk->fileHandle, ofs+(Cluster>>1)*3, 3, &val);
		if( Cluster & 1 ) {
			val &= 0xFFF000;
			val |= eoc;
		}
		else {
			val &= 0x000FFF;
			val |= eoc<<12;
		}
		VFS_WriteAt(Disk->fileHandle, ofs+(Cluster>>1)*3, 3, &val);
	} else if(Disk->type == FAT16) {
		VFS_ReadAt(Disk->fileHandle, ofs+Previous*2, 2, &ret);
		VFS_ReadAt(Disk->fileHandle, ofs+Cluster*2, 2, &eoc);
	} else {
		VFS_ReadAt(Disk->fileHandle, ofs+Previous*4, 4, &ret);
		VFS_ReadAt(Disk->fileHandle, ofs+Cluster*4, 4, &eoc);
	}
	return ret;
	#endif
}

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
		//LOG("Reading past EOF (%i > %i)", offset, node->Size);
		LEAVE('i', 0);
		return 0;
	}
	// Clamp Size
	if(offset + length > Node->Size) {
		//LOG("Reading past EOF (%lli + %lli > %lli), clamped to %lli",
		//	offset, length, node->Size, node->Size - offset);
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
	
	preSkip = offset / bpc;
	
	//Skip previous clusters
	for(i=preSkip;i--;)	{
		cluster = FAT_int_GetFatValue(disk, cluster);
		if(cluster == -1) {
			Warning("FAT_Read - Offset is past end of cluster chain mark");
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
	LOG("pos=%i\n", pos);
	LOG("Reading the rest of the clusters\n");
	#endif
	
	
	//Read the rest of the cluster data
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
	LOG("Free tmpBuf(0x%x) and Return\n", tmpBuf);
	#endif
	
	free(tmpBuf);
	LEAVE('X', length);
	return length;
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
 * \fn char *FAT_int_CreateName(tVFS_Node *parent, fat_filetable *ft, char *LongFileName)
 * \brief Converts either a LFN or a 8.3 Name into a proper name
 */
char *FAT_int_CreateName(tVFS_Node *parent, fat_filetable *ft, char *LongFileName)
{
	char	*ret;
	 int	len;
	#if USE_LFN
	if(LongFileName && LongFileName[0] != '\0')
	{	
		len = strlen(LongFileName);
		ret = malloc(len+1);
		strcpy(ret, LongFileName);
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
	return ret;
}

/**
 * \fn tVFS_Node *FAT_int_CreateNode(tVFS_Node *parent, fat_filetable *ft, char *LongFileName)
 * \brief Creates a tVFS_Node structure for a given file entry
 */
tVFS_Node *FAT_int_CreateNode(tVFS_Node *parent, fat_filetable *ft, char *LongFileName)
{
	tVFS_Node	node = {0};
	tVFS_Node	*ret;
	tFAT_VolInfo	*disk = parent->ImplPtr;
	
	ENTER("pParent pFT sLongFileName", parent, ft, LongFileName);
	
	// Set Other Data
	node.Inode = ft->cluster | (ft->clusterHi<<16);
	node.Size = ft->size;
	LOG("ft->size = %i", ft->size);
	node.ImplPtr = parent->ImplPtr;
	node.UID = 0;	node.GID = 0;
	node.NumACLs = 1;
	
	node.Flags = 0;
	if(ft->attrib & ATTR_DIRECTORY)	node.Flags |= VFS_FFLAG_DIRECTORY;
	if(ft->attrib & ATTR_READONLY) {
		node.Flags |= VFS_FFLAG_READONLY;
		node.ACLs = &gVFS_ACL_EveryoneRX;	// R-XR-XR-X
	}
	else {
		node.ACLs = &gVFS_ACL_EveryoneRWX;	// RWXRWXRWX
	}
	
	node.ATime = timestamp(0,0,0,
			((ft->adate&0x1F)-1),	//Days
			((ft->adate&0x1E0)-1),		//Months
			1980+((ft->adate&0xFF00)>>8));	//Years
	
	node.CTime = ft->ctimems * 10;	//Miliseconds
	node.CTime += timestamp(
			(ft->ctime&0x1F)<<1,	//Seconds
			((ft->ctime&0x3F0)>>5),	//Minutes
			((ft->ctime&0xF800)>>11),	//Hours
			((ft->cdate&0x1F)-1),		//Days
			((ft->cdate&0x1E0)-1),		//Months
			1980+((ft->cdate&0xFF00)>>8));	//Years
			
	node.MTime = timestamp(
			(ft->mtime&0x1F)<<1,	//Seconds
			((ft->mtime&0x3F0)>>5),	//Minuites
			((ft->mtime&0xF800)>>11),	//Hours
			((ft->mdate&0x1F)-1),		//Days
			((ft->mdate&0x1E0)-1),		//Months
			1980+((ft->mdate&0xFF00)>>8));	//Years
	
	if(node.Flags & VFS_FFLAG_DIRECTORY) {
		node.ReadDir = FAT_ReadDir;
		node.FindDir = FAT_FindDir;
		node.MkNod = FAT_Mknod;
		node.Size = -1;
	} else {
		node.Read = FAT_Read;
		node.Write = FAT_Write;
	}
	node.Close = FAT_CloseFile;
	node.Relink = FAT_Relink;
	
	ret = Inode_CacheNode(disk->inodeHandle, &node);
	LEAVE('p', ret);
	return ret;
}

#if USE_LFN
/**
 \fn char *FAT_int_GetLFN(tVFS_Node *node)
 \brief Return pointer to LFN cache entry
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
 \fn void FAT_int_DelLFN(tVFS_Node *node)
 \brief Delete a LFN cache entry
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
 \fn char *FAT_ReadDir(tVFS_Node *Node, int ID)
 \param Node	Node structure of directory
 \param ID	Directory position
**/
char *FAT_ReadDir(tVFS_Node *Node, int ID)
{
	fat_filetable	fileinfo[16];	//Sizeof=32, 16 per sector
	 int	a=0;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	Uint32	cluster, offset;
	 int	preSkip;
	char	*ret;
	#if USE_LFN
	char	*lfn = NULL;
	#endif
	
	ENTER("pNode iID", Node, ID);
	
	// Get Byte Offset and skip
	offset = ID * sizeof(fat_filetable);
	preSkip = offset / (512 * disk->bootsect.spc);
	LOG("disk->bootsect.spc = %i", disk->bootsect.spc);
	LOG("Node->size = %i", Node->Size);
	cluster = Node->Inode & 0xFFFFFFFF;	// Cluster ID
	
	// Do Cluster Skip
	// - Pre FAT32 had a reserved area for the root.
	if( disk->type == FAT32 || cluster != disk->rootOffset )
	{
		//Skip previous clusters
		for(a=preSkip;a--;)	{
			cluster = FAT_int_GetFatValue(disk, cluster);
			// Check for end of cluster chain
			if(cluster == -1) {	LEAVE('n');	return NULL;}
		}
	}
	
	// Bounds Checking (Used to spot heap overflows)
	if(cluster > disk->ClusterCount + 2)
	{
		Log_Warning("FAT", "Cluster ID is over cluster count (0x%x>0x%x)",
			cluster, disk->ClusterCount+2);
		LEAVE('n');
		return NULL;
	}
	
	LOG("cluster=0x%x, ID=%i", cluster, ID);
	
	// Compute Offsets
	// - Pre FAT32 cluster base (in sectors)
	if( cluster == disk->rootOffset && disk->type != FAT32 )
		offset = disk->bootsect.resvSectCount + cluster*disk->bootsect.spc;
	else
	{	// FAT32 cluster base (in sectors)
		offset = disk->firstDataSect;
		offset += (cluster - 2) * disk->bootsect.spc;
	}
	// Sector in cluster
	if(disk->bootsect.spc != 1)
		offset += (ID / 16) % disk->bootsect.spc;
	// Offset in sector
	a = ID % 16;

	LOG("offset=%i, a=%i", offset, a);
	
	// Read Sector
	VFS_ReadAt(disk->fileHandle, offset*512, 512, fileinfo);	// Read Dir Data
	
	LOG("name[0] = 0x%x", (Uint8)fileinfo[a].name[0]);
	//Check if this is the last entry
	if( fileinfo[a].name[0] == '\0' ) {
		Node->Size = ID;
		LOG("End of list");
		LEAVE('n');
		return NULL;	// break
	}
	
	// Check for empty entry
	if( (Uint8)fileinfo[a].name[0] == 0xE5 ) {
		LOG("Empty Entry");
		LEAVE('p', VFS_SKIP);
		return VFS_SKIP;	// Skip
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
	
	LOG("name='%c%c%c%c%c%c%c%c.%c%c%c'\n",
		fileinfo[a].name[0], fileinfo[a].name[1], fileinfo[a].name[2], fileinfo[a].name[3],
		fileinfo[a].name[4], fileinfo[a].name[5], fileinfo[a].name[6], fileinfo[a].name[7],
		fileinfo[a].name[8], fileinfo[a].name[9], fileinfo[a].name[10] );
	
	#if USE_LFN
	//node = FAT_int_CreateNode(Node, &fileinfo[a], lfn);
	ret = FAT_int_CreateName(Node, &fileinfo[a], lfn);
	lfn[0] = '\0';
	#else
	//node = FAT_int_CreateNode(Node, &fileinfo[a], NULL);
	ret = FAT_int_CreateName(Node, &fileinfo[a], NULL);
	#endif
	
	LEAVE('s', ret);
	return ret;
}

/**
 * \fn tVFS_Node *FAT_FindDir(tVFS_Node *node, char *name)
 * \brief Finds an entry in the current directory
 */
tVFS_Node *FAT_FindDir(tVFS_Node *Node, char *name)
{
	fat_filetable	fileinfo[16];
	char	tmpName[11];
	#if USE_LFN
	fat_longfilename	*lfnInfo;
	char	*lfn = NULL;
	 int	lfnPos=255, lfnId = -1;
	#endif
	 int	i=0;
	tVFS_Node	*tmpNode;
	Uint64	diskOffset;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	Uint32	dirCluster;
	Uint32	cluster;
	
	ENTER("pNode sname", Node, name);
	
	// Fast Returns
	if(!name || name[0] == '\0') {
		LEAVE('n');
		return NULL;
	}
	
	#if USE_LFN
	lfn = FAT_int_GetLFN(Node);
	#endif
	
	dirCluster = Node->Inode & 0xFFFFFFFF;
	// Seek to Directory
	if( dirCluster == disk->rootOffset && disk->type != FAT32 )
		diskOffset = (disk->bootsect.resvSectCount+dirCluster*disk->bootsect.spc) << 9;
	else
		diskOffset = (disk->firstDataSect+(dirCluster-2)*disk->bootsect.spc) << 9;
	
	for(;;i++)
	{
		// Load sector
		if((i & 0xF) == 0) {
			//Log("FAT_FindDir: diskOffset = 0x%x", diskOffset);
			VFS_ReadAt(disk->fileHandle, diskOffset, 512, fileinfo);
			diskOffset += 512;
		}
		
		//Check if the files are free
		if(fileinfo[i&0xF].name[0] == '\0')	break;		//Free and last
		if(fileinfo[i&0xF].name[0] == '\xE5')	goto loadCluster;	//Free
		
		
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
		
			//Only Long name is case sensitive, 8.3 is not
			#if USE_LFN
			if(strucmp(tmpName, name) == 0 || strcmp(lfn, name) == 0) {
			#else
			if(strucmp(tmpName, name) == 0) {
			#endif
				cluster = fileinfo[i&0xF].cluster | (fileinfo[i&0xF].clusterHi << 16);
				tmpNode = Inode_GetCache(disk->inodeHandle, cluster);
				if(tmpNode == NULL)	// Node is not cached
				{
					#if USE_LFN
					tmpNode = FAT_int_CreateNode(Node, &fileinfo[i&0xF], lfn);
					#else
					tmpNode = FAT_int_CreateNode(Node, &fileinfo[i&0xF], NULL);
					#endif
				}
				#if USE_LFN
				lfn[0] = '\0';
				#endif
				LEAVE('p', tmpNode);
				return tmpNode;
			}
		#if USE_LFN
		}
		#endif
		
	loadCluster:
		//Load Next cluster?
		if( ((i+1) >> 4) % disk->bootsect.spc == 0 && ((i+1) & 0xF) == 0)
		{
			if( dirCluster == disk->rootOffset && disk->type != FAT32 )
				continue;
			dirCluster = FAT_int_GetFatValue(disk, dirCluster);
			if(dirCluster == -1)	break;
			diskOffset = (disk->firstDataSect+(dirCluster-2)*disk->bootsect.spc)*512;
		}
	}
	
	LEAVE('n');
	return NULL;
}

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
	return 0;
}

/**
 * \fn void FAT_CloseFile(tVFS_Node *Node)
 * \brief Close an open file
 */
void FAT_CloseFile(tVFS_Node *Node)
{
	tFAT_VolInfo	*disk = Node->ImplPtr;
	if(Node == NULL)	return ;
	
	Inode_UncacheNode(disk->inodeHandle, Node->Inode);
	#if USE_LFN
	// If node has been uncached and is a directory, delete the LFN cache
	if(	!Inode_GetCache(disk->inodeHandle, Node->Inode) && Node->Flags & VFS_FFLAG_DIRECTORY)
		FAT_int_DelLFN(Node);
	else	// Get Cache references the node, so dereference it
		Inode_UncacheNode(disk->inodeHandle, Node->Inode);
	#endif
	return ;
}

/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver core
 *
 * Reference: ntfsdoc.pdf
 */
#define DEBUG	0
#define VERBOSE	0
#include <acess.h>
#include <vfs.h>
#include "common.h"
#include <modules.h>
#include <utf16.h>

// === PROTOTYPES ===
 int	NTFS_Install(char **Arguments);
 int	NTFS_Detect(int FD);
tVFS_Node	*NTFS_InitDevice(const char *Devices, const char **Options);
void	NTFS_Unmount(tVFS_Node *Node);
// - MFT Related Functions
tNTFS_FILE_Header	*NTFS_GetMFT(tNTFS_Disk *Disk, Uint32 MFTEntry);
void	NTFS_ReleaseMFT(tNTFS_Disk *Disk, Uint32 MFTEntry, tNTFS_FILE_Header *Entry);
tNTFS_Attrib	*NTFS_GetAttrib(tNTFS_Disk *Disk, Uint32 MFTEntry, int Type, const char *Name, int DesIdx);
size_t	NTFS_ReadAttribData(tNTFS_Attrib *Attrib, Uint64 Offset, size_t Length, void *Buffer);
void	NTFS_DumpEntry(tNTFS_Disk *Disk, Uint32 Entry);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0A /*v0.1*/, FS_NTFS, NTFS_Install, NULL);
tVFS_Driver	gNTFS_FSInfo = {
	.Name = "ntfs",
	.Detect = NTFS_Detect,
	.InitDevice = NTFS_InitDevice,
	.Unmount = NTFS_Unmount,
	.GetNodeFromINode = NULL
};
tVFS_NodeType	gNTFS_DirType = {
	.TypeName = "NTFS-Dir",
	.ReadDir = NTFS_ReadDir,
	.FindDir = NTFS_FindDir,
	.Close = NTFS_Close
	};
tVFS_NodeType	gNTFS_FileType = {
	.TypeName = "NTFS-File",
	.Read = NTFS_ReadFile,
	.Close = NTFS_Close
	};

tNTFS_Disk	gNTFS_Disks;

// === CODE ===
/**
 * \brief Installs the NTFS driver
 */
int NTFS_Install(char **Arguments)
{
	VFS_AddDriver( &gNTFS_FSInfo );
	return 0;
}

/**
 * \brief Detect if a volume is NTFS
 */
int NTFS_Detect(int FD)
{
	tNTFS_BootSector	bs;
	VFS_ReadAt(FD, 0, 512, &bs);
	
	if( bs.BytesPerSector == 0 || (bs.BytesPerSector & 511) )
		return 0;

	Uint64	ncluster = bs.TotalSectorCount / bs.SectorsPerCluster;
	if( bs.MFTStart >= ncluster || bs.MFTMirrorStart >= ncluster )
		return 0;

	if( memcmp(bs.SystemID, "NTFS    ", 8) != 0 )
		return 0;
	
	return 1;
}

/**
 * \brief Mount a NTFS volume
 */
tVFS_Node *NTFS_InitDevice(const char *Device, const char **Options)
{
	tNTFS_Disk	*disk;
	tNTFS_BootSector	bs;
	
	disk = calloc( sizeof(tNTFS_Disk), 1 );
	
	disk->FD = VFS_Open(Device, VFS_OPENFLAG_READ);
	if(!disk->FD) {
		free(disk);
		return NULL;
	}
	
	VFS_ReadAt(disk->FD, 0, 512, &bs);

#if 0	
	Log_Debug("FS_NTFS", "Jump = %02x%02x%02x",
		bs.Jump[0],
		bs.Jump[1],
		bs.Jump[2]);
	Log_Debug("FS_NTFS", "SystemID = %02x%02x%02x%02x%02x%02x%02x%02x (%8C)",
		bs.SystemID[0], bs.SystemID[1], bs.SystemID[2], bs.SystemID[3],
		bs.SystemID[4], bs.SystemID[5], bs.SystemID[6], bs.SystemID[7],
		bs.SystemID
		);
	Log_Debug("FS_NTFS", "BytesPerSector = %i", bs.BytesPerSector);
	Log_Debug("FS_NTFS", "SectorsPerCluster = %i", bs.SectorsPerCluster);
	Log_Debug("FS_NTFS", "MediaDescriptor = 0x%x", bs.MediaDescriptor);
	Log_Debug("FS_NTFS", "SectorsPerTrack = %i", bs.SectorsPerTrack);
	Log_Debug("FS_NTFS", "Heads = %i", bs.Heads);
	Log_Debug("FS_NTFS", "TotalSectorCount = 0x%llx", bs.TotalSectorCount);
	Log_Debug("FS_NTFS", "MFTStart = 0x%llx", bs.MFTStart);
	Log_Debug("FS_NTFS", "MFTMirrorStart = 0x%llx", bs.MFTMirrorStart);
	Log_Debug("FS_NTFS", "ClustersPerMFTRecord = %i", bs.ClustersPerMFTRecord);
	Log_Debug("FS_NTFS", "ClustersPerIndexRecord = %i", bs.ClustersPerIndexRecord);
	Log_Debug("FS_NTFS", "SerialNumber = 0x%llx", bs.SerialNumber);
#endif
	
	disk->ClusterSize = bs.BytesPerSector * bs.SectorsPerCluster;
	disk->MFTBase = bs.MFTStart;
	Log_Debug("NTFS", "Cluster Size = %i KiB", disk->ClusterSize/1024);
	Log_Debug("NTFS", "MFT Base = %i", disk->MFTBase);
	Log_Debug("NTFS", "TotalSectorCount = 0x%x", bs.TotalSectorCount);
	
	if( bs.ClustersPerMFTRecord < 0 ) {
		disk->MFTRecSize = 1 << (-bs.ClustersPerMFTRecord);
	}
	else {
		disk->MFTRecSize = bs.ClustersPerMFTRecord * disk->ClusterSize;
	}
	//NTFS_DumpEntry(disk, 0);	// $MFT
	//NTFS_DumpEntry(disk, 3);	// $VOLUME

	disk->InodeCache = Inode_GetHandle(NTFS_FreeNode);
	
	disk->MFTDataAttr = NULL;
	disk->MFTDataAttr = NTFS_GetAttrib(disk, 0, NTFS_FileAttrib_Data, "", 0);
	//NTFS_DumpEntry(disk, 5);	// .

	disk->RootDir.I30Root = NTFS_GetAttrib(disk, 5, NTFS_FileAttrib_IndexRoot, "$I30", 0);
	disk->RootDir.I30Allocation = NTFS_GetAttrib(disk, 5, NTFS_FileAttrib_IndexAllocation, "$I30", 0);
	disk->RootDir.Node.Inode = 5;	// MFT Ent #5 is filesystem root
	disk->RootDir.Node.ImplPtr = disk;
	disk->RootDir.Node.Type = &gNTFS_DirType;
	disk->RootDir.Node.Flags = VFS_FFLAG_DIRECTORY;
	
	disk->RootDir.Node.UID = 0;
	disk->RootDir.Node.GID = 0;
	
	disk->RootDir.Node.NumACLs = 1;
	disk->RootDir.Node.ACLs = &gVFS_ACL_EveryoneRX;

	#if 0
	{
		// Read from allocation
		char buf[disk->ClusterSize];
		size_t len = NTFS_ReadAttribData(disk->RootDir.I30Allocation, 0, sizeof(buf), buf);
		Debug_HexDump("RootDir allocation", buf, len);
	}
	#endif

	return &disk->RootDir.Node;
}

/**
 * \brief Unmount an NTFS Disk
 */
void NTFS_Unmount(tVFS_Node *Node)
{
	tNTFS_Disk	*Disk = Node->ImplPtr;
	VFS_Close(Disk->FD);
	free(Disk);
}

void NTFS_Close(tVFS_Node *Node)
{
	tNTFS_Disk	*Disk = Node->ImplPtr;
	Inode_UncacheNode(Disk->InodeCache, Node->Inode);
}

void NTFS_FreeNode(tVFS_Node *Node)
{
	if( Node->Type == &gNTFS_DirType ) {
		tNTFS_Directory	*Dir = (void*)Node;
		NTFS_FreeAttrib(Dir->I30Root);
		NTFS_FreeAttrib(Dir->I30Allocation);
	}
	else {
		tNTFS_File	*File = (void*)Node;
		NTFS_FreeAttrib(File->Data);
	}
}

int NTFS_int_ApplyUpdateSequence(void *Buffer, size_t BufLen, const Uint16 *Sequence, size_t NumEntries)
{
	Uint16	cksum = Sequence[0];
	LOG("cksum = %04x", cksum);
	Sequence ++;
	Uint16	*buf16 = Buffer;
	for( int i = 0; i < NumEntries-1; i ++ )
	{
		size_t	ofs = (i+1)*512 - 2;
		if( ofs + 2 > BufLen ) {
			// Oops?
			Log_Warning("NTFS", "%x > %x", ofs+2, BufLen);
		}
		Uint16	*cksum_word = &buf16[ofs/2];
		LOG("[%i]: %04x => %04x", i, Sequence[i], *cksum_word);
		if( *cksum_word != cksum ) {
			Log_Warning("NTFS", "Disk corruption detected");
			return 1;
		}
		*cksum_word = Sequence[i];
	}
	return 0;
}

tNTFS_FILE_Header *NTFS_GetMFT(tNTFS_Disk *Disk, Uint32 MFTEntry)
{
	// TODO: Cache MFT allocation for short-term	

	// 
	tNTFS_FILE_Header	*ret = malloc( Disk->MFTRecSize );
	if(!ret) {
		Log_Warning("FS_NTFS", "malloc() fail!");
		return NULL;
	}
	
	// NOTE: The MFT is a file, and can get fragmented
	if( !Disk->MFTDataAttr ) {
		VFS_ReadAt( Disk->FD,
			Disk->MFTBase * Disk->ClusterSize + MFTEntry * Disk->MFTRecSize,
			Disk->MFTRecSize,
			ret);
	}
	else {
		NTFS_ReadAttribData(Disk->MFTDataAttr, MFTEntry * Disk->MFTRecSize, Disk->MFTRecSize, ret);
	}

	NTFS_int_ApplyUpdateSequence(ret, Disk->MFTRecSize,
		(void*)((char*)ret + ret->UpdateSequenceOfs), ret->UpdateSequenceSize
		);

	return ret;
}

void NTFS_ReleaseMFT(tNTFS_Disk *Disk, Uint32 MFTEntry, tNTFS_FILE_Header *Entry)
{
	free(Entry);
}

static inline Uint64 _getVariableLengthInt(const void *Ptr, int Length, int bExtend)
{
	const Uint8	*data = Ptr;
	Uint64	bits = 0;
	for( int i = 0; i < Length; i ++ )
		bits |= (Uint64)data[i] << (i*8);
	if( bExtend && Length && data[Length-1] & 0x80 ) {
		for( int i = Length; i < 8; i ++ )
			bits |= 0xFF << (i*8);
	}
	return bits;	// 
}

const void *_GetDataRun(const void *ptr, const void *limit, Uint64 LastLCN, Uint64 *Count, Uint64 *LCN)
{
	// Clean exit?
	if( ptr == limit ) {
		LOG("Clean end of list");
		return NULL;
	}
	
	const Uint8	*data = ptr;
	
	// Offset size
	Uint8	ofsSize = data[0] >> 4;
	Uint8	lenSize = data[0] & 0xF;
	LOG("ofsSize = %i, lenSize = %i", ofsSize, lenSize);
	if( ofsSize > 8 )
		return NULL;
	if( lenSize > 8 || lenSize < 1 )
		return NULL;
	if( data + 1 + ofsSize + lenSize > (const Uint8*)limit )
		return NULL;
	
	if( Count ) {
		*Count = _getVariableLengthInt(data + 1, lenSize, 0);
	}
	if( LCN ) {
		*LCN = LastLCN + (Sint64)_getVariableLengthInt(data + 1 + lenSize, ofsSize, 1);
	}
	
	return data + 1 + ofsSize + lenSize;
}

tNTFS_Attrib *NTFS_GetAttrib(tNTFS_Disk *Disk, Uint32 MFTEntry, int Type, const char *Name, int DesIdx)
{
	ENTER("pDisk xMFTEntry xType sName iDesIdx",
		Disk, MFTEntry, Type, Name, DesIdx);
	 int	curIdx = 0;
	// TODO: Scan cache of attributes
	
	// Load MFT entry
	tNTFS_FILE_Header *hdr = NTFS_GetMFT(Disk, MFTEntry);
	LOG("hdr = %p", hdr);

	tNTFS_FILE_Attrib	*attr;
	for( size_t ofs = hdr->FirstAttribOfs; ofs < hdr->RecordSize; ofs += attr->Size )
	{
		attr = (void*)( (tVAddr)hdr + ofs );
		// Sanity #1: Type
		if( ofs + 4 > hdr->RecordSize )
			break ;
		// End-of-list?
		if( attr->Type == 0xFFFFFFFF )
			break;
		// Sanity #2: Type,Size
		if( ofs + 8 > hdr->RecordSize )
			break;
		// Sanity #3: Reported size
		if( attr->Size < sizeof(attr->Resident) )
			break;
		// Sanity #4: Reported size fits
		if( ofs + attr->Size > hdr->RecordSize )
			break;
		
		// - Chceck if this attribute is the one requested
		LOG("Type check %x == %x", attr->Type, Type);
		if( attr->Type != Type )
			continue;
		if( Name ) {
			if( attr->NameOffset + attr->NameLength*2 > attr->Size ) {
				break;
			}
			const void	*name16 = (char*)attr + attr->NameOffset;
			LOG("Name check: '%s' == '%.*ls'",
				Name, attr->NameLength, name16);
			if( UTF16_CompareWithUTF8(attr->NameLength, name16, Name) != 0 )
				continue ;
		}
		LOG("Idx check %i", curIdx);
		if( curIdx++ != DesIdx )
			continue ;

		// - Construct (and cache) attribute description
		ASSERT(attr->NameOffset % 1 == 0);
		Uint16	*name16 = (Uint16*)attr + attr->NameOffset/2;
		size_t	namelen = UTF16_ConvertToUTF8(0, NULL, attr->NameLength, name16);
		size_t	edatalen = (attr->NonresidentFlag ? 0 : attr->Resident.AttribLen);
		tNTFS_Attrib *ret = malloc( sizeof(tNTFS_Attrib) + namelen + 1 + edatalen );
		if(!ret) {
			goto _error;
		}
		if( attr->NonresidentFlag )
			ret->Name = (void*)(ret + 1);
		else {
			ret->ResidentData = ret + 1;
			ret->Name = (char*)ret->ResidentData + edatalen;
		}
		
		ret->Disk = Disk;
		ret->Type = attr->Type;
		UTF16_ConvertToUTF8(namelen+1, ret->Name, attr->NameLength, name16);
		ret->IsResident = !(attr->NonresidentFlag);

		LOG("Creating with %x '%s'", ret->Type, ret->Name);

		if( attr->NonresidentFlag )
		{
			ret->DataSize = attr->NonResident.RealSize;
			ret->NonResident.CompressionUnitL2Size = attr->NonResident.CompressionUnitSize;
			ret->NonResident.FirstPopulatedCluster = attr->NonResident.StartingVCN;
			// Count data runs
			const char *limit = (char*)attr + attr->Size;
			 int	nruns = 0;
			const char *datarun = (char*)attr + attr->NonResident.DataRunOfs;
			while( (datarun = _GetDataRun(datarun, limit, 0, NULL, NULL)) )
				nruns ++;
			LOG("nruns = %i", nruns);
			// Allocate data runs
			ret->NonResident.nRuns = nruns;
			ret->NonResident.Runs = malloc( sizeof(tNTFS_AttribDataRun) * nruns );
			 int	i = 0;
			datarun = (char*)attr + attr->NonResident.DataRunOfs;
			Uint64	lastLCN = 0;
			while( datarun && i < nruns )
			{
				tNTFS_AttribDataRun	*run = &ret->NonResident.Runs[i];
				datarun = _GetDataRun(datarun,limit, lastLCN, &run->Count, &run->LCN);
				LOG("Run %i: %llx+%llx", i, run->LCN, run->Count);
				lastLCN = run->LCN;
				i ++;
			}
		}
		else
		{
			ret->DataSize = edatalen;
			memcpy(ret->ResidentData, (char*)attr + attr->Resident.AttribOfs, edatalen);
			Debug_HexDump("GetAttrib Resident", ret->ResidentData, edatalen);
		}
		
		NTFS_ReleaseMFT(Disk, MFTEntry, hdr);
		LEAVE('p', ret);
		return ret;
	}

_error:
	NTFS_ReleaseMFT(Disk, MFTEntry, hdr);
	LEAVE('n');
	return NULL;
}

void NTFS_FreeAttrib(tNTFS_Attrib *Attrib)
{
	if( Attrib )
		free(Attrib);
}

size_t NTFS_ReadAttribData(tNTFS_Attrib *Attrib, Uint64 Offset, size_t Length, void *Buffer)
{
	if( !Attrib )
		return 0;
	if( Offset >= Attrib->DataSize )
		return 0;
	if( Length > Attrib->DataSize )
		Length = Attrib->DataSize;
	if( Offset + Length > Attrib->DataSize )
		Length = Attrib->DataSize - Offset;
		
	if( Attrib->IsResident )
	{
		memcpy(Buffer, Attrib->ResidentData, Length);
		return Length;
	}
	else
	{
		size_t	ret = 0;
		tNTFS_Disk	*Disk = Attrib->Disk;
		Uint64	first_cluster = Offset / Disk->ClusterSize;
		size_t	cluster_ofs = Offset % Disk->ClusterSize;
		if( first_cluster < Attrib->NonResident.FirstPopulatedCluster ) {
			Log_Warning("NTFS", "TODO: Ofs < FirstVCN");
		}
		first_cluster -= Attrib->NonResident.FirstPopulatedCluster;
		if( Attrib->NonResident.CompressionUnitL2Size )
		{
			// TODO: Compression
			Log_Warning("NTFS", "Compression unsupported");
			// NOTE: Compressed blocks show up in pairs of runs
			// - The first contains the compressed data
			// - The second is a placeholder 'sparse' (LCN=0) to align to the compression unit
		}
		else
		{
			// Iterate through data runs until the desired run is located
			for( int i = 0; i < Attrib->NonResident.nRuns && Length; i ++ )
			{
				tNTFS_AttribDataRun	*run = &Attrib->NonResident.Runs[i];
				if( first_cluster > run->Count ) {
					first_cluster -= run->Count;
					continue ;
				}
				size_t	avail_bytes = (run->Count-first_cluster)*Disk->ClusterSize - cluster_ofs;
				if( avail_bytes > Length )
					avail_bytes = Length;
				// Read from this extent
				if( run->LCN == 0 ) {
					memset(Buffer, 0, avail_bytes);
				}
				else {
					VFS_ReadAt(Disk->FD,
						(run->LCN + first_cluster)*Disk->ClusterSize + cluster_ofs,
						avail_bytes,
						Buffer
						);
				}
				Length -= avail_bytes;
				Buffer += avail_bytes;
				ret += avail_bytes;
				first_cluster = 0;
				cluster_ofs = 0;
				continue ;
			}
		}
		return ret;
	}
}

/**
 * \brief Dumps a MFT Entry
 */
void NTFS_DumpEntry(tNTFS_Disk *Disk, Uint32 Entry)
{
	tNTFS_FILE_Attrib	*attr;
	 int	i;
	

	tNTFS_FILE_Header	*hdr = NTFS_GetMFT(Disk, Entry);
	if(!hdr) {
		Log_Warning("FS_NTFS", "malloc() fail!");
		return ;
	}
	
	Log_Debug("FS_NTFS", "MFT Entry #%i", Entry);
	Log_Debug("FS_NTFS", "- Magic = 0x%08x (%4C)", hdr->Magic, &hdr->Magic);
	Log_Debug("FS_NTFS", "- UpdateSequenceOfs = 0x%x", hdr->UpdateSequenceOfs);
	Log_Debug("FS_NTFS", "- UpdateSequenceSize = 0x%x", hdr->UpdateSequenceSize);
	Log_Debug("FS_NTFS", "- LSN = 0x%x", hdr->LSN);
	Log_Debug("FS_NTFS", "- SequenceNumber = %i", hdr->SequenceNumber);
	Log_Debug("FS_NTFS", "- HardLinkCount = %i", hdr->HardLinkCount);
	Log_Debug("FS_NTFS", "- FirstAttribOfs = 0x%x", hdr->FirstAttribOfs);
	Log_Debug("FS_NTFS", "- Flags = 0x%x", hdr->Flags);
	Log_Debug("FS_NTFS", "- RecordSize = 0x%x", hdr->RecordSize);
	Log_Debug("FS_NTFS", "- RecordSpace = 0x%x", hdr->RecordSpace);
	Log_Debug("FS_NTFS", "- Reference = 0x%llx", hdr->Reference);
	Log_Debug("FS_NTFS", "- NextAttribID = 0x%04x", hdr->NextAttribID);
	
	attr = (void*)( (char*)hdr + hdr->FirstAttribOfs );
	i = 0;
	while( (tVAddr)attr < (tVAddr)hdr + hdr->RecordSize )
	{
		if(attr->Type == 0xFFFFFFFF)	break;
		Log_Debug("FS_NTFS", "- Attribute %i", i ++);
		Log_Debug("FS_NTFS", " > Type = 0x%x", attr->Type);
		Log_Debug("FS_NTFS", " > Size = 0x%x", attr->Size);
		Log_Debug("FS_NTFS", " > ResidentFlag = 0x%x", attr->NonresidentFlag);
		Log_Debug("FS_NTFS", " > NameLength = %i", attr->NameLength);
		Log_Debug("FS_NTFS", " > NameOffset = 0x%x", attr->NameOffset);
		Log_Debug("FS_NTFS", " > Flags = 0x%x", attr->Flags);
		Log_Debug("FS_NTFS", " > AttributeID = 0x%x", attr->AttributeID);
		{
			Uint16	*name16 = (void*)((char*)attr + attr->NameOffset);
			size_t	len = UTF16_ConvertToUTF8(0, NULL, attr->NameLength, name16);
			char	name[len+1];
			UTF16_ConvertToUTF8(len+1, name, attr->NameLength, name16);
			Log_Debug("FS_NTFS", " > Name = '%s'", name);
		}
		if( !attr->NonresidentFlag ) {
			Log_Debug("FS_NTFS", " > AttribLen = 0x%x", attr->Resident.AttribLen);
			Log_Debug("FS_NTFS", " > AttribOfs = 0x%x", attr->Resident.AttribOfs);
			Log_Debug("FS_NTFS", " > IndexedFlag = 0x%x", attr->Resident.IndexedFlag);
			Debug_HexDump("FS_NTFS",
				(void*)( (tVAddr)attr + attr->Resident.AttribOfs ),
				attr->Resident.AttribLen
				);
		}
		else {
			Log_Debug("FS_NTFS", " > StartingVCN = 0x%llx", attr->NonResident.StartingVCN);
			Log_Debug("FS_NTFS", " > LastVCN = 0x%llx", attr->NonResident.LastVCN);
			Log_Debug("FS_NTFS", " > DataRunOfs = 0x%x", attr->NonResident.DataRunOfs);
			Log_Debug("FS_NTFS", " > CompressionUnitSize = 0x%x", attr->NonResident.CompressionUnitSize);
			Log_Debug("FS_NTFS", " > AllocatedSize = 0x%llx", attr->NonResident.AllocatedSize);
			Log_Debug("FS_NTFS", " > RealSize = 0x%llx", attr->NonResident.RealSize);
			Log_Debug("FS_NTFS", " > InitiatedSize = 0x%llx", attr->NonResident.InitiatedSize);
			Debug_HexDump("FS_NTFS",
				(char*)attr + attr->NonResident.DataRunOfs,
				attr->Size - attr->NonResident.DataRunOfs
				);
		}
		
		attr = (void*)( (tVAddr)attr + attr->Size );
	}
	
	NTFS_ReleaseMFT(Disk, Entry, hdr);
}

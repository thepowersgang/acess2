/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * dir.c - Directory Handling
 */
#define DEBUG	1
#include "common.h"
//#include "index.h"
#include <utf16.h>

// === PROTOTYPES ===
 int	NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
tVFS_Node	*NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
Uint64	NTFS_int_IndexLookup(Uint64 Inode, const char *IndexName, const char *Str);

// === CODE ===
/**
 * \brief Get the name of an indexed directory entry
 */
int NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	tNTFS_Directory	*dir = (void*)Node;
	tNTFS_Disk	*disk = Node->ImplPtr;

	ASSERT(dir->I30Root->IsResident);
	const tNTFS_Attrib_IndexRoot	*idxroot = dir->I30Root->ResidentData;
	//const tNTFS_Attrib_IndexEntry	*rootents = (void*)(idxroot + 1);

	if( idxroot->Flags & 0x01 )
	{
		// Read from allocation
		char buf[disk->ClusterSize];
		struct sNTFS_IndexHeader *hdr = (void*)buf;
		size_t	ofs = 0;
		size_t len = sizeof(buf);
		struct sNTFS_IndexEntry_Filename *ent = (void*)(buf + len);
		
		for( ; ; ent = (void*)((char*)ent + ent->EntrySize) )
		{
			if( (char*)ent == buf + len ) {
				if( len < sizeof(buf))
					break ;
				len = NTFS_ReadAttribData(dir->I30Allocation, ofs, sizeof(buf), buf);
				ofs += sizeof(buf);
				//Debug_HexDump("NTFS_ReadDir", buf, sizeof(*hdr));
				ent = (void*)(buf + (hdr->EntriesOffset + 0x18));
			}
			// TODO: When end of node is hit, should the next cluster be loaded?
			if( ent->IndexFlags & 0x02 )
				break;
			// A little hacky - hide all internal files
			if( (ent->MFTReference & ((1ULL << 48)-1)) < 12 )
				continue;
			if( Pos -- <= 0 )
				break;

			//LOG("ent = {.MFTEnt=%llx,.FilenameOfs=%x}", ent->MFTReference, ent->FilenameOfs);
			#if 0
			//Uint16	*name16 = (Uint16*)ent + ent->FilenameOfs/2;
			Uint16	*name16 = ent->Filename.Filename;
			size_t	nlen = UTF16_ConvertToUTF8(0, NULL, ent->Filename.FilenameLength, name16);
			char tmpname[ nlen+1 ];
			UTF16_ConvertToUTF8(nlen+1, tmpname, ent->Filename.FilenameLength, name16);
			LOG("name = %i '%s'", ent->Filename.FilenameNamespace, tmpname);
			#elif 0
			LOG("name = '%.*ls'", ent->Filename.FilenameLength, ent->Filename.Filename);
			#endif
		}

		if( Pos >= 0 )
			return -1;
		// Last entry does not refer to a file
		if( ent->IndexFlags & 0x02 )
			return -1;
		
		if( ent->Filename.FilenameNamespace == NTFS_FilenameNamespace_DOS )
			return 1; 

		//Uint16	*name16 = (Uint16*)ent + ent->FilenameOfs/2;
		Uint16	*name16 = ent->Filename.Filename;
		UTF16_ConvertToUTF8(FILENAME_MAX, Dest, ent->Filename.FilenameLength, name16);
		LOG("Filename '%s'", Dest);
		return 0;
	}
	else
	{
		// Local only
	}

	return -ENOTIMPL;
}

typedef int (*tNTFS_BTreeSearch_CmpFcn)(const tNTFS_IndexEntry_Filename *Ent, size_t SLen, const void *Search);

int NTFS_BTreeSearch_CmpI30(const tNTFS_IndexEntry_Filename *Ent, size_t SLen, const void *Search)
{
	#if 0
	size_t	fname_len = Ent->Filename.FilenameLength*2;
	size_t	cmplen = MIN(fnamelen, SLen);
	int ret = memcmp(Ent->Filename.Filename, Search, cmplen);
	if( ret != 0 )
		return ret;
	if( cmplen < SLen )
		return -1;
	else if( cmplen == SLen )
		return 0;
	else
		return 1;
	#else
	LOG("Cmp '%.*ls' == '%s'", Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
//	return UTF16_CompareWithUTF8(Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
	return UTF16_CompareWithUTF8CI(Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
	#endif
}

Uint64 NTFS_BTreeSearch(size_t Length, tNTFS_IndexHeader *IndexHdr,
	tNTFS_BTreeSearch_CmpFcn Cmp, size_t SLen, const void *Search)
{
	void	*buffer_end = (char*)IndexHdr + Length;
	tNTFS_IndexEntry_Filename *ent = (void*)((char*)IndexHdr + IndexHdr->EntriesOffset + 0x18);
	while( !(ent->IndexFlags & 0x02) )
	{
		if( (void*)(&ent->_rsvd + 1) > buffer_end ) {
			// on-disk error
			return 0;
		}
		// TODO: Handle collations?
		int cmp = Cmp(ent, SLen, Search);
		if( cmp == 0 ) {
			LOG("Located at %p: 0x%016llx", ent->MFTReference);
			return ent->MFTReference & ((1ULL << 48)-1);
		}
		if( cmp > 0 )
			break;
		
		ent = (void*)((char*)ent + ent->EntrySize);
	}
	if( ent->IndexFlags & 0x01 ) {
		LOG("Descend to VCN %llx", *(Uint64*)((char*)ent + ent->EntrySize - 8));
		return (1ULL << 63) | *(Uint64*)((char*)ent + ent->EntrySize - 8);
	}
	LOG("Not found");
	return 0;
}

tVFS_Node *NTFS_int_CreateNode(tNTFS_Disk *Disk, Uint64 MFTEntry)
{
	tNTFS_FILE_Header	*ent = NTFS_GetMFT(Disk, MFTEntry);
	if( !ent || !(ent->Flags & 0x01) )
		return NULL;	

	tVFS_Node	*ret;
	size_t	size;
	union {
		tNTFS_Directory	tpl_dir;
		tNTFS_File	tpl_file;
	} types;
	memset(&types, 0, sizeof(tVFS_Node));
	if( ent->Flags & 0x02 )
	{
		// Directory
		size = sizeof(types.tpl_dir);
		ret = &types.tpl_dir.Node;
		ret->Type = &gNTFS_DirType;
		ret->Flags = VFS_FFLAG_DIRECTORY;
		
		types.tpl_dir.I30Root = NTFS_GetAttrib(Disk, MFTEntry, NTFS_FileAttrib_IndexRoot, "$I30", 0);
		types.tpl_dir.I30Allocation = NTFS_GetAttrib(Disk, MFTEntry, NTFS_FileAttrib_IndexAllocation, "$I30", 0);
	}
	else
	{
		// File
		size = sizeof(types.tpl_file);
		ret = &types.tpl_file.Node;
		ret->Type = &gNTFS_FileType;
		types.tpl_file.Data = NTFS_GetAttrib(Disk, MFTEntry, NTFS_FileAttrib_Data, "", 0); 
	}
	ret->Inode = MFTEntry;
	ret->ImplPtr = Disk;

	// TODO: Permissions
	
	NTFS_ReleaseMFT(Disk, MFTEntry, ent);
	return Inode_CacheNodeEx(Disk->InodeCache, ret, size);
}

/**
 * \brief Get an entry from a directory by name
 */
tVFS_Node *NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tNTFS_Directory	*dir = (void*)Node;
	tNTFS_Disk	*disk = Node->ImplPtr;
	ASSERT(dir->I30Root->IsResident);
	const tNTFS_Attrib_IndexRoot	*idxroot = dir->I30Root->ResidentData;

	#if 0
	size_t	name16len = UTF16_ConvertFromUTF8(0, NULL, Name);
	Uint16	name16[name16len+1];
	UTF16_ConvertFromUTF8(name16len+1, name16, Name);
	#endif

	Uint64	mftent = 0;
	if( idxroot->Flags & 0x01 )
	{
		size_t	unit_len = MAX(disk->ClusterSize, 2048);
		char buf[ unit_len ];
		do {
			size_t ofs = (mftent & 0xFFFFFF) * unit_len;
			size_t len = NTFS_ReadAttribData(dir->I30Allocation, ofs, sizeof(buf), buf);
			//mftent = NTFS_BTreeSearch(len, (void*)buf, NTFS_BTreeSearch_CmpI30, name16len*2, name16);
			mftent = NTFS_BTreeSearch(len, (void*)buf, NTFS_BTreeSearch_CmpI30, -1, Name);
		} while(mftent & (1ULL << 63));
	}
	else
	{
	}
	
	if( !mftent )
		return NULL;
	
	// Allocate node
	tVFS_Node	*ret = Inode_GetCache(disk->InodeCache, mftent);
	if(ret)
		return ret;
	return NTFS_int_CreateNode(disk, mftent);
}


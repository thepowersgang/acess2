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
const tNTFS_IndexEntry_Filename *NTFS_int_IterateIndex(const char *Buf, size_t Len, int *Count)
{
	const tNTFS_IndexEntry_Filename *ent;
	for( size_t ofs = 0; ofs < Len; ofs += ent->EntrySize )
	{
		ent = (const void*)(Buf + ofs);

		if( ofs + sizeof(*ent) > Len ) {
			break;
		}
		if( ent->EntrySize < sizeof(*ent) ) {
			break;
		}
		// End of block - move on to the next one
		if( ent->IndexFlags & 0x02 ) {
			LOG("end of block at ofs %x", ofs);
			break;
		}
		
		// A little hacky - hide all internal files
		if( (ent->MFTReference & ((1ULL << 48)-1)) < 12 )
			continue;
		// Skip DOS filenames
		if( ent->Filename.FilenameNamespace == NTFS_FilenameNamespace_DOS )
			continue ;
		
		// Located
		if( (*Count)-- <= 0 )
			return ent;
	}
	return NULL;
}

/**
 * \brief Get the name of an indexed directory entry
 */
int NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX])
{
	tNTFS_Directory	*dir = (void*)Node;

	ENTER("XNode->Inode iPos", Node->Inode, Pos);

	ASSERT(dir->I30Root->IsResident);
	const tNTFS_Attrib_IndexRoot	*idxroot = dir->I30Root->ResidentData;

	// Check resident root
	const tNTFS_IndexEntry_Filename *ent;
	ent = NTFS_int_IterateIndex(
		(char*)idxroot + (0x10 + idxroot->FirstEntryOfs),
		dir->I30Root->DataSize - (0x10 + idxroot->FirstEntryOfs),
		&Pos);
	char buf[idxroot->AllocEntrySize];
	size_t	vcn = 0;
	size_t	len;
	while( !ent && (len = NTFS_ReadAttribData(dir->I30Allocation, vcn*sizeof(buf), sizeof(buf), buf)) )
	{
		// Check allocation
		struct sNTFS_IndexHeader *hdr = (void*)buf;
		ASSERT(hdr->EntriesOffset + 0x18 < len);
		// Apply update sequence
		ASSERT(hdr->UpdateSequenceOfs + 2*hdr->UpdateSequenceSize <= len);
		NTFS_int_ApplyUpdateSequence(buf,len, (void*)(buf+hdr->UpdateSequenceOfs), hdr->UpdateSequenceSize);
		size_t	ofs = hdr->EntriesOffset + 0x18;
		ent = NTFS_int_IterateIndex(buf + ofs, len - ofs, &Pos);
		vcn ++;
	}
	if( !ent ) {
		LEAVE('i', 1);
		return -1;
	}

	// TODO: This is not future-proof
	const Uint16	*name16 = ent->Filename.Filename;
	UTF16_ConvertToUTF8(FILENAME_MAX, Dest, ent->Filename.FilenameLength, name16);
	LOG("Filename '%s'", Dest);
	LEAVE('i', 0);
	return 0;
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
	//LOG("Cmp '%.*ls' == '%s'", Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
//	return UTF16_CompareWithUTF8(Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
	return UTF16_CompareWithUTF8CI(Ent->Filename.FilenameLength, Ent->Filename.Filename, Search);
	#endif
}

Uint64 NTFS_BTreeSearch(size_t Length, const void *Data,
	tNTFS_BTreeSearch_CmpFcn Cmp, size_t SLen, const void *Search)
{
	void	*buffer_end = (char*)Data + Length;
	const tNTFS_IndexEntry_Filename *ent = Data;
	while( !(ent->IndexFlags & 0x02) )
	{
		if( (void*)(&ent->_rsvd + 1) > buffer_end ) {
			// on-disk error
			return 0;
		}
		// TODO: Handle collations?
		int cmp = Cmp(ent, SLen, Search);
		if( cmp == 0 ) {
			LOG("Located at %p: 0x%016llx", ent, ent->MFTReference);
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

#define _MFTREF_IDX(ref)	((ref) & ((1ULL<<48)-1))
#define _MFTREF_SEQ(ref)	((ref) >> 48)

void NTFS_int_DumpIndex(tNTFS_Attrib *Allocation, Uint AttribID)
{
	ENTER("pAllocation xAttribID", Allocation, AttribID);
	if(!Allocation) {
		LEAVE('-');
		return ;
	}
	Uint32	vcn = 0;
	size_t	block_size = MAX(2048, Allocation->Disk->ClusterSize);
	char	buf[block_size];
	size_t	len;
	while( (len = NTFS_ReadAttribData(Allocation, vcn*block_size, block_size, buf)) )
	{
		struct sNTFS_IndexHeader	*hdr = (void*)buf;
		// Apply update sequence
		ASSERT(hdr->UpdateSequenceOfs + 2*hdr->UpdateSequenceSize <= len);
		NTFS_int_ApplyUpdateSequence(buf,len, (void*)(buf+hdr->UpdateSequenceOfs), hdr->UpdateSequenceSize);
		
		LOG("VCN %x: Ofs=%x, Size=%x",
			vcn, hdr->EntriesOffset, hdr->EntriesSize);
		if( hdr->ThisVCN != vcn ) {
			Log_Notice("NTFS", "Data error: Index header VCN mismatch (%x!=exp %x)", hdr->ThisVCN, vcn);
		}
		size_t	ofs = hdr->EntriesOffset + 0x18;
		while( ofs < hdr->EntriesSize + (hdr->EntriesOffset + 0x18) )
		{
			struct sNTFS_IndexEntry	*ent = (void*)(buf + ofs);
			if( ofs + sizeof(*ent) > len )
				break;
			LOG("%03x: L=%02x,M=%02x,F=%02x, Ref=%x/%llx", ofs,
				ent->EntrySize, ent->MessageLen, ent->IndexFlags,
				_MFTREF_SEQ(ent->MFTReference), _MFTREF_IDX(ent->MFTReference));

			if( ent->EntrySize < sizeof(*ent) ) {
				Log_Notice("NTFS", "Data error: Index entry size too small");
				break ;
			}
			if( ent->MessageLen + sizeof(*ent) > ent->EntrySize ) {
				Log_Notice("NTFS", "Data error: Index entry message size > entry size");
			}
			
			if( ent->IndexFlags & NTFS_IndexFlag_HasSubNode )
			{
				if( ent->EntrySize < sizeof(*ent) + 8 ) {
					Log_Notice("NTFS", "Data error: Index entry size too small (SubVCN)");
				}
				LOG("- SubVCN=%llx", *(Uint64*)((char*)ent + ent->EntrySize - 8));
			}
			if( ent->IndexFlags & NTFS_IndexFlag_IsLast )
				break;
			
			switch(AttribID)
			{
			case 0x30: {
				struct sNTFS_Attrib_Filename	*fname = (void*)(buf + ofs + sizeof(*ent));
				LOG("- Filename: %i %.*ls",
					fname->FilenameNamespace,
					fname->FilenameLength, fname->Filename);
				break; }
			}

			ofs += ent->EntrySize;
		}
		vcn ++;
	}
	LEAVE('-');
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
		types.tpl_dir.I30Allocation = NTFS_GetAttrib(Disk, MFTEntry,
			NTFS_FileAttrib_IndexAllocation, "$I30", 0);
		
		ASSERT(types.tpl_dir.I30Root->IsResident);
		#if 0
		Debug_HexDump("NTFS CreateNode Root",
			types.tpl_dir.I30Root->ResidentData,
			types.tpl_dir.I30Root->DataSize);
		if( types.tpl_dir.I30Allocation ) {
			NTFS_int_DumpIndex(types.tpl_dir.I30Allocation, 0x30);
		}
		#endif
	
		#if 0
		if( MFTEntry == 0x1C )
		{	
			char tmpbuf[Disk->ClusterSize];
			NTFS_ReadAttribData(types.tpl_dir.I30Allocation, 0, sizeof(tmpbuf), tmpbuf);
			Debug_HexDump("NTFS CreateNode VCN#0", tmpbuf, sizeof(tmpbuf));
			NTFS_ReadAttribData(types.tpl_dir.I30Allocation, sizeof(tmpbuf), sizeof(tmpbuf), tmpbuf);
			Debug_HexDump("NTFS CreateNode VCN#1", tmpbuf, sizeof(tmpbuf));
		}
		#endif
	}
	else
	{
		// File
		size = sizeof(types.tpl_file);
		ret = &types.tpl_file.Node;
		ret->Type = &gNTFS_FileType;
		types.tpl_file.Data = NTFS_GetAttrib(Disk, MFTEntry, NTFS_FileAttrib_Data, "", 0); 
		ret->Size = types.tpl_file.Data->DataSize;
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

	ENTER("XNode->Inode sName xFlags",
		Node->Inode, Name, Flags);

	#if 0
	size_t	name16len = UTF16_ConvertFromUTF8(0, NULL, Name);
	Uint16	name16[name16len+1];
	UTF16_ConvertFromUTF8(name16len+1, name16, Name);
	#endif

	Uint64	mftent = 0;

	// Check resident first
	size_t	ofs = 0x10 + idxroot->FirstEntryOfs;
	mftent = NTFS_BTreeSearch(
		dir->I30Root->DataSize - ofs, dir->I30Root->ResidentData + ofs,
		NTFS_BTreeSearch_CmpI30, -1, Name
		);
	while( mftent & (1ULL << 63) )
	{
		size_t	unit_len = idxroot->AllocEntrySize;
		char buf[ unit_len ];
		size_t ofs = (mftent & 0xFFFFFF) * unit_len;
		size_t len = NTFS_ReadAttribData(dir->I30Allocation, ofs, sizeof(buf), buf);
		if( len == 0 )
			break;
		tNTFS_IndexHeader	*hdr = (void*)buf;
		
		if( memcmp(&hdr->Magic, "INDX", 4) != 0 ) {
			Log_Notice("NTFS", "FindDir %p:%X:%x index magic bad %08x",
				disk, Node->Inode, ofs / unit_len, hdr->Magic);
			break;
		}
		// Apply update sequence
		if(hdr->UpdateSequenceOfs + 2*hdr->UpdateSequenceSize > len) {
			Log_Notice("NTFS", "FindDir %p:%X:%x index update sequence out of buffer (%x+%x>%x)",
				disk, Node->Inode, ofs / unit_len,
				hdr->UpdateSequenceOfs, 2*hdr->UpdateSequenceSize, len);
			break;
		}
		NTFS_int_ApplyUpdateSequence(buf,len, (void*)(buf+hdr->UpdateSequenceOfs), hdr->UpdateSequenceSize);
		// Search
		//mftent = NTFS_BTreeSearch(len, (void*)buf, NTFS_BTreeSearch_CmpI30, name16len*2, name16);
		mftent = NTFS_BTreeSearch(
			len-(hdr->EntriesOffset+0x18), buf+hdr->EntriesOffset+0x18,
			NTFS_BTreeSearch_CmpI30, -1, Name
			);
	}
	
	if( !mftent || (mftent & (1ULL << 63)) ) {
		LEAVE('n');
		return NULL;
	}
	
	// Allocate node
	tVFS_Node	*ret = Inode_GetCache(disk->InodeCache, mftent);
	if(!ret)
		ret = NTFS_int_CreateNode(disk, mftent);
	LEAVE('p', ret);
	return ret;
}


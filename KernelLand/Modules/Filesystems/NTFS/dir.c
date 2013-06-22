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
#include "index.h"
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
		
		for(;;)
		{
			if( (char*)ent == buf + len ) {
				if( len < sizeof(buf))
					break ;
				len = NTFS_ReadAttribData(dir->I30Allocation, ofs, sizeof(buf), buf);
				ofs += sizeof(buf);
				//Debug_HexDump("NTFS_ReadDir", buf, sizeof(*hdr));
				ent = (void*)(buf + (hdr->EntriesOffset + 0x18));
			}
			if( Pos -- <= 0 )
				break;
			LOG("ent = {.MFTEnt=%llx,.FilenameOfs=%x}", ent->MFTReference, ent->FilenameOfs);
			//Uint16	*name16 = (Uint16*)ent + ent->FilenameOfs/2;
			Uint16	*name16 = ent->Filename.Filename;
			size_t	nlen = UTF16_ConvertToUTF8(0, NULL, ent->Filename.FilenameLength, name16);
			char tmpname[ nlen+1 ];
			UTF16_ConvertToUTF8(nlen+1, tmpname, ent->Filename.FilenameLength, name16);
			LOG("name = '%s'", tmpname);
			ent = (void*)((char*)ent + ent->EntrySize);
		}

		if( Pos < 0 )
		{
			//Uint16	*name16 = (Uint16*)ent + ent->FilenameOfs/2;
			Uint16	*name16 = ent->Filename.Filename;
			UTF16_ConvertToUTF8(FILENAME_MAX, Dest, ent->Filename.FilenameLength, name16);
			return 0;
		}
	}
	else
	{
		// Local only
	}

	return -ENOTIMPL;
}

/**
 * \brief Get an entry from a directory by name
 */
tVFS_Node *NTFS_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	#if 0
	tNTFS_Directory	*dir = (void*)Node;
	tNTFS_Disk	*disk = Node->ImplPtr;
	ASSERT(dir->I30Root->IsResident);
	const tNTFS_Attrib_IndexRoot	*idxroot = dir->I30Root->ResidentData;

	if( idxroot->Flags & 0x01 )
	{
		char buf[disk->ClusterSize];
		size_t len = NTFS_ReadAttribData(dir->I30Allocation, 0, sizeof(buf), buf);
		struct sNTFS_IndexHeader *hdr = (void*)buf;
	}
	#endif
	
	return NULL;
}

/**
 * \brief Scans an index for the requested value and returns the associated ID
 */
Uint64 NTFS_int_IndexLookup(Uint64 Inode, const char *IndexName, const char *Str)
{
	return 0;
}

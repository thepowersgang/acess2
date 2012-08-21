/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * common.h - Common Types and Definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <acess.h>
#include <vfs.h>

typedef Uint16	WCHAR;

// === STRUCTURES ===
/**
 * In-memory representation of an NTFS Disk
 */
typedef struct sNTFS_Disk
{
	 int	FD;
	 int	CacheHandle;
	 
	 int	ClusterSize;
	Uint64	MFTBase;
	Uint32	MFTRecSize;
	
	tVFS_Node	RootNode;
}	tNTFS_Disk;

typedef struct sNTFS_BootSector
{
	// 0
	Uint8	Jump[3];
	Uint8	SystemID[8];	// = "NTFS    "
	Uint16	BytesPerSector;
	Uint8	SectorsPerCluster;
	
	// 0xE
	Uint8	Unused[7];
	Uint8	MediaDescriptor;
	Uint16	Unused2;
	Uint16	SectorsPerTrack;
	Uint16	Heads;
	
	// 0x1C
	Uint64	Unused3;
	Uint32	Unkown;	// Usually 0x00800080 (according to Linux docs)
	
	// 0x28
	Uint64	TotalSectorCount;	// Size of volume in sectors
	Uint64	MFTStart;	// Logical Cluster Number of Cluster 0 of MFT
	Uint64	MFTMirrorStart;	// Logical Cluster Number of Cluster 0 of MFT Backup
	
	// 0x40
	// If either of these are -ve, the size can be obtained via
	// SizeInBytes = 2^(-1 * Value)
	Sint8	ClustersPerMFTRecord;
	Uint8	Unused4[3];
	Sint8	ClustersPerIndexRecord;
	Uint8	Unused5[3];
	
	Uint64	SerialNumber;
	
	Uint8	Padding[512-0x50];
	
} PACKED	tNTFS_BootSector;

/**
 * FILE header, an entry in the MFT
 */
typedef struct sNTFS_FILE_Header
{
	Uint32	Magic;	// 'FILE'
	Uint16	UpdateSequenceOfs;
	Uint16	UpdateSequenceSize;	// Size in words of the UpdateSequenceArray
	
	Uint64	LSN;	// $LogFile Sequence Number
	
	Uint16	SequenceNumber;
	Uint16	HardLinkCount;	
	Uint16	FirstAttribOfs;	// Size of header?
	Uint16	Flags;	// 0: In Use, 1: Directory
	
	Uint32	RecordSize;		// Real Size of FILE Record
	Uint32	RecordSpace;	// Allocated Size for FILE Record
	
	/**
	 * Base address of the MFT containing this record
	 */
	Uint64	Reference;	// "File reference to the base FILE record" ???
	
	Uint16	NextAttribID;
	union
	{
		// Only in XP
		struct {
			Uint16	AlignTo4Byte;
			Uint16	RecordNumber;	// Number of this MFT Record
			Uint16	UpdateSequenceNumber;
			Uint16	UpdateSequenceArray[];
		}	XP;
		struct {
			Uint16	UpdateSequenceNumber;
			Uint16	UpdateSequenceArray[];
		}	All;
	} OSDep;	
	
} PACKED	tNTFS_FILE_Header;

/**
 * File Attribute, follows the FILE header
 */
typedef struct sNTFS_FILE_Attrib
{
	Uint32	Type;	// See eNTFS_FILE_Attribs
	Uint32	Size;	// Includes header
	
	Uint8	ResidentFlag;	// (What does this mean?)
	Uint8	NameLength;
	Uint16	NameOffset;
	Uint16	Flags;	// 0: Compressed, 14: Encrypted, 15: Sparse
	Uint16	AttributeID;
	
	union
	{
		struct {
			Uint32	AttribLen;	// In words
			Uint16	AttribOfs;
			Uint8	IndexedFlag;
			Uint8	Padding;
			
			Uint16	Name[];	// UTF-16
			// Attribute Data
		}	Resident;
		struct {
			Uint64	StartingVCN;
			Uint64	LastVCN;
			Uint16	DataRunOfs;
			Uint16	CompressionUnitSize;
			Uint32	Padding;
			Uint64	AllocatedSize;
			Uint64	RealSize;
			Uint64	InitiatedSize;	// One assumes, ammount of actual data stored
			Uint16	Name[];	// UTF-16
			// Data Runs
		}	NonResident;
	};
} PACKED	tNTFS_FILE_Attrib;

extern int	NTFS_ReadDir(tVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
extern tVFS_Node	*NTFS_FindDir(tVFS_Node *Node, const char *Name);

#endif

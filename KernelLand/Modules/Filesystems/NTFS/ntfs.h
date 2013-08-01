/*
 * Acess2 NTFS Driver
 * - By John Hodge (thePowersGang)
 *
 * ntfs.h
 * - NTFS on-disk structures
 */
#ifndef _NTFS__NTFS_H_
#define _NTFS__NTFS_H_

typedef struct sNTFS_BootSector	tNTFS_BootSector;

struct sNTFS_BootSector
{
	// 0
	Uint8	Jump[3];
	Uint8	SystemID[8];	// = "NTFS    "
	Uint16	BytesPerSector;
	Uint8	SectorsPerCluster;
	
	// 0xE
	Uint8	_unused[7];
	Uint8	MediaDescriptor;
	Uint16	_unused2;
	Uint16	SectorsPerTrack;
	Uint16	Heads;
	
	// 0x1C
	Uint64	_unused3;
	Uint32	_unknown;	// Usually 0x00800080 (according to Linux docs)
	
	// 0x28
	Uint64	TotalSectorCount;	// Size of volume in sectors
	Uint64	MFTStart;	// Logical Cluster Number of Cluster 0 of MFT
	Uint64	MFTMirrorStart;	// Logical Cluster Number of Cluster 0 of MFT Backup
	
	// 0x40
	// If either of these are -ve, the size can be obtained via
	// SizeInBytes = 2^(-1 * Value)
	Sint8	ClustersPerMFTRecord;
	Uint8	_unused4[3];
	Sint8	ClustersPerIndexRecord;
	Uint8	_unused5[3];
	
	Uint64	SerialNumber;
	
	Uint8	Padding[512-0x50];
	
} PACKED;

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
	
	Uint8	NonresidentFlag;
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
			Uint64	StartingVCN;	// VCN of first data run
			Uint64	LastVCN;	// Last VCN in data runs
			Uint16	DataRunOfs;
			Uint16	CompressionUnitSize;
			Uint32	Padding;
			Uint64	AllocatedSize;	// Allocated clusters in bytes
			Uint64	RealSize;
			Uint64	InitiatedSize;	// One assumes, ammount of actual data stored
			Uint16	Name[];	// UTF-16
			// Data Runs
		}	NonResident;
	};
} PACKED	tNTFS_FILE_Attrib;

#include "attributes.h"

typedef struct sNTFS_IndexHeader	tNTFS_IndexHeader;
typedef struct sNTFS_IndexEntry_Filename	tNTFS_IndexEntry_Filename;

struct sNTFS_IndexHeader
{
	Uint32	Magic;	// = 'INDX' LE
	Uint16	UpdateSequenceOfs;
	Uint16	UpdateSequenceSize;	// incl number
	Uint64	LogFileSequenceNum;
	Uint64	ThisVCN;
	Uint32	EntriesOffset;	// add 0x18
	Uint32	EntriesSize;	// (maybe) add 0x18
	Uint32	EntriesAllocSize;
	Uint8	Flags;	// [0]: Not leaf node
	Uint8	_pad[3];
	Uint16	UpdateSequence;
	Uint16	UpdateSequenceArray[];
} PACKED;

#define NTFS_IndexFlag_HasSubNode	0x01
#define NTFS_IndexFlag_IsLast	0x02

struct sNTFS_IndexEntry
{
	Uint64	MFTReference;
	Uint16	EntrySize;
	Uint16	MessageLen;
	Uint16	IndexFlags;	// [0]: Points to sub-node, [1]: Last entry in node
	Uint16	_rsvd;
} PACKED;
struct sNTFS_IndexEntry_Filename
{
	Uint64	MFTReference;
	Uint16	EntrySize;
	Uint16	FilenameOfs;
	Uint16	IndexFlags;	// [0]: Points to sub-node, [1]: Last entry in node
	Uint16	_rsvd;

	#if 1
	struct sNTFS_Attrib_Filename	Filename;
	#else
	Uint64	ParentMFTReference;
	Uint64	CreationTime;
	Uint64	ModifcationTime;
	Uint64	MFTModTime;
	Uint64	AccessTime;
	Uint64	AllocSize;
	Uint64	RealSize;
	Uint64	FileFlags;
	
	Uint8	FilenameLen;
	Uint8	FilenameNamespace;
	#endif
	// Filename
} PACKED;

#endif


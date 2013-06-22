/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * attributes.h - MFT Attribute Types
 */
#ifndef _ATTRIBUTES_H_
#define _ATTRIBUTES_H_

enum eNTFS_FILE_Attribs
{
	NTFS_FileAttrib_StandardInformation = 0x10,
	NTFS_FileAttrib_FileName	= 0x30,
	NTFS_FileAttrib_SecurityDescriptor = 0x50,
	NTFS_FileAttrib_Data    	= 0x80,
	NTFS_FileAttrib_IndexRoot	= 0x90,
	NTFS_FileAttrib_IndexAllocation = 0xA0,
	NTFS_FileAttrib_Bitmap  	= 0xB0,
	// NTFS_FileAttrib_
};

typedef struct
{
	Uint64	CreationTime;
	Uint64	AlterTime;
	Uint64	MFTChangeTime;
	Uint64	ReadTime;
	Uint32	DOSFilePermissions;
	Uint32	MaxNumVersions;
	Uint32	VersionNumber;
	Uint32	ClassID;
	// 2K+
	Uint32	OwnerID;
	Uint32	SecurityID;
	Uint64	QuotaCharged;
	Uint64	UpdateSequenceNumber;
} PACKED tNTFS_Attrib_StandardInformation;

enum eNTFS_Filename_Namespaces
{
	NTFS_FilenameNamespace_POSIX,	// [^NUL,/]
	NTFS_FilenameNamespace_Win32,	//
	NTFS_FilenameNamespace_DOS,	//
	NTFS_FilenameNamespace_Win32DOS,	// Same name in both Win32/DOS, so merged
};

typedef struct sNTFS_Attrib_Filename
{
	Uint64	ParentDirectory;	//!< Parent directory MFT entry
	Sint64	CreationTime;	//!< Time the file was created
	Sint64	LastDataModTime;	//!< Last change time for the data
	Sint64	LastMftModTime;	//!< Last change time for the MFT entry
	Sint64	LastAccessTime;	//!< Last Access Time (unreliable on most systems)
	
	Uint64	AllocatedSize;	//!< Allocated data size for $DATA unnamed stream
	Uint64	DataSize;	//!< Actual size of $DATA unnamed stream
	Uint32	Flags;	//!< File attribute files
	
	union {
		struct {
			Uint16	PackedSize;	//!< Size of buffer needed for extended attributes
			Uint16	_reserved;
		} PACKED	ExtAttrib;
		struct {
			Uint32	Tag;	//!< Type of reparse point
		} PACKED	ReparsePoint;
	} PACKED	Type;

	Uint8	FilenameLength;	
	Uint8	FilenameNamespac;	//!< Filename namespace (DOS, Windows, Unix)
	Uint16	Filename[0];
} PACKED	tNTFS_Attrib_Filename;

typedef struct sNTFS_Attrib_IndexEntry
{
	Uint64	FileReference;
	Uint16	EntryLength;
	Uint16	StreamLength;
	Uint8	Flags;	// [0]: Sub-node, [1]: Last entry in node
	Uint8	_pad[3];
	// Stream data and sub-node VCN (64-bit) follows
} PACKED tNTFS_Attrib_IndexEntry;

typedef struct sNTFS_Attrib_IndexRoot
{
	// Index Root
	Uint32	AttributeType;	// Type of indexed attribute
	Uint32	CollationRule;	// Sorting method
	Uint32	AllocEntrySize;	// 
	Uint8	ClustersPerIndexRec;
	Uint8	_pad1[3];
	// Index Header
	Uint32	FirstEntryOfs;
	Uint32	TotalSize;
	Uint32	AllocSize;
	Uint8	Flags;
	Uint8	_pad2[3];
	// List of IndexEntry structures follow
} PACKED	tNTFS_Attrib_IndexRoot;


#endif

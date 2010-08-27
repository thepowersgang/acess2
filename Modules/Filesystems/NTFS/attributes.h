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

typedef struct
{
	Uint64	ParentDirectory;	//!< Parent directory MFT entry
	Sint64	CreationTime;	//!< Time the file was created
	Sint64	LastDataModTime;	//!< Last change time for the data
	Sint64	LastMtfModTime;	//!< Last change time for the MFT entry
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
	
	Uint8	FilenameType;	//!< Filename namespace (DOS, Windows, Unix)
	WCHAR	Filename[0];
} PACKED	tNTFS_Attrib_Filename;

#endif

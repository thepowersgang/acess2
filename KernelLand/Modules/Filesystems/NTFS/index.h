/*
 * Acess2 - NTFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * index.h - Index Types
 */
#ifndef _INDEX_H_
#define _INDEX_H_

#include "attributes.h"

typedef struct
{
	Uint32	EntryOffset;
	Uint32	IndexLength;
	Uint32	AllocateSize;
	Uint8	Flags;
	Uint8	_reserved[3];
} PACKED	tNTFS_IndexHeader;

typedef struct
{
	Uint32	Type;
	Uint32	CollationRule;
	Uint32	IndexBlockSize;
	Uint8	ClustersPerIndexBlock;
	Uint8	_reserved[3];
	tNTFS_IndexHeader	Header;
} PACKED	tNTFS_IndexRoot;

typedef struct
{
	union {
		struct {
			Uint64	File;	// MFT Index of file
		} PACKED	Dir;
		/**
		 * Views/Indexes
		 */
		struct {
			Uint16	DataOffset;
			Uint16	DataLength;
			Uint32	_reserved;
		} PACKED	ViewIndex;
	} PACKED	Header;
	
	Uint16	Length;	//!< Size of the index entry (multiple of 8 bytes)
	Uint16	KeyLength;	//!< Size of key value
	Uint16	Flags;	//!< Flags Bitfield
	Uint16	_reserved;
	
	/**
	 * \brief Key Data
	 * \note Only valid if \a Flags does not have \a INDEX_ENTRY_END set
	 * \note In NTFS3 only \a Filename is used
	 */
	union {
		tNTFS_Attrib_Filename	Filename;
		//TODO: more key types
	} PACKED	Key;
} PACKED	tNTFS_IndexEntry;

#endif

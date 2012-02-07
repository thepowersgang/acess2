/*
 * Acess 2 LEAN Filesystem Driver
 * By John Hodge (thePowersGang)
 *
 * lean.h - Filesystem Structure Definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#define EXTENTS_PER_INODE	6
#define EXTENTS_PER_INDIRECT	38

struct sLEAN_Superblock
{
	// TODO
};

struct sLEAN_IndirectBlock
{
	Uint32	Checksum;	// Block checksum
	Uint32	Magic;	// 'INDX'
	Uint64	SectorCount;	// Number of blocks referenced by this block
	Uint64	Inode;	// Inode this block belongs to (in for robustness)
	Uint64	ThisSector;	// Sector number of this block
	Uint64	PrevIndirect;	// Backlink to the previous indirect block (or zero)
	Uint64	NextIndirect;	// Link to the next indirect block (or zero)
	
	/**
	 * Number of extents stored in this block (only the last block can
	 * have this as < \a EXTENTS_PER_INDIRECT
	 */
	Uint8	NumExtents;
	Uint8	reserved1[3];	//!< Reserved/Padding
	Uint32	reserved2;	//!< Reserved/Padding
	
	Uint64	ExtentsStarts[EXTENTS_PER_INDIRECT];
	Uint32	ExtentsSizes[EXTENTS_PER_INDIRECT];
}

struct sLEAN_Inode
{
	Uint32	Checksum;	//!< Checksum of the inode
	Uint32	Magic;	//!< Magic Value 'NODE'
	
	Uint8	ExtentCount;	//!< Number of extents defined in the inode structure
	Uint8	reserved1[3];	//!< Reserved/Padding
	
	Uint32	IndirectCount;	//!< Number of indirect extent blocks used by the file
	Uint32	LinkCount;	//!< Number of directory entires that refer to this inode
	
	Uint32	UID;	//!< Owning User
	Uint32	GID;	//!< Owning Group
	Uint32	Attributes;	//!< 
	Uint64	FileSize;	//!< Size of the file data (not including inode and other bookkeeping)
	Uint64	SectorCount;	//!< Number of sectors allocated to the file
	Sint64	LastAccessTime;	//!< Last access time
	Sint64	StatusChangeTime;	//!< Last change of the status bits time
	Sint64	ModifcationTime;	//!< Last modifcation time
	Sint64	CreationTime;	//!< File creation time
	
	Uint64	FirstIndirect;	//!< Sector number of the first indirect block
	Uint64	LastIndirect;	//!< Sector number of the last indirect block
	
	Uint64	Fork;	//!< ????
	
	Uint64	ExtentsStarts[EXTENTS_PER_INODE];
	Uint32	ExtentsSizes[EXTENTS_PER_INODE];
};

enum eLEAN_InodeAttributes
{
	LEAN_iaXOth = 1 << 0,	LEAN_iaWOth = 1 << 1,	LEAN_iaROth = 1 << 2,
	LEAN_iaXGrp = 1 << 3,	LEAN_iaWGrp = 1 << 4,	LEAN_iaRGrp = 1 << 5,
	LEAN_iaXUsr = 1 << 6,	LEAN_iaWUsr = 1 << 7,	LEAN_iaRUsr = 1 << 8,
	
	LEAN_iaSVTX = 1 << 9,
	LEAN_iaSGID = 1 << 10,
	LEAN_iaSUID = 1 << 11,
	
	LEAN_iaHidden	= 1 << 12,
	LEAN_iaSystem	= 1 << 13,
	LEAN_iaArchive	= 1 << 14,	// Set on any write
	LEAN_iaSync 	= 1 << 15,
	LEAN_iaNoAccessTime	= 1 << 16,	//!< Don't update the last accessed time
	LEAN_iaImmutable	= 1 << 17,	//!< Don't move sectors (defragger flag)
	LEAN_iaPrealloc	= 1 << 18,	//!< Keep preallocated sectors after close
	LEAN_iaInlineExtAttr	= 1 << 19,	//!< Reserve the first sector
	
	LEAN_iaFmtMask	= 7 << 29,	//!< Format mask
	LEAN_iaFmtRegular	= 1 << 29,	//!< Regular File
	LEAN_iaFmtDirectory	= 2 << 29,	//!< Directory
	LEAN_iaFmtSymlink	= 3 << 29,	//!< Symlink
	LEAN_iaFmtFork	= 4 << 29,	//!< Fork
};

struct tLEAN_ExtendedAttribute
{
	Uint32	Header;	// 8.24 Name.Value Sizes
};

#endif

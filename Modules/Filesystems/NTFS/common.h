/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

// === STRUCTURES ===
typedef struct sNTFS_Disk
{
	
}	tNTFS_Disk;

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
	
}	tNTFS_FILE_Header;

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
}	tNTFS_FILE_Attrib;

#endif

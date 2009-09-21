/*
AcessOS/AcessBasic v1
PE Loader
HEADER
*/
#ifndef _EXE_PE_H
#define _EXE_PE_H

enum ePE_MACHINES {
	PE_MACHINE_I386 = 0x14c,	// Intel 386+
	PE_MACHINE_IA64 = 0x200		// Intel-64
};

enum ePE_DIR_INDX {
	PE_DIR_EXPORT,		// 0
	PE_DIR_IMPORT,		// 1
	PE_DIR_RESOURCE,	// 2
	PE_DIR_EXCEPTION,	// 3
	PE_DIR_SECRURITY,	// 4
	PE_DIR_RELOC,		// 5
	PE_DIR_DEBUG,		// 6
	PE_DIR_COPYRIGHT,	// 7
	PE_DIR_ARCHITECTURE,// 8
	PE_DIR_GLOBALPTR,	// 9
	PE_DIR_TLS,			// 10
	PE_DIR_LOAD_CFG,	// 11
	PE_DIR_BOUND_IMPORT,// 12
	PE_DIR_IAT,			// 13
	PE_DIR_DELAY_IMPORT,// 14
	PE_DIR_COM_DESCRIPTOR,	//15
	PE_DIR_LAST
};

typedef struct {
	Uint32	RVA;
	Uint32	Size;
} tPE_DATA_DIR;

typedef struct {
	Uint32	*ImportLookupTable;	//0x80000000 is Ordninal Flag
	Uint32	TimeStamp;
	Uint32	FowarderChain;
	char	*DLLName;
	Uint32	*ImportAddressTable;	// Array of Addresses - To be edited by loader
} tPE_IMPORT_DIR;

typedef struct {
	Uint32	Flags;
	Uint32	Timestamp;
	Uint16	MajorVer;
	Uint16	MinorVer;
	Uint32	NameRVA;	// DLL Name
	Uint32	OrdinalBase;
	Uint32	NumAddresses;
	Uint32	NumNamePointers;
	Uint32	AddressRVA;
	Uint32	NamePointerRVA;
	Uint32	OrdinalTableRVA;
} tPE_EXPORT_DIR;

typedef struct {
	Uint16	Hint;
	char	Name[];	// Zero Terminated String
} tPE_HINT_NAME;

typedef struct {
	char	Name[8];
	Uint32	VirtualSize;
	Uint32	RVA;
	Uint32	RawSize;
	Uint32	RawOffs;
	Uint32	RelocationsPtr;	//Set to 0 in executables
	Uint32	LineNumberPtr;	//Pointer to Line Numbers
	Uint16	RelocationCount;	// Set to 0 in executables
	Uint16	LineNumberCount;
	Uint32	Flags;
} tPE_SECTION_HEADER;

#define PE_SECTION_FLAG_CODE	0x00000020	// Section contains executable code.
#define PE_SECTION_FLAG_IDATA	0x00000040	// Section contains initialized data.
#define PE_SECTION_FLAG_UDATA	0x00000080	// Section contains uninitialized data.
#define PE_SECTION_FLAG_DISCARDABLE	0x02000000	// Section can be discarded as needed.
#define PE_SECTION_FLAG_MEM_NOT_CACHED	0x04000000	// Section cannot be cached.
#define PE_SECTION_FLAG_MEM_NOT_PAGED	0x08000000	// Section is not pageable.
#define PE_SECTION_FLAG_MEM_SHARED	0x10000000	// Section can be shared in memory.
#define PE_SECTION_FLAG_MEM_EXECUTE	0x20000000	// Section can be executed as code.
#define PE_SECTION_FLAG_MEM_READ	0x40000000	// Section can be read.
#define PE_SECTION_FLAG_MEM_WRITE	0x80000000	// Section can be written to.

typedef struct {
	Uint32	page;
	Uint32	size;
	Uint16	ents[];
} tPE_FIXUP_BLOCK;

//File Header
typedef struct {
	Uint16	Machine;
	Uint16	SectionCount;
	Uint32	CreationTimestamp;
	Uint32	SymbolTableOffs;
	Uint32	SymbolCount;
	Uint16	OptHeaderSize;
	Uint16	Flags;
} tPE_FILE_HEADER;

typedef struct {
	Uint16	Magic;	//0x10b: 32Bit, 0x20b: 64Bit
	Uint16	LinkerVersion;
	Uint32	CodeSize;	//Sum of all Code Segment Sizes
	Uint32	DataSize;	//Sum of all Intialised Data Segments
	Uint32	BssSize;	//Sum of all Unintialised Data Segments
	Uint32	EntryPoint;
	Uint32	CodeRVA;
	Uint32	DataRVA;
	Uint32	ImageBase;	//Prefered Base Address
	Uint32	SectionAlignment;
	Uint32	FileAlignment;
	Uint32	WindowsVersion;	//Unused/Irrelevent
	Uint32	ImageVersion;	//Unused/Irrelevent
	Uint32	SubsystemVersion;	//Unused, Set to 4
	Uint32	Win32Version;	//Unused
	Uint32	ImageSize;
	Uint32	HeaderSize;
	Uint32	Checksum;	//Unknown Method, Can be set to 0
	Uint16	Subsystem;	//Required Windows Subsystem (None, GUI, Console)
	Uint16	DllFlags;
	Uint32	MaxStackSize;		//Reserved Stack Size
	Uint32	InitialStackSize;	//Commited Stack Size
	Uint32	InitialReservedHeap;	// Reserved Heap Size
	Uint32	InitialCommitedHeap;	// Commited Heap Size
	Uint32	LoaderFlags;	// Obselete
	Uint32	NumberOfDirEntries;
	tPE_DATA_DIR	Directory[16];
} tPE_OPT_HEADER;

// Root Header
typedef struct {
	Uint32	Signature;
	tPE_FILE_HEADER	FileHeader;
	tPE_OPT_HEADER	OptHeader;
} tPE_IMAGE_HEADERS;

typedef struct {
	Uint16	Ident;
	Uint16	Resvd[29];
	Uint32	PeHdrOffs;		// File address of new exe header
} tPE_DOS_HEADER;

#endif

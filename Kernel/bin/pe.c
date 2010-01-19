/*
 * Acess v1
 * Portable Executable Loader
 */
#define DEBUG	1
#include <acess.h>
#include <binary.h>
#include <modules.h>
#include "pe.h"

// === PROTOTYPES ===
 int	PE_Install(char **Arguments);
tBinary	*PE_Load(int fp);
tBinary	*MZ_Open(int fp);
 int	PE_Relocate(void *Base);
 int	PE_GetSymbol(void *Base, char *Name, Uint *Ret);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, BinPE, PE_Install, NULL, NULL);
char	*gsPE_DefaultInterpreter = "/Acess/Libs/ld-acess.so";
tBinaryType	gPE_Loader = {
	NULL,
	('M'|('Z'<<8)), 0xFFFF,	// 'MZ'
	"PE/DOS",
	PE_Load, PE_Relocate, PE_GetSymbol
	};

// === CODE ===
int PE_Install(char **Arguments)
{
	Binary_RegisterType(&gPE_Loader);
	return 1;
}

/**
 * \brief Loads a PE Binary
 */
tBinary *PE_Load(int FP)
{
	 int	count, i, j, k;
	 int	iPageCount;
	tBinary	*ret;
	tPE_DOS_HEADER		dosHdr;
	tPE_IMAGE_HEADERS	peHeaders;
	tPE_SECTION_HEADER	*peSections;
	char	namebuf[9] = {0};
	Uint	iFlags, iVA;
	
	ENTER("xFP", FP);
	
	// Read DOS header and check
	VFS_Read(FP, sizeof(tPE_DOS_HEADER), &dosHdr);
	if( dosHdr.Ident != ('M'|('Z'<<8)) ) {
		LEAVE('n');
		return NULL;
	}
	
	// - Read PE Header
	VFS_Seek(FP, dosHdr.PeHdrOffs, SEEK_SET);
	if( VFS_Tell(FP) != dosHdr.PeHdrOffs ) {
		ret = MZ_Open(FP);
		LEAVE('p', ret);
		return ret;
	}
	VFS_Read(FP, sizeof(tPE_IMAGE_HEADERS), &peHeaders);
	
	// - Check PE Signature and pass on to the MZ Loader if invalid
	if( peHeaders.Signature != (('P')|('E'<<8)) ) {
		ret = MZ_Open(FP);
		LEAVE('p', ret);
		return ret;
	}
	
	// Read Sections (Uses `count` as a temp variable)
	count = sizeof(tPE_SECTION_HEADER) * peHeaders.FileHeader.SectionCount;
	peSections = malloc( count );
	if(!peSections)
	{
		Warning("PE_Load - Unable to allocate `peSections`, 0x%x bytes", count);
		LEAVE('n');
		return NULL;
	}
	VFS_Read(FP, count, peSections);
	
	// Count Pages
	iPageCount = 1;	// 1st page is headers
	for( i = 0; i < peHeaders.FileHeader.SectionCount; i++ )
	{
		// Check if the section is loadable
		// (VA is zero in non-loadable sections)
		if(peSections[i].RVA + peHeaders.OptHeader.ImageBase == 0)		continue;
		
		// Moar pages
		iPageCount += (peSections[i].VirtualSize + 0xFFF) >> 12;
	}
	
	LOG("%i Pages", iPageCount);
	
	// Initialise Executable Information
	ret = malloc(sizeof(tBinary) + sizeof(tBinaryPage)*iPageCount);
	
	ret->Entry = peHeaders.OptHeader.EntryPoint + peHeaders.OptHeader.ImageBase;
	ret->Base = peHeaders.OptHeader.ImageBase;
	ret->Interpreter = gsPE_DefaultInterpreter;
	ret->NumPages = iPageCount;
	
	LOG("Entry=%p, BaseAddress=0x%x\n", ret->Entry, ret->Base);
	
	ret->Pages[0].Virtual = peHeaders.OptHeader.ImageBase;
	ret->Pages[0].Physical = 0;
	ret->Pages[0].Size = 4096;
	ret->Pages[0].Flags = 0;
	
	// Parse Sections
	j = 1;	// Page Index
	for( i = 0; i < peHeaders.FileHeader.SectionCount; i++ )
	{
		iVA = peSections[i].RVA + peHeaders.OptHeader.ImageBase;
		
		// Skip non-loadable sections
		if(iVA == 0)	continue;
		
		// Create Name Buffer
		memcpy(namebuf, peSections[i].Name, 8);
		LOG("Section %i '%s', iVA = %p", i, namebuf, iVA);
		
		// Create Flags
		iFlags = 0;
		if(peSections[i].Flags & PE_SECTION_FLAG_MEM_EXECUTE)
			iFlags |= BIN_PAGEFLAG_EXEC;
		if( !(peSections[i].Flags & PE_SECTION_FLAG_MEM_WRITE) )
			iFlags |= BIN_PAGEFLAG_RO;
		
		// Create Page Listing
		count = (peSections[i].RawSize + 0xFFF) >> 12;
		for(k=0;k<count;k++)
		{
			ret->Pages[j+k].Virtual = iVA + (k<<12);
			ret->Pages[j+k].Physical = peSections[i].RawOffs + (k<<12);	// Store the offset in the physical address
			if(k == count-1 && (peSections[i].RawSize & 0xFFF))
				ret->Pages[j+k].Size = peSections[i].RawSize & 0xFFF;	// Byte count in page
			else
				ret->Pages[j+k].Size = 4096;
			ret->Pages[j+k].Flags = iFlags;
		}
		count = (peSections[i].VirtualSize + 0xFFF) >> 12;
		for(;k<count;k++)
		{
			ret->Pages[j+k].Virtual = iVA + (k<<12);
			ret->Pages[j+k].Physical = -1;	// -1 = Fill with zeros
			if(k == count-1 && (peSections[i].VirtualSize & 0xFFF))
				ret->Pages[j+k].Size = peSections[i].VirtualSize & 0xFFF;	// Byte count in page
			else
				ret->Pages[j+k].Size = 4096;
			ret->Pages[j+k].Flags = iFlags;
		}
		j += count;
		
		LOG("%i Name:'%s', RVA: 0x%x, Size: 0x%x (0x%x), Ofs: 0x%x, Flags: 0x%x",
			i, namebuf, 
			iVA,
			peSections[i].VirtualSize, peSections[i].RawSize, peSections[i].RawOffs,
			peSections[i].Flags
			);
		
	}
	// Free Executable Memory
	free(peSections);
	
	LEAVE('p', ret);
	return ret;
}

/**
 */
tBinary *MZ_Open(int FP)
{
	ENTER("xFP", FP);
	UNIMPLEMENTED();
	LEAVE('n');
	return NULL;
}

int PE_Relocate(void *Base)
{
	tPE_DOS_HEADER		*dosHdr;
	tPE_IMAGE_HEADERS	*peHeaders;
	tPE_SECTION_HEADER	*peSections;
	tPE_DATA_DIR	*directory;
	tPE_IMPORT_DIR	*impDir;
	 int	i;
	Uint	iBase = (Uint)Base;
	#if 0
	void	*hLibrary;
	char	*libPath;
	#endif
	
	ENTER("pBase", Base);
	dosHdr = Base;
	peHeaders = (void*)( iBase + dosHdr->PeHdrOffs );
	LOG("Prefered Base %p", peHeaders->OptHeader.ImageBase);
	peSections = (void*)( iBase + sizeof(tPE_IMAGE_HEADERS) );
	
	directory = (void*)(peSections[0].RVA + iBase);
	
	// === Load Import Tables
	impDir = (void*)( directory[PE_DIR_IMPORT].RVA + iBase );
	for( i = 0; impDir[i].DLLName != NULL; i++ )
	{
		impDir[i].DLLName += iBase;
		impDir[i].ImportLookupTable += iBase/4;
		impDir[i].ImportAddressTable += iBase/4;
		LOG("DLL Required '%s'(0x%x)", impDir[i].DLLName, impDir[i].DLLName);
		#if 0
		libPath = FindLibrary(impDir[i].DLLName);
		if(libPath == NULL)
		{
			Warning("PE_Relocate - Unable to find library '%s'");
			LEAVE('i', -1);
			return -1;
		}
		LOG("DLL Path = '%s'", libPath);
		hLibrary = DynLib_Load(libPath, 0);
		#endif
	}
	
	for(i=0;i<PE_DIR_LAST;i++)
		LOG("directory[%i] = {RVA=0x%x,Size=0x%x}", i, directory[i].RVA, directory[i].Size);
	
	LEAVE('i', 0);
	return 0;
}

int PE_GetSymbol(void *Base, char *Name, Uint *Ret)
{
	return 0;
}

/*
 * Acess v1
 * Portable Executable Loader
 */
#include "common.h"
#include "pe.h"

#define PE_DEBUG	0
#define DEBUG	(ACESS_DEBUG|PE_DEBUG)

//#define DLL_BASE_PATH	"/Acess/Windows/"
#define DLL_BASE_PATH	NULL

#if DEBUG
# define DEBUGS(v...)	SysDebug(v)
#else
# define DEBUGS(v...)
#endif

// === PROTOTYPES ===
 int	PE_Relocate(void *Base, char **envp, char *Filename);
char	*PE_int_GetTrueFile(char *file);
 int	PE_int_GetForwardSymbol(char *Fwd, Uint *Value);

// === CODE ===
int PE_Relocate(void *Base, char *envp[], char *Filename)
{
	tPE_DOS_HEADER		*dosHdr = Base;
	tPE_IMAGE_HEADERS	*peHeaders;
	tPE_DATA_DIR	*directory;
	tPE_IMPORT_DIR	*impDir;
	tPE_HINT_NAME	*name;
	Uint32	*importTab, *aIAT;
	 int	i, j;
	Uint	iBase = (Uint)Base;
	Uint	iLibBase;
	
	DEBUGS("PE_Relocate: (Base=0x%x)\n", Base);
	
	peHeaders = Base + dosHdr->PeHdrOffs;
	
	directory = peHeaders->OptHeader.Directory;
	
	// === Load Import Tables
	impDir = Base + directory[PE_DIR_IMPORT].RVA;
	for( i = 0; impDir[i].DLLName != NULL; i++ )
	{
		impDir[i].DLLName += iBase;
		impDir[i].ImportLookupTable += iBase/4;
		impDir[i].ImportAddressTable += iBase/4;
		DEBUGS(" PE_Relocate: DLL Required '%s'(0x%x)\n", impDir[i].DLLName, impDir[i].DLLName);
		iLibBase = LoadLibrary(PE_int_GetTrueFile(impDir[i].DLLName), DLL_BASE_PATH, envp);
		if(iLibBase == 0) {
			SysDebug("Unable to load required library '%s'\n", impDir[i].DLLName);
			return 0;
		}
		DEBUGS(" PE_Relocate: Loaded as 0x%x\n", iLibBase);
		importTab = impDir[i].ImportLookupTable;
		aIAT = impDir[i].ImportAddressTable;
		for( j = 0; importTab[j] != 0; j++ )
		{
			if( importTab[j] & 0x80000000 )
				DEBUGS(" PE_Relocate: Import Ordinal %i\n", importTab[j] & 0x7FFFFFFF);
			else
			{
				name = (void*)( iBase + importTab[j] );
				DEBUGS(" PE_Relocate: Import Name '%s', Hint 0x%x\n", name->Name, name->Hint);
				if( GetSymbolFromBase(iLibBase, name->Name, (Uint*)&aIAT[j]) == 0 ) {
					SysDebug("Unable to find symbol '%s' in library '%s'\n", name->Name, impDir[i].DLLName);
					return 0;
				}
			}
		}
	}
	
	#if DEBUG
	for(i=0;i<PE_DIR_LAST;i++) {
		if(directory[i].RVA == 0)	continue;
		SysDebug("directory[%i] = {RVA=0x%x,Size=0x%x}\n", i, directory[i].RVA, directory[i].Size);
	}
	#endif
	
	DEBUGS("PE_Relocate: RETURN 0x%x\n", iBase + peHeaders->OptHeader.EntryPoint);
	
	return iBase + peHeaders->OptHeader.EntryPoint;
}

/**
 * \fn int PE_GetSymbol(Uint Base, char *Name, Uint *Ret)
 */
int PE_GetSymbol(Uint Base, char *Name, Uint *Ret)
{
	tPE_DOS_HEADER		*dosHdr = (void*)Base;
	tPE_IMAGE_HEADERS	*peHeaders;
	tPE_DATA_DIR	*directory;
	tPE_EXPORT_DIR	*expDir;
	Uint32	*nameTable, *addrTable;
	Uint16	*ordTable;
	 int	i;
	 int	symbolCount;
	char	*name;
	Uint	retVal;
	Uint	expLen;
	
	peHeaders = (void*)( Base + dosHdr->PeHdrOffs );
	directory = peHeaders->OptHeader.Directory;
	
	expDir = (void*)( Base + directory[PE_DIR_EXPORT].RVA );
	expLen = directory[PE_DIR_EXPORT].Size;
	symbolCount = expDir->NumNamePointers;
	nameTable = (void*)( Base + expDir->NamePointerRVA );
	ordTable = (void*)( Base + expDir->OrdinalTableRVA );
	addrTable = (void*)( Base + expDir->AddressRVA );
	//DEBUGS(" PE_GetSymbol: %i Symbols\n", symbolCount);
	for(i=0;i<symbolCount;i++)
	{
		name = (char*)( Base + nameTable[i] );
		//DEBUGS(" PE_GetSymbol: '%s' = 0x%x\n", name, Base + addrTable[ ordTable[i] ]);
		if(strcmp(name, Name) == 0)
		{
			retVal = Base + addrTable[ ordTable[i] ];
			// Check for forwarding
			if((Uint)expDir < retVal && retVal < (Uint)expDir + expLen) {
				char *fwd = (char*)retVal;
				DEBUGS(" PE_GetSymbol: '%s' forwards to '%s'\n", name, fwd);
				return PE_int_GetForwardSymbol(fwd, Ret);
			}
			*Ret = retVal;
			return 1;
		}
	}
	return 0;
}

/**
 * \fn char *PE_int_GetTrueFile(char *file)
 * \brief Get the true file name
 */
char *PE_int_GetTrueFile(char *file)
{
	 int	i;
	if(file[0] != ':')	return file;

	// Translate name
	for(i=1;file[i];i++)
		if(file[i] == '_')
			file[i] = '.';
	
	return &file[1];
}

int PE_int_GetForwardSymbol(char *Fwd, Uint *Value)
{
	char	*libname;
	char	*sym;
	 int	i;
	Uint	libbase;
	 int	ret;
	
	// -- Find seperator
	// PE spec says that there sould only be one, but the string may
	// be mutilated in the loader
	for(i=0;Fwd[i]!='\0';i++);
	for(;i&&Fwd[i]!='.';i--);
	
	Fwd[i] = '\0';
	sym = &Fwd[i+1];
	
	libname = PE_int_GetTrueFile(Fwd);
	
	DEBUGS(" PE_int_GetForwardSymbol: Get '%s' from '%s'\n", sym, libname);
	
	libbase = LoadLibrary(libname, DLL_BASE_PATH, NULL);
	ret = GetSymbolFromBase(libbase, sym, Value);
	if(!ret) {
		SysDebug(" PE_int_GetForwardSymbol: Unable to find '%s' in '%s'\n", sym, libname);
	}
	Fwd[i] = '.';
	return ret;
}

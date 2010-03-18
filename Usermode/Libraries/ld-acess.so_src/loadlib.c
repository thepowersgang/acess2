/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"

#define DEBUG	0

#if DEBUG
# define DEBUGS(v...)	SysDebug(v)
#else
# define DEBUGS(v...)	
#endif

// === CONSTANTS ===
#define	MAX_LOADED_LIBRARIES	64
#define	MAX_STRINGS_BYTES	4096
#define	SYSTEM_LIB_DIR	"/Acess/Libs/"

// === PROTOTYPES ===
Uint	IsFileLoaded(char *file);
 int	GetSymbolFromBase(Uint base, char *name, Uint *ret);

// === GLOABLS ===
struct {
	Uint	Base;
	char	*Name;
}	gLoadedLibraries[MAX_LOADED_LIBRARIES];
char	gsLoadedStrings[MAX_STRINGS_BYTES];
char	*gsNextAvailString = gsLoadedStrings;
//tLoadLib	*gpLoadedLibraries = NULL;

// === CODE ===
char *FindLibrary(char *DestBuf, char *SoName, char *ExtraSearchDir)
{	
	// -- #1: Executable Specified
	if(ExtraSearchDir)
	{
		strcpy(DestBuf, ExtraSearchDir);
		strcat(DestBuf, "/");
		strcat(DestBuf, SoName);
		if(file_exists(DestBuf))	return DestBuf;
	}
	
	// -- #2: System
	strcpy(DestBuf, SYSTEM_LIB_DIR);
	strcat(DestBuf, SoName);
	if(file_exists(DestBuf))	return DestBuf;
	
	// -- #3: Current Directory
	if(file_exists(SoName))	return SoName;
	
	return NULL;
}

/**
 */
Uint LoadLibrary(char *SoName, char *SearchDir, char **envp)
{
	char	sTmpName[1024];
	char	*filename;
	Uint	iArg;
	void	(*fEntry)(int, int, char *[], char**);
	
	DEBUGS("LoadLibrary: (filename='%s', envp=0x%x)\n", filename, envp);
	
	// Create Temp Name
	filename = FindLibrary(sTmpName, SoName, SearchDir);
	if(filename == NULL) {
		DEBUGS("LoadLibrary: RETURN 0\n");
		return 0;
	}
	DEBUGS(" LoadLibrary: filename='%s'\n", filename);
	
	if( (iArg = IsFileLoaded(filename)) )
		return iArg;
	
	// Load Library
	iArg = SysLoadBin(filename, (Uint*)&fEntry);
	if(iArg == 0) {
		DEBUGS("LoadLibrary: RETURN 0\n");
		return 0;
	}
	
	DEBUGS(" LoadLibrary: iArg=0x%x, iEntry=0x%x\n", iArg, fEntry);
	
	// Load Symbols
	fEntry = (void*)DoRelocate( iArg, envp, filename );
	
	// Call Entrypoint
	DEBUGS(" LoadLibrary: '%s' Entry 0x%x\n", SoName, fEntry);
	fEntry(iArg, 0, NULL, envp);
	
	DEBUGS("LoadLibrary: RETURN 1\n");
	return iArg;
}

/**
 * \fn Uint IsFileLoaded(char *file)
 * \brief Determine if a file is already loaded
 */
Uint IsFileLoaded(char *file)
{
	 int	i;
	DEBUGS("IsFileLoaded: (file='%s')", file);
	for( i = 0; i < MAX_LOADED_LIBRARIES; i++ )
	{
		if(gLoadedLibraries[i].Base == 0)	break;	// Last entry has Base set to NULL
		DEBUGS(" strcmp('%s', '%s')", gLoadedLibraries[i].Name, file);
		if(strcmp(gLoadedLibraries[i].Name, file) == 0) {
			DEBUGS("IsFileLoaded: Found %i (0x%x)", i, gLoadedLibraries[i].Base);
			return gLoadedLibraries[i].Base;
		}
	}
	DEBUGS("IsFileLoaded: Not Found");
	return 0;
}

/**
 * \fn void AddLoaded(char *File, Uint base)
 * \brief Add a file to the loaded list
 */
void AddLoaded(char *File, Uint base)
{
	 int	i, length;
	char	*name = gsNextAvailString;
	
	DEBUGS("AddLoaded: (File='%s', base=0x%x)", File, base);
	
	// Find a free slot
	for( i = 0; i < MAX_LOADED_LIBRARIES; i ++ )
	{
		if(gLoadedLibraries[i].Base == 0)	break;
	}
	if(i == MAX_LOADED_LIBRARIES) {
		SysDebug("ERROR - ld-acess.so has run out of load slots!");
		return;
	}
	
	// Check space in string buffer
	length = strlen(File);
	if(&name[length+1] >= &gsLoadedStrings[MAX_STRINGS_BYTES]) {
		SysDebug("ERROR - ld-acess.so has run out of string buffer memory!");
		return;
	}
	
	// Set information
	gLoadedLibraries[i].Base = base;
	strcpy(name, File);
	gLoadedLibraries[i].Name = name;
	gsNextAvailString = &name[length+1];
	DEBUGS("'%s' (0x%x) loaded as %i\n", name, base, i);
	return;
}

/**
 * \fn void Unload(Uint Base)
 */
void Unload(Uint Base)
{	
	 int	i, j;
	 int	id;
	char	*str;
	for( id = 0; id < MAX_LOADED_LIBRARIES; id++ )
	{
		if(gLoadedLibraries[id].Base == Base)	break;
	}
	if(id == MAX_LOADED_LIBRARIES)	return;
	
	// Unload Binary
	SysUnloadBin( Base );
	// Save String Pointer
	str = gLoadedLibraries[id].Name;
	
	// Compact Loaded List
	j = id;
	for( i = j + 1; i < MAX_LOADED_LIBRARIES; i++, j++ )
	{
		if(gLoadedLibraries[i].Base == 0)	break;
		// Compact String
		strcpy(str, gLoadedLibraries[i].Name);
		str += strlen(str)+1;
		// Compact Entry
		gLoadedLibraries[j].Base = gLoadedLibraries[i].Base;
		gLoadedLibraries[j].Name = str;
	}
	
	// NULL Last Entry
	gLoadedLibraries[j].Base = 0;
	gLoadedLibraries[j].Name = NULL;
	// Save next string
	gsNextAvailString = str;
}

/**
 \fn Uint GetSymbol(char *name)
 \brief Gets a symbol value from a loaded library
*/
Uint GetSymbol(char *name)
{
	 int	i;
	Uint	ret;
	for(i=0;i<sizeof(gLoadedLibraries)/sizeof(gLoadedLibraries[0]);i++)
	{
		if(gLoadedLibraries[i].Base == 0)	break;
		
		//SysDebug(" GetSymbol: Trying 0x%x, '%s'\n",
		//	gLoadedLibraries[i].Base, gLoadedLibraries[i].Name);
		if(GetSymbolFromBase(gLoadedLibraries[i].Base, name, &ret))	return ret;
	}
	SysDebug("GetSymbol: === Symbol '%s' not found ===\n", name);
	return 0;
}

/**
 \fn int GetSymbolFromBase(Uint base, char *name, Uint *ret)
 \breif Gets a symbol from a specified library
*/
int GetSymbolFromBase(Uint base, char *name, Uint *ret)
{
	if(*(Uint32*)base == (0x7F|('E'<<8)|('L'<<16)|('F'<<24)))
		return ElfGetSymbol(base, name, ret);
	if(*(Uint16*)base == ('M'|('Z'<<8)))
		return PE_GetSymbol(base, name, ret);
	return 0;
}


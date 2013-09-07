/*
 AcessOS 1 - Dynamic Loader
 By thePowersGang
*/
#include "common.h"
#include <stdint.h>
#include <acess/sys.h>

#define DEBUG	0

#if DEBUG
# define DEBUGS(v...)	SysDebug(v)
#else
# define DEBUGS(v...)	
#endif

// === PROTOTYPES ===
void	*IsFileLoaded(const char *file);

// === IMPORTS ===
extern const struct {
	void	*Value;
	char	*Name;
}	caLocalExports[];
extern const int	ciNumLocalExports;
extern char	**gEnvP;
extern char	gLinkedBase[];

// === GLOABLS ===
tLoadedLib	gLoadedLibraries[MAX_LOADED_LIBRARIES];
char	gsLoadedStrings[MAX_STRINGS_BYTES];
char	*gsNextAvailString = gsLoadedStrings;
//tLoadLib	*gpLoadedLibraries = NULL;

// === CODE ===
void ldacess_DumpLoadedLibraries(void)
{
	for( int i = 0; i < MAX_LOADED_LIBRARIES; i ++ )
	{
		if(gLoadedLibraries[i].Base == 0)	break;	// Last entry has Base set to NULL
		_SysDebug("%p: %s",
			gLoadedLibraries[i].Base,
			gLoadedLibraries[i].Name
			);
	}
}

const char *FindLibrary(char *DestBuf, const char *SoName, const char *ExtraSearchDir)
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
void *LoadLibrary(const char *SoName, const char *SearchDir, char **envp)
{
	char	sTmpName[1024];
	const char	*filename;
	void	*base;
	void	(*fEntry)(void *, int, char *[], char**);
	
	DEBUGS("LoadLibrary: (SoName='%s', SearchDir='%s', envp=%p)", SoName, SearchDir, envp);
	
	// Create Temp Name
	filename = FindLibrary(sTmpName, SoName, SearchDir);
	if(filename == NULL) {
		DEBUGS("LoadLibrary: RETURN 0");
		return 0;
	}
	DEBUGS(" LoadLibrary: filename='%s'", filename);
	
	if( (base = IsFileLoaded(filename)) )
		return base;

	DEBUGS(" LoadLibrary: SysLoadBin()");	
	// Load Library
	base = _SysLoadBin(filename, (void**)&fEntry);
	if(!base) {
		DEBUGS("LoadLibrary: RETURN 0");
		return 0;
	}
	
	DEBUGS(" LoadLibrary: iArg=%p, fEntry=%p", base, fEntry);
	
	// Load Symbols
	fEntry = DoRelocate( base, envp, filename );
	if( !fEntry ) {
		return 0;
	}
	
	// Call Entrypoint
	DEBUGS(" LoadLibrary: '%s' Entry %p", SoName, fEntry);
	fEntry(base, 0, NULL, gEnvP);
	
	DEBUGS("LoadLibrary: RETURN 1");
	return base;
}

/**
 * \fn Uint IsFileLoaded(char *file)
 * \brief Determine if a file is already loaded
 */
void *IsFileLoaded(const char *file)
{
	 int	i;
	DEBUGS("IsFileLoaded: (file='%s')", file);

	// Applications link against either libld-acess.so or ld-acess.so
	if( strcmp(file, "/Acess/Libs/libld-acess.so") == 0
	 || strcmp(file, "/Acess/Libs/ld-acess.so") == 0 )
	{
		DEBUGS("IsFileLoaded: Found local (%p)", &gLinkedBase);
		return &gLinkedBase;
	}

	for( i = 0; i < MAX_LOADED_LIBRARIES; i++ )
	{
		if(gLoadedLibraries[i].Base == 0)	break;	// Last entry has Base set to NULL
		DEBUGS(" strcmp('%s', '%s')", gLoadedLibraries[i].Name, file);
		if(strcmp(gLoadedLibraries[i].Name, file) == 0) {
			DEBUGS("IsFileLoaded: Found %i (%p)", i, gLoadedLibraries[i].Base);
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
void AddLoaded(const char *File, void *base)
{
	 int	i, length;
	char	*name = gsNextAvailString;
	
	DEBUGS("AddLoaded: (File='%s', base=%p)", File, base);
	
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
	DEBUGS("'%s' (%p) loaded as %i", name, base, i);
	return;
}

/**
 * \fn void Unload(Uint Base)
 */
void Unload(void *Base)
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
	_SysUnloadBin( Base );
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
 \fn Uint GetSymbol(const char *name)
 \brief Gets a symbol value from a loaded library
*/
int GetSymbol(const char *name, void **Value, size_t *Size)
{
	 int	i;
	
	//SysDebug("ciNumLocalExports = %i", ciNumLocalExports);
	for(i=0;i<ciNumLocalExports;i++)
	{
		if( strcmp(caLocalExports[i].Name, name) == 0 ) {
			*Value = caLocalExports[i].Value;
			if(Size)
				*Size = 0;
			return 1;
		}
	}
	
	// Entry 0 is ld-acess, ignore it
	for(i = 0; i < MAX_LOADED_LIBRARIES; i ++)
	{
		if(gLoadedLibraries[i].Base == 0)
			break;
		
		//SysDebug(" GetSymbol: Trying 0x%x, '%s'",
		//	gLoadedLibraries[i].Base, gLoadedLibraries[i].Name);
		if(GetSymbolFromBase(gLoadedLibraries[i].Base, name, Value, Size))
			return 1;
	}
	SysDebug("GetSymbol: === Symbol '%s' not found ===", name);
	return 0;
}

/**
 \fn int GetSymbolFromBase(Uint base, char *name, Uint *ret)
 \breif Gets a symbol from a specified library
*/
int GetSymbolFromBase(void *base, const char *name, void **ret, size_t *Size)
{
	uint8_t	*hdr = base;
	if(hdr[0] == 0x7F && hdr[1] == 'E' && hdr[2] == 'L' && hdr[3] == 'F')
		return ElfGetSymbol(base, name, ret, Size);
	if(hdr[0] == 'M' && hdr[1] == 'Z')
		return PE_GetSymbol(base, name, ret, Size);
	SysDebug("Unknown type at %p (%02x %02x %02x %02x)", base,
		hdr[0], hdr[1], hdr[2], hdr[3]);
	return 0;
}


/*
 * AcessNative
 */
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LIBRARY_PATH	"$$$$../Usermode/Output/i386/Libs"

// === TYPES ===
typedef struct sBinary {
	struct sBinary	*Next;
	void	*Base;
	 int	Ready;
	tBinFmt	*Format;
	char	Path[];
}	tBinary;

// === IMPORTS ===
extern void	*Elf_Load(int fd);
extern uintptr_t	Elf_Relocate(void *Base);
extern int	Elf_GetSymbol(void *Base, char *Name, uintptr_t *ret);
extern int	ciNumBuiltinSymbols;
extern tSym	caBuiltinSymbols[];

// === PROTOTYPES ===
void	Binary_AddToList(const char *Filename, void *Base, tBinFmt *Format);

// === GLOBALS ===
tBinFmt	gElf_FormatDef = {
//	.Mask = 0xFFFFFFFF,
//	.Magic = "\x7F""ELF",
	.Name = "ELF32",
	.Load = Elf_Load,
	.Relocate = Elf_Relocate,
	.GetSymbol = Elf_GetSymbol
	};
tBinary	*gLoadedBinaries;

// === CODE ===
char *Binary_LocateLibrary(const char *Name)
{
	char	*envPath = getenv("ACESS_LIBRARY_PATH");
	 int	nameLen = strlen(Name);
	 int	fd;
	
	if( strcmp(Name, "libld-acess.so") == 0 ) {
		return strdup("libld-acess.so");
	}
	
	// Try the environment ACESS_LIBRARY_PATH
	if( envPath && envPath[0] != '\0' )
	{
		 int	len = strlen(envPath)+1+nameLen+1;
		char	tmp[len];

		strcpy(tmp, envPath);
		strcat(tmp, "/");
		strcat(tmp, Name);
		
		fd = acess_open(tmp, 4);	// OPENFLAG_EXEC
		if(fd != -1) {
			acess_close(fd);
			return strdup(tmp);
		}
	}		

	{
		 int	len = strlen(LIBRARY_PATH)+1+nameLen+1;
		char	tmp[len];

		strcpy(tmp, LIBRARY_PATH);
		strcat(tmp, "/");
		strcat(tmp, Name);
		
		#if DEBUG
		printf("Binary_LocateLibrary: tmp = '%s'\n", tmp);
		#endif

		fd = acess_open(tmp, 4);	// OPENFLAG_EXEC
		if(fd != -1) {
			acess_close(fd);
			return strdup(tmp);
		}
	}		

	#if DEBUG
	fprintf(stderr, "Unable to locate library '%s'\n", Name);
	#endif

	return NULL;
}

void *Binary_LoadLibrary(const char *Name)
{
	char	*path;
	void	*ret;
	 int	(*entry)(int,char*[],char**) = NULL;

	// Find File
	path = Binary_LocateLibrary(Name);
	#if DEBUG
	printf("Binary_LoadLibrary: path = '%s'\n", path);
	#endif
	if( !path ) {
		return NULL;
	}

	ret = Binary_Load(path, (uintptr_t*)&entry);
	free(path);
	
	#if DEBUG
	printf("Binary_LoadLibrary: ret = %p, entry = %p\n", ret, entry);
	#endif
	if( entry ) {
		char	*argv[] = {NULL};
		#if DEBUG
		printf("Calling '%s' entry point %p\n", Name, entry);
		#endif
		entry(0, argv, NULL);
	}

	return ret;
}

void *Binary_Load(const char *Filename, uintptr_t *EntryPoint)
{
	 int	fd;
	uint32_t	dword = 0xFA17FA17;
	void	*ret;
	uintptr_t	entry = 0;
	tBinFmt	*fmt;

	// Ignore loading ld-acess
	if( strcmp(Filename, "libld-acess.so") == 0 ) {
		*EntryPoint = 0;
		return (void*)-1;
	}

	{
		tBinary	*bin;
		for(bin = gLoadedBinaries; bin; bin = bin->Next)
		{
			if( strcmp(Filename, bin->Path) == 0 ) {
				return bin->Base;
			}
		}
	}

	fd = acess_open(Filename, 2|1);	// Execute and Read
	if( fd == -1 ) {
		// TODO: Handle libary directories
		perror("Opening binary");
		return NULL;
	}

	acess_read(fd, 4, &dword);
	acess_seek(fd, 0, ACESS_SEEK_SET);
	
	if( memcmp(&dword, "\x7F""ELF", 4) == 0 ) {
		fmt = &gElf_FormatDef;
	}
	else {
		fprintf(stderr, "Unknown executable format (0x%08x)\n", dword);
		acess_close(fd);
		return NULL;
	}
	
	#if DEBUG
	printf("fmt->Load(%i)...\n", fd);
	#endif
	ret = fmt->Load(fd);
	acess_close(fd);
	#if DEBUG
	printf("fmt->Load(%p): %p\n", fd, ret);
	#endif
	if( !ret ) {
		return NULL;
	}
	
	Binary_AddToList(Filename, ret, fmt);

	entry = fmt->Relocate(ret);
	#if DEBUG
	printf("fmt->Relocate(%p): %p\n", ret, (void*)entry);
	#endif
	if( !entry ) {
		// TODO: Clean up
		return NULL;
	}
	
	if( EntryPoint )
		*EntryPoint = entry;

	Binary_SetReadyToUse(ret);

	return ret;
}

void Binary_AddToList(const char *Filename, void *Base, tBinFmt *Format)
{
	tBinary	*bin = malloc(sizeof(tBinary) + strlen(Filename) + 1);
	bin->Base = Base;
	bin->Format = Format;
	strcpy(bin->Path, Filename);
	bin->Ready = 0;
	
	bin->Next = gLoadedBinaries;
	gLoadedBinaries = bin;
}

void Binary_SetReadyToUse(void *Base)
{
	tBinary	*bin;
	for(bin = gLoadedBinaries; bin; bin = bin->Next)
	{
		if( bin->Base != Base )	continue ;
		bin->Ready = 1;
	}
}

int Binary_GetSymbol(const char *SymbolName, uintptr_t *Value)
{
	 int	i;
	tBinary	*bin;
	
	//printf("Binary_GetSymbol: (SymbolName='%s', Value=%p)\n",
	//	SymbolName, Value);

	// Search builtins
	// - Placed first to override smartarses that define their own versions
	//   of system calls
	for( i = 0; i < ciNumBuiltinSymbols; i ++ )
	{
		if( strcmp(caBuiltinSymbols[i].Name, SymbolName) == 0 ) {
			*Value = (uintptr_t)caBuiltinSymbols[i].Value;
			return 1;
		}
	}
	
	// Search list of loaded binaries
	for(bin = gLoadedBinaries; bin; bin = bin->Next)
	{
		if( !bin->Ready )	continue;
		//printf(" Binary_GetSymbol: bin = %p{%p, %s}\n", bin, bin->Base, bin->Path);
		if( bin->Format->GetSymbol(bin->Base, (char*)SymbolName, Value) )
			return 1;
	}

	//printf("Binary_GetSymbol: RETURN 0, not found\n");
	
	return 0;
}

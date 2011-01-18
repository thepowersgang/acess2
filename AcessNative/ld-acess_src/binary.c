/*
 * AcessNative
 */
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LIBRARY_PATH	"../Usermode/Output/i386/Libs"

// === TYPES ===
typedef struct sBinary {
	struct sBinary	*Next;
	void	*Base;
	 int	Ready;
	tBinFmt	*Format;
	char	Path[];
}	tBinary;

// === IMPORTS ===
extern void	*Elf_Load(FILE *FP);
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
	FILE	*fp;
	
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
		
		fp = fopen(tmp, "r");
		if(fp) {
			fclose(fp);
			return strdup(tmp);
		}
	}		

	{
		 int	len = strlen(LIBRARY_PATH)+1+nameLen+1;
		char	tmp[len];

		strcpy(tmp, LIBRARY_PATH);
		strcat(tmp, "/");
		strcat(tmp, Name);
		
		printf("Binary_LocateLibrary: tmp = '%s'\n", tmp);

		fp = fopen(tmp, "r");
		if(fp) {
			fclose(fp);
			return strdup(tmp);
		}
	}		

	fprintf(stderr, "Unable to locate library '%s'\n", Name);

	return NULL;
}

void *Binary_LoadLibrary(const char *Name)
{
	char	*path;
	void	*ret;
	 int	(*entry)(int,char*[],char**) = NULL;

	// Find File
	path = Binary_LocateLibrary(Name);
	printf("Binary_LoadLibrary: path = '%s'\n", path);
	if( !path ) {
		return NULL;
	}

	ret = Binary_Load(path, (uintptr_t*)&entry);
	free(path);
	
	printf("Binary_LoadLibrary: ret = %p, entry = %p\n", ret, entry);
	if( entry ) {
		char	*argv[] = {NULL};
		printf("Calling '%s' entry point %p\n", Name, entry);
		entry(0, argv, NULL);
	}

	return ret;
}

void *Binary_Load(const char *Filename, uintptr_t *EntryPoint)
{
	FILE	*fp;
	uint32_t	dword;
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

	fp = fopen(Filename, "r");
	if( !fp ) {
		// TODO: Handle libary directories
		perror("Opening binary");
		return NULL;
	}

	fread(&dword, 1, 4, fp);
	fseek(fp, 0, SEEK_SET);
	printf("dword = %08x\n", dword);
	
	if( memcmp(&dword, "\x7F""ELF", 4) == 0 ) {
		fmt = &gElf_FormatDef;
	}
	else {
		fclose(fp);
		return NULL;
	}
	
	printf("fmt->Load(%p)...\n", fp);
	ret = fmt->Load(fp);
	printf("fmt->Load(%p): %p\n", fp, ret);
	if( !ret ) {
		fclose(fp);
		return NULL;
	}
	
	Binary_AddToList(Filename, ret, fmt);

	entry = fmt->Relocate(ret);
	printf("fmt->Relocate(%p): %p\n", ret, (void*)entry);
	if( !entry ) {
		// TODO: Clean up
		return NULL;
	}
	
	if( EntryPoint )
		*EntryPoint = entry;

	fclose(fp);

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
	
	printf("Binary_GetSymbol: (SymbolName='%s', Value=%p)\n",
		SymbolName, Value);

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
	
	// TODO: Search list of loaded binaries
	for(bin = gLoadedBinaries; bin; bin = bin->Next)
	{
		if( !bin->Ready )	continue;
		printf(" Binary_GetSymbol: bin = %p{%p, %s}\n", bin, bin->Base, bin->Path);
		if( bin->Format->GetSymbol(bin->Base, (char*)SymbolName, Value) )
			return 1;
	}

	printf("Binary_GetSymbol: RETURN 0, not found\n");
	
	return 0;
}

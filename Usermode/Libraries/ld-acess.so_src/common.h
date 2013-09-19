/*
 AcessOS v1
 By thePowersGang
 ld-acess.so
 COMMON.H
*/
#ifndef _COMMON_H
#define _COMMON_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <acess/sys.h>
#include <assert.h>

typedef	uintptr_t	Uint;
typedef uint8_t 	Uint8;
typedef uint16_t	Uint16;
typedef uint32_t	Uint32;

// HACK: Replace with underscored
#define SysDebug	_SysDebug

// === CONSTANTS ===
#define	MAX_LOADED_LIBRARIES	64
#define	MAX_STRINGS_BYTES	4096
#define	SYSTEM_LIB_DIR	"/Acess/Libs/"

// === Types ===
typedef struct {
	void	*Base;
	char	*Name;
}	tLoadedLib;

// === GLOBALS ===
extern tLoadedLib	gLoadedLibraries[MAX_LOADED_LIBRARIES];

// === Main ===
extern void	*DoRelocate(void *Base, char **envp, const char *Filename);

// === Library/Symbol Manipulation ==
extern void	*LoadLibrary(const char *Filename, const char *SearchDir, char **envp);
extern void	AddLoaded(const char *File, void *base);
extern int	GetSymbol(const char *Name, void **Value, size_t *size);
extern int	GetSymbolFromBase(void *base, const char *name, void **ret, size_t *size);

// === Library Functions ===
extern char	*strcpy(char *dest, const char *src);
extern char	*strcat(char *dest, const char *src);
extern int	strcmp(const char *s1, const char *s2);
extern int	strlen(const char *str);
extern int	file_exists(const char *filename);
extern void	*memcpy(void *dest, const void *src, size_t len);

// === System Calls ===

// === ELF Loader ===
extern void	*ElfRelocate(void *Base, char **envp, const char *Filename);
extern int	ElfGetSymbol(void *Base, const char *name, void **ret, size_t *Size);

// === PE Loader ===
extern void	*PE_Relocate(void *Base, char **envp, const char *Filename);
extern int	PE_GetSymbol(void *Base, const char *Name, void **ret, size_t *Size);

#endif

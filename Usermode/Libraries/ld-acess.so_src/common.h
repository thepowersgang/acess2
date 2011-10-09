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
extern void	*GetSymbol(const char *name);
extern int	GetSymbolFromBase(void *base, const char *name, void **ret);

// === Library Functions ===
extern char	*strcpy(char *dest, const char *src);
extern char	*strcat(char *dest, const char *src);
extern int	strcmp(const char *s1, const char *s2);
extern int	strlen(const char *str);
extern int	file_exists(const char *filename);

// === System Calls ===
extern void	_exit(int retval);
extern void	SysDebug(const char *fmt, ...);	//!< Now implemented in main.c
extern void	SysDebugV(const char *fmt, ...);
extern void	*SysLoadBin(const char *path, void **entry);
extern int	SysUnloadBin(void *Base);
extern void	SysSetFaultHandler(int (*Hanlder)(int));
extern int	open(const char *filename, int flags);
extern int	close(int fd);

// === ELF Loader ===
extern int	ElfGetSymbol(void *Base, const char *name, void **ret);

// === PE Loader ===
extern int	PE_GetSymbol(void *Base, const char *Name, void **ret);

#endif

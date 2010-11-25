/*
 AcessOS v1
 By thePowersGang
 ld-acess.so
 COMMON.H
*/
#ifndef _COMMON_H
#define _COMMON_H

#define	NULL	((void*)0)

#include <stdarg.h>

// HACK: Replace with underscored
#define SysDebug	_SysDebug

// === CONSTANTS ===
#define	MAX_LOADED_LIBRARIES	64
#define	MAX_STRINGS_BYTES	4096
#define	SYSTEM_LIB_DIR	"/Acess/Libs/"

// === Types ===
typedef unsigned int	Uint;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef signed char		Sint8;
typedef signed short	Sint16;
typedef signed long		Sint32;

typedef struct {
	Uint	Base;
	char	*Name;
}	tLoadedLib;

// === GLOBALS ===
extern tLoadedLib	gLoadedLibraries[MAX_LOADED_LIBRARIES];

// === Main ===
extern int	DoRelocate( Uint base, char **envp, char *Filename );

// === Library/Symbol Manipulation ==
extern Uint	LoadLibrary(char *filename, char *SearchDir, char **envp);
extern void	AddLoaded(char *File, Uint base);
extern Uint	GetSymbol(char *name);
extern int	GetSymbolFromBase(Uint base, char *name, Uint *ret);

// === Library Functions ===
extern char	*strcpy(char *dest, const char *src);
extern char	*strcat(char *dest, const char *src);
extern int	strcmp(const char *s1, const char *s2);
extern int	strlen(const char *str);
extern int	file_exists(char *filename);

// === System Calls ===
extern void	_exit(int retval);
extern void	SysDebug(char *fmt, ...);	//!< Now implemented in main.c
extern void	SysDebugV(char *fmt, ...);
extern Uint	SysLoadBin(char *path, Uint *entry);
extern Uint	SysUnloadBin(Uint Base);
extern void	SysSetFaultHandler(int (*Hanlder)(int));
extern int	open(char *filename, int flags);
extern void	close(int fd);

// === ELF Loader ===
extern int	ElfGetSymbol(Uint Base, char *name, Uint *ret);

// === PE Loader ===
extern int	PE_GetSymbol(Uint Base, char *Name, Uint *ret);

#endif

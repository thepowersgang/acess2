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

// === Types ===
typedef unsigned int	Uint;
typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef signed char		Sint8;
typedef signed short	Sint16;
typedef signed long		Sint32;

// === Main ===
extern int	DoRelocate( Uint base, char **envp, char *Filename );

// === Library/Symbol Manipulation ==
extern Uint	LoadLibrary(char *filename, char *SearchDir, char **envp);
extern void	AddLoaded(char *File, Uint base);
extern Uint	GetSymbol(char *name);
extern int	GetSymbolFromBase(Uint base, char *name, Uint *ret);

// === Library Functions ===
extern void	strcpy(char *dest, char *src);
extern int	strcmp(char *s1, char *s2);
extern int	strlen(char *str);

// === System Calls ===
extern void	SysExit();
extern void	SysDebug(char *fmt, ...);	//!< Now implemented in main.c
extern void	SysDebugV(char *fmt, ...);
extern Uint	SysLoadBin(char *path, Uint *entry);
extern Uint	SysUnloadBin(Uint Base);

// === ELF Loader ===
extern int	ElfGetSymbol(Uint Base, char *name, Uint *ret);

// === PE Loader ===
extern int	PE_GetSymbol(Uint Base, char *Name, Uint *ret);

#endif

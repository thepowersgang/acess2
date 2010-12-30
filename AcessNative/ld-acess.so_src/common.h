/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

extern int	Binary_GetSymbol(const char *SymbolName, uintptr_t *Value);
extern void	*Binary_LoadLibrary(const char *Path);
extern void	*Binary_Load(const char *Path, uintptr_t *EntryPoint);
extern void	Binary_SetReadyToUse(void *Base);

extern int	AllocateMemory(uintptr_t VirtAddr, size_t ByteCount);
extern uintptr_t	FindFreeRange(size_t ByteCount, int MaxBits);

extern void	Warning(const char *Format, ...);
extern void	Notice(const char *Format, ...);

typedef struct {
	char	*Name;
	void	*Value;
}	tSym;

typedef struct sBinFmt {
	struct sBinFmt	*Next;
	char	*Name;
	void	*(*Load)(FILE *fp);
	uintptr_t	(*Relocate)(void *base);
	 int	(*GetSymbol)(void*,char*,uintptr_t*);
}	tBinFmt;

#endif


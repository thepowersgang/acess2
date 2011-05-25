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

#define ACESS_SEEK_CUR	0
#define ACESS_SEEK_SET	1
#define ACESS_SEEK_END	-1

extern int	acess_open(const char *Path, int Flags);
extern void	acess_close(int FD);
extern size_t	acess_read(int FD, size_t Bytes, void *Dest);
extern int	acess_seek(int FD, int64_t Offset, int Dir);

typedef struct {
	char	*Name;
	void	*Value;
}	tSym;

typedef struct sBinFmt {
	struct sBinFmt	*Next;
	char	*Name;
	void	*(*Load)(int fd);
	uintptr_t	(*Relocate)(void *base);
	 int	(*GetSymbol)(void*,char*,uintptr_t*);
}	tBinFmt;

#endif


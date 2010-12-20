/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

extern int	Binary_GetSymbol(const char *SymbolName, intptr_t *Value);
extern int	Binary_LoadLibrary(const char *Path);

extern int	AllocateMemory(intptr_t VirtAddr, size_t ByteCount);

extern int	Warning(const char *Format, ...);
extern int	Notice(const char *Format, ...);

#endif


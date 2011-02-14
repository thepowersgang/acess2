/*
 * Acess 2
 * binary_ext.h
 * - Exported Symbols from the binary loader
 */
#ifndef _BINARY_EXT_H
#define _BINARY_EXT_H

// === FUNCTIONS ===
extern void	*Binary_LoadFile(const char *Path);
extern void	*Binary_LoadKernel(const char *Path);
extern Uint	Binary_Relocate(void *Mem);
extern void	Binary_Unload(void *Base);
extern int	Binary_GetSymbol(const char *Name, Uint *Dest);
extern Uint	Binary_FindSymbol(void *Base, const char *Name, Uint *Val);

#endif

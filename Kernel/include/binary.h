/*
 */
#ifndef _BINARY_H
#define _BINARY_H

// === TYPES ===
/**
 * \struct sBinary
 * \brief Defines a binary file
 */
typedef struct sBinary {
	struct sBinary	*Next;
	char	*TruePath;
	char	*Interpreter;
	Uint	Entry;
	Uint	Base;
	 int	NumPages;
	 int	ReferenceCount;
	struct {
		Uint	Physical;
		Uint	Virtual;
		Uint16	Size, Flags;
	}	Pages[];
} tBinary;

/**
 * \struct sBinaryType
 */
typedef struct sBinaryType {
	struct sBinaryType	*Next;
	Uint32	Ident;
	Uint32	Mask;
	char	*Name;
	tBinary	*(*Load)(int FD);
	 int	(*Relocate)(void *Base);
	 int	(*GetSymbol)(void *Base, char *Name, Uint *Dest);
} tBinaryType;

extern char	*Binary_RegInterp(char *Path);

#endif

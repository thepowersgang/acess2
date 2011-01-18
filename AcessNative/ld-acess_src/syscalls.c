/*
 */
#include "../../Usermode/include/acess/sys.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "request.h"

// === Types ===

// === IMPORTS ===

// === CODE ===
const char *ReadEntry(tOutValue **OutDest, tInValue **InDest,
	int *Direction, const char *ArgTypes, va_list Args)
{
	uint64_t	val64, *ptr64;
	uint32_t	val32, *ptr32;
	 int	direction = 0;	// 0: Invalid, 1: Out, 2: In, 3: Out
	char	*str;
	 int	len;
	
	// Eat whitespace
	while(*ArgTypes && *ArgTypes == ' ')	ArgTypes ++;
	if( *ArgTypes == '\0' )	return ArgTypes;
	
	// Get direction
	switch(*ArgTypes)
	{
	case '>':	direction = 1;	break;
	case '<':	direction = 2;	break;
	case '?':	direction = 3;	break;
	default:
		return NULL;
	}
	ArgTypes ++;
	
	// Eat whitespace
	while(*ArgTypes && *ArgTypes == ' ')	ArgTypes ++;
	if( *ArgTypes == '\0' )	return ArgTypes;
	
	// Internal helper macro
	#define MAKE_OUT(_dest,_typeChar,_typeName,_value) do{if((_dest)){\
		*(_dest) = (tOutValue*)malloc(sizeof(tOutValue)+sizeof(_typeName));\
		(*(_dest))->Type=(_typeChar);(*(_dest))->Length=sizeof(_typeName);\
		*(_typeName*)((*(_dest))->Data) = (_value);\
		}}while(0)
	#define MAKE_IN(_dest,_typeChar,_typeName,_value) do{if((_dest)){\
		*(_dest) = (tInValue*)malloc(sizeof(tInValue));\
		(*(_dest))->Type=(_typeChar);(*(_dest))->Length=sizeof(_typeName);\
		(*(_dest))->Data = (_value);\
		}}while(0)
	
	// Get type
	switch(*ArgTypes)
	{
	case 'i':	// 32-bit integer
		// Input?
		if( direction & 2 )
		{
			ptr32 = va_arg(Args, uint32_t*);
			MAKE_IN(InDest, 'i', uint32_t*, ptr32);
			if( direction & 1 )
				MAKE_OUT(OutDest, 'i', uint32_t, *ptr32);
		}
		else
		{
			val32 = va_arg(Args, uint32_t);
			MAKE_OUT(OutDest, 'i', uint32_t, val32);
		}
		break;
	case 'I':	// 64-bit integer
		// Input?
		if( direction & 2 )
		{
			ptr64 = va_arg(Args, uint64_t*);
			MAKE_IN(InDest, 'I', uint64_t*, ptr64);
			if( direction & 1 )
				MAKE_OUT(OutDest, 'I', uint64_t, *ptr64);
		}
		else
		{
			val64 = va_arg(Args, uint64_t);
			MAKE_OUT(OutDest, 'I', uint64_t, val64);
		}
		break;
	case 's':
		// Input string makes no sense!
		if( direction & 2 ) {
			fprintf(stderr, "ReadEntry: Incoming string is not defined\n");
			return NULL;
		}
		
		str = va_arg(Args, char*);
		if( OutDest )
		{
			 int	len = strlen(str) + 1;
			*OutDest = malloc( sizeof(tOutValue) + len );
			(*OutDest)->Type = 's';
			(*OutDest)->Length = len;
			memcpy((*OutDest)->Data, str, len);
		}
		break;
	
	case 'd':
		len = va_arg(Args, int);
		str = va_arg(Args, char*);
		
		// Input ?
		if( (direction & 2) && InDest )
		{
			*InDest = (tInValue*)malloc( sizeof(tInValue) );
			(*InDest)->Type = 'd';
			(*InDest)->Length = len;
			(*InDest)->Data = str;
		}
		
		// Output ?
		if( (direction & 1) && InDest )
		{
			*OutDest = (tOutValue*)malloc( sizeof(tOutValue) + len );
			(*OutDest)->Type = 'd';
			(*OutDest)->Length = len;
			memcpy((*OutDest)->Data, str, len);
		}
		break;
	
	default:
		return NULL;
	}
	ArgTypes ++;
	#undef MAKE_ASSIGN
	
	*Direction = direction;
	
	return ArgTypes;
}

/**
 * \param ArgTypes
 *
 * Whitespace is ignored
 * >i:	Input Integer (32-bits)
 * >I:	Input Long Integer (64-bits)
 * >s:	Input String
 * >d:	Input Buffer (Preceded by valid size)
 * <I:	Output long integer
 * <d:	Output Buffer (Preceded by valid size)
 * ?d:  Bi-directional buffer (Preceded by valid size), buffer contents
 *      are returned
 */
int _Syscall(const char *ArgTypes, ...)
{
	va_list	args;
	 int	outCount = 0;
	 int	inCount = 0;
	const char	*str;
	
	tOutValue	**output;
	tInValue	**input;
	
	// Get data size
	va_start(args, ArgTypes);
	str = ArgTypes;
	while(*str)
	{
		 int	dir;
		
		str = ReadEntry(NULL, NULL, &dir, str, args);
		if( !str )	break;
		
		// Out!
		if( dir & 1 )	outCount ++;
		
		// and.. In!
		if( dir & 2 )	inCount ++;
	}
	va_end(args);
	
	// Allocate buffers
	output = malloc( outCount*sizeof(tOutValue*) );
	input = malloc( inCount*sizeof(tInValue*) );
	
	// - re-zero so they can be used as indicies
	outCount = 0;
	inCount = 0;
	
	// Fill `output` and `input`
	va_start(args, ArgTypes);
	str = ArgTypes;
	while(*str)
	{
		tOutValue	*outParam;
		tInValue	*inParam;
		 int	dir;
		
		str = ReadEntry(&outParam, &inParam, &dir, str, args);
		if( !str )	break;
		
		if( dir & 1 )
			output[outCount++] = outParam;
		if( dir & 2 )
			input[inCount++] = inParam;
	}
	va_end(args);
	
	// Send syscall request
	
	
	// Clean up
	while(outCount--)	free(output[outCount]);
	free(output);
	while(inCount--)	free(input[inCount]);
	free(input);
	
	return 0;
}

// --- VFS Calls
int open(const char *Path, int Flags) {
	return _Syscall(">s >i", Path, Flags);
}

void close(int FD) {
	_Syscall(">i", FD);
}

size_t read(int FD, size_t Bytes, void *Dest) {
	return _Syscall(">i >i <d", FD, Bytes, Bytes, Dest);
}

size_t write(int FD, size_t Bytes, void *Src) {
	return _Syscall(">i >i >d", FD, Bytes, Bytes, Src);
}

int seek(int FD, int64_t Ofs, int Dir) {
	return _Syscall(">i >I >i", FD, Ofs, Dir);
}

uint64_t tell(int FD) {
	uint64_t	ret;
	_Syscall("<I >i", &ret, FD);
	return ret;
}

int ioctl(int fd, int id, void *data) {
	// NOTE: 1024 byte size is a hack
	return _Syscall(">i >i ?d", fd, id, 1024, data);
}
int finfo(int fd, t_sysFInfo *info, int maxacls) {
	return _Syscall(">i <d >i", fd, maxacls*sizeof(t_sysFInfo), info, maxacls);
}

int readdir(int fd, char *dest) {
	return _Syscall(">i <s", fd, dest);
}

int _SysOpenChild(int fd, char *name, int flags) {
	return _Syscall(">i >s >i", fd, name, flags);
}

int _SysGetACL(int fd, t_sysACL *dest) {
	return _Syscall(">i <d", fd, sizeof(t_sysACL), dest);
}

int _SysMount(const char *Device, const char *Directory, const char *Type, const char *Options) {
	return _Syscall(">s >s >s >s", Device, Directory, Type, Options);
}


// --- Error Handler
int	_SysSetFaultHandler(int (*Handler)(int)) {
	return 0;
}


// === Symbol List ===
#define DEFSYM(name)	{#name, name}
const tSym	caBuiltinSymbols[] = {
	{"_exit", exit},
	
	DEFSYM(open),
	DEFSYM(close),
	DEFSYM(read),
	DEFSYM(write),
	DEFSYM(seek),
	DEFSYM(tell),
	DEFSYM(ioctl),
	DEFSYM(finfo),
	DEFSYM(readdir),
	DEFSYM(_SysOpenChild),
	DEFSYM(_SysGetACL),
	DEFSYM(_SysMount),
	
	{"_SysSetFaultHandler", _SysSetFaultHandler}
};

const int	ciNumBuiltinSymbols = sizeof(caBuiltinSymbols)/sizeof(caBuiltinSymbols[0]);


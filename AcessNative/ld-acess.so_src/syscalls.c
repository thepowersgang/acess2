/*
 */
//#include <acess/sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// === IMPORTS ===
/**
 * \param ArgTypes
 *
 * Whitespace is ignored
 * >i:	Input Integer
 * >I:	Input Long Integer (64-bits)
 * >s:	Input String
 * >d:	Input Buffer (Preceded by valid size)
 * <I:	Output long integer
 * <d:	Output Buffer (Preceded by valid size)
 */
int _Syscall(const char *ArgTypes, ...)
{
	 int	outBufSize = 0;
	va_list	args;
	
	va_start(args, ArgTypes);
	va_end(args);
	
}

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

uint64_t tell(int FD) {
	uint64_t	ret;
	_Syscall("<I >i", &ret, FD);
	return ret;
}

int seek(int FD, uint64_t Ofs, int Dir) {
	return _Syscall(">i >I >i", FD, Ofs, Dir);
}


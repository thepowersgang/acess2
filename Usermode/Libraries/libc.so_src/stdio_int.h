/*
 * AcessOS Standard C Library
 * SDTIO_INT.H
 * Configuration Options
 */
#ifndef _STDIO_INT_H
#define _STDIO_INT_H

#include <sys/types.h>
#include <stddef.h>

// === CONSTANTS ===
#define FILE_FLAG_MODE_MASK	0x0007
#define FILE_FLAG_MODE_READ	0x0001
#define FILE_FLAG_MODE_WRITE	0x0002
#define FILE_FLAG_MODE_EXEC	0x0003
#define FILE_FLAG_MODE_APPEND	0x0004
#define FILE_FLAG_M_EXT 	0x0010
#define FILE_FLAG_M_BINARY	0x0020
#define FILE_FLAG_EOF   	0x0100
#define FILE_FLAG_DIRTY 	0x0200
#define FILE_FLAG_ALLOC 	0x1000

// === TYPES ===
struct sFILE {
  	 int	Flags;
  	 int	FD;
	off_t	Pos;	

	#if DEBUG_BUILD
	char	*FileName;	// heap
	#endif
	void	*Buffer;
	off_t	BufferStart;
	size_t	BufferSize;
};

#endif

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
#define FILE_FLAG_MODE_READ	0x0001
#define FILE_FLAG_MODE_WRITE	0x0002
#define FILE_FLAG_MODE_EXEC	0x0003
#define FILE_FLAG_MODE_APPEND	0x0004
#define FILE_FLAG_MODE_MASK	0x0007

#define FILE_FLAG_M_EXT 	0x0010
#define FILE_FLAG_M_BINARY	0x0020

#define FILE_FLAG_EOF   	0x0100
#define FILE_FLAG_LINEBUFFERED	0x1000	// Flush when '\n' encountered
#define FILE_FLAG_OURBUFFER	0x2000	// Buffer is owned by stdio

#define FILE_FLAG_ALLOC 	0x8000	// Internal 'is used' flag

#define _GetFileMode(fp)	((fp)->Flags & FILE_FLAG_MODE_MASK)

// === TYPES ===
struct sFILE {
  	 int	Flags;
  	 int	FD;
	off_t	Pos;	

	#if DEBUG_BUILD
	char	*FileName;	// heap
	#endif
	off_t	BufferOfs;	// File offset of first byte in buffer
	char	*Buffer;
	size_t	BufferPos;	// First unused byte in the buffer (read/write pos essentially)
	size_t	BufferSpace;	// Number of bytes allocated in \a Buffer
	
	// open_memstream
	char	**BufPtr;
	size_t	*LenPtr;
};

#endif

/*
 * AcessOS Standard C Library
 * SDTIO_INT.H
 * Configuration Options
 */
#ifndef _STDIO_INT_H
# define _STDIO_INT_H

// === CONSTANTS ===
#define FILE_FLAG_MODE_MASK	0x07
#define FILE_FLAG_MODE_READ		0x01
#define FILE_FLAG_MODE_WRITE	0x02
#define FILE_FLAG_MODE_EXEC		0x03
#define FILE_FLAG_MODE_APPEND	0x04
#define FILE_FLAG_M_EXT		0x10

// === TYPES ===
struct sFILE {
  	 int	FD;
  	 int	Flags;
	#if DEBUG_BUILD
	char	*FileName;
	#endif
	#if STDIO_LOCAL_BUFFER
	char	*Buffer;
	Uint64	BufferStart;
	 int	BufferSize;
	#endif
};

#endif

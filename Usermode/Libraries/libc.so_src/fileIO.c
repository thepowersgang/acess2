/*
 * AcessOS Basic C Library
 * stdio.c
 */
#include "config.h"
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"
#include "stdio_int.h"

#define DEBUG_BUILD	0

// === CONSTANTS ===
#define	_stdin	0
#define	_stdout	1

// === PROTOTYPES ===
EXPORT void	itoa(char *buf, uint64_t num, uint base, int minLength, char pad, int bSigned);
struct sFILE	*get_file_struct();

// === GLOBALS ===
struct sFILE	_iob[STDIO_MAX_STREAMS];	// IO Buffer
struct sFILE	*stdin;	// Standard Input
struct sFILE	*stdout;	// Standard Output
struct sFILE	*stderr;	// Standard Error
///\note Initialised in SoMain

// === CODE ===
/**
 * \fn FILE *freopen(char *file, char *mode, FILE *fp)
 */
EXPORT FILE *freopen(const char *file, const char *mode, FILE *fp)
{
	 int	openFlags = 0;
	 int	i;
	
	// Sanity Check Arguments
	if(!fp || !file || !mode)	return NULL;
	
	if(fp->Flags) {
		fflush(fp);
	} else
		fp->FD = -1;
	
	// Get main mode
	switch(mode[0])
	{
	case 'r':	fp->Flags = FILE_FLAG_MODE_READ;	break;
	case 'w':	fp->Flags = FILE_FLAG_MODE_WRITE;	break;
	case 'a':	fp->Flags = FILE_FLAG_MODE_APPEND;	break;
	case 'x':	fp->Flags = FILE_FLAG_MODE_EXEC;	break;
	default:
		return NULL;
	}
	// Get Modifiers
	for(i=1;mode[i];i++)
	{
		switch(mode[i])
		{
		case '+':	fp->Flags |= FILE_FLAG_M_EXT;
		}
	}
	
	// Get Open Flags
	switch(mode[0])
	{
	// Read
	case 'r':	openFlags = OPENFLAG_READ;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_WRITE;
		break;
	// Write
	case 'w':	openFlags = OPENFLAG_WRITE;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case 'x':	openFlags = OPENFLAG_EXEC;
		break;
	}
	
	//Open File
	if(fp->FD != -1)
		fp->FD = reopen(fp->FD, file, openFlags);
	else
		fp->FD = open(file, openFlags);
	if(fp->FD == -1) {
		fp->Flags = 0;
		return NULL;
	}
	
	if(mode[0] == 'a') {
		seek(fp->FD, 0, SEEK_END);	//SEEK_END
	}
	
	return fp;
}
/**
 \fn FILE *fopen(const char *file, const char *mode)
 \brief Opens a file and returns the pointer
 \param file	String - Filename to open
 \param mode	Mode to open in
*/
EXPORT FILE *fopen(const char *file, const char *mode)
{
	FILE	*retFile;
	
	// Sanity Check Arguments
	if(!file || !mode)	return NULL;
	
	// Create Return Structure
	retFile = get_file_struct();
	
	return freopen(file, mode, retFile);
}

EXPORT void fclose(FILE *fp)
{
	close(fp->FD);
	free(fp);
}

EXPORT void fflush(FILE *fp)
{
	///\todo Implement
}

EXPORT long int ftell(FILE *fp)
{
	if(!fp || !fp->FD)	return -1;
	
	return tell(fp->FD);
}

EXPORT int fseek(FILE *fp, long int amt, int whence)
{
	if(!fp || !fp->FD)	return -1;
	
	return seek(fp->FD, amt, whence);
}


/**
 * \fn EXPORT int vfprintf(FILE *fp, const char *format, va_list args)
 * \brief Print to a file from a variable argument list
 */
EXPORT int vfprintf(FILE *fp, const char *format, va_list args)
{
	va_list	tmpList = args;
	 int	size;
	char	sbuf[1024];
	char	*buf = sbuf;
	 
	if(!fp || !format)	return -1;
	
	size = vsnprintf(sbuf, 1024, (char*)format, tmpList);
	
	if( size >= 1024 )
	{
		buf = (char*)malloc(size+1);
		if(!buf) {
			write(_stdout, 31, "vfprintf ERROR: malloc() failed");
			return 0;
		}
		buf[size] = '\0';
	
		// Print
		vsnprintf(buf, size+1, (char*)format, args);
	}
	
	// Write to stream
	write(fp->FD, size, buf);
	
	// Free buffer
	free(buf);
	
	// Return written byte count
	return size;
}

/**
 * \fn int fprintf(FILE *fp, const char *format, ...)
 * \brief Print a formatted string to a stream
 */
EXPORT int fprintf(FILE *fp, const char *format, ...)
{
	va_list	args;
	 int	ret;
	
	// Get Size
	va_start(args, format);
	ret = vfprintf(fp, (char*)format, args);
	va_end(args);
	
	return ret;
}

/**
 * \fn EXPORT size_t fwrite(void *ptr, size_t size, size_t num, FILE *fp)
 * \brief Write to a stream
 */
EXPORT size_t fwrite(void *ptr, size_t size, size_t num, FILE *fp)
{
	 int	ret;
	if(!fp || !fp->FD)	return -1;
	
	ret = write(fp->FD, size*num, ptr);
	
	return ret;
}

/**
 * \fn EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
 * \brief Read from a stream
 */
EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
{
	 int	ret;
	if(!fp || !fp->FD)	return -1;
	
	ret = read(fp->FD, size*num, ptr);
	
	return ret;
}

/**
 * \fn EXPORT int fputc(int c, FILE *fp)
 * \brief Write a single character to the stream
 */
EXPORT int fputc(int c, FILE *fp)
{
	if(!fp || !fp->FD)	return -1;
	return write(fp->FD, 1, &c);
}

/**
 * \fn EXPORT int fgetc(FILE *fp)
 * \brief Read a character from the stream
 */
EXPORT int fgetc(FILE *fp)
{
	 int	ret = 0;
	if(!fp)	return -1;
	if(read(fp->FD, 1, &ret) == -1)	return -1;
	return ret;
}

// --- INTERNAL ---
/**
 * \fn FILE *get_file_struct()
 * \brief Returns a file descriptor structure
 */
FILE *get_file_struct()
{
	 int	i;
	for(i=0;i<STDIO_MAX_STREAMS;i++)
	{
		if(_iob[i].Flags == 0)	return &_iob[i];
	}
	return NULL;
}

EXPORT int putchar(int ch)
{
	return write(_stdout, 1, (char*)&ch);
}

EXPORT int	puts(const char *str)
{
	 int	len;
	
	if(!str)	return 0;
	len = strlen(str);
	
	len = write(_stdout, len, (char*)str);
	write(_stdout, 1, "\n");
	return len;
}

EXPORT int vsprintf(char * __s, const char *__format, va_list __args)
{
	return vsnprintf(__s, 0x7FFFFFFF, __format, __args);
}

//sprintfv
/**
 \fn EXPORT void vsnprintf(char *buf, const char *format, va_list args)
 \brief Prints a formatted string to a buffer
 \param buf	Pointer - Destination Buffer
 \param format	String - Format String
 \param args	VarArgs List - Arguments
*/
EXPORT int vsnprintf(char *buf, size_t __maxlen, const char *format, va_list args)
{
	char	tmp[65];
	 int	c, minSize;
	 int	pos = 0;
	char	*p;
	char	pad;
	uint64_t	arg;
	 int	bLongLong;

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		// Non-control character
		if (c != '%') {
			if(buf && pos < __maxlen)	buf[pos] = c;
			pos ++;
			continue;
		}
		
		// Control Character
		c = *format++;
		if(c == '%') {	// Literal %
			if(buf && pos < __maxlen)	buf[pos] = '%';
			pos ++;
			continue;
		}
		
		// Padding
		if(c == '0') {
			pad = '0';
			c = *format++;
		} else
			pad = ' ';
		minSize = 0;
		if('1' <= c && c <= '9')
		{
			while('0' <= c && c <= '9')
			{
				minSize *= 10;
				minSize += c - '0';
				c = *format++;
			}
		}
	
		// Check for long long
		bLongLong = 0;
		if(c == 'l')
		{
			c = *format++;
			if(c == 'l') {
				bLongLong = 1;
			}
		}
			
		p = tmp;
		
		// Get Type
		switch( c )
		{
		// Signed Integer
		case 'd':	case 'i':
			// Get Argument
			if(bLongLong)	arg = va_arg(args, int64_t);
			else			arg = va_arg(args, int32_t);
			itoa(tmp, arg, 10, minSize, pad, 1);
			goto sprintf_puts;
		
		// Unsigned Integer
		case 'u':
			// Get Argument
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 10, minSize, pad, 0);
			goto sprintf_puts;
		
		// Pointer
		case 'p':
			if(buf && pos+2 < __maxlen) {
				buf[pos] = '*';
				buf[pos+1] = '0';
				buf[pos+2] = 'x';
			}
			pos += 3;
			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 16, minSize, pad, 0);
			goto sprintf_puts;
			// Fall through to hex
		// Unsigned Hexadecimal
		case 'x':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 16, minSize, pad, 0);
			goto sprintf_puts;
		
		// Unsigned Octal
		case 'o':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 8, minSize, pad, 0);
			goto sprintf_puts;
		
		// Unsigned binary
		case 'b':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 2, minSize, pad, 0);
			goto sprintf_puts;

		// String
		case 's':
			arg = va_arg(args, uint32_t);
			p = (void*)(intptr_t)arg;
		sprintf_puts:
			if(!p)	p = "(null)";
			//_SysDebug("vsnprintf: p = '%s'", p);
			if(buf) {
				while(*p) {
					if(pos < __maxlen)	buf[pos] = *p;
					pos ++;	p ++;
				}
			}
			else {
				while(*p) {
					pos++; p++;
				}
			}
			break;

		// Unknown, just treat it as a character
		default:
			arg = va_arg(args, uint32_t);
			if(buf && pos < __maxlen)	buf[pos] = arg;
			pos ++;
			break;
		}
    }
	if(buf && pos < __maxlen)	buf[pos] = '\0';
	
	//_SysDebug("vsnprintf: buf = '%s'", buf);
	
	return pos;
}

const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn static void itoa(char *buf, uint64_t num, int base, int minLength, char pad, int bSigned)
 * \brief Convert an integer into a character string
 */
EXPORT void itoa(char *buf, uint64_t num, size_t base, int minLength, char pad, int bSigned)
{
	char	tmpBuf[64];
	 int	pos=0, i;

	if(!buf)	return;
	if(base > 16 || base < 2) {
		buf[0] = 0;
		return;
	}
	
	if(bSigned && (int64_t)num < 0)
	{
		num = -num;
		bSigned = 1;
	} else
		bSigned = 0;
	
	while(num > base-1) {
		tmpBuf[pos++] = cUCDIGITS[ num % base ];
		num = (uint64_t) num / (uint64_t)base;		// Shift {number} right 1 digit
	}

	tmpBuf[pos++] = cUCDIGITS[ num % base ];		// Last digit of {number}
	if(bSigned)	tmpBuf[pos++] = '-';	// Append sign symbol if needed
	
	i = 0;
	minLength -= pos;
	while(minLength-- > 0)	buf[i++] = pad;
	while(pos-- > 0)		buf[i++] = tmpBuf[pos];	// Reverse the order of characters
	buf[i] = 0;
}

/**
 * \fn EXPORT int printf(const char *format, ...)
 * \brief Print a string to stdout
 */
EXPORT int printf(const char *format, ...)
{
	#if 1
	 int	size;
	char	sbuf[1024];
	char	*buf = sbuf;
	va_list	args;
	
	// Get final size
	va_start(args, format);
	size = vsnprintf(sbuf, 1024, (char*)format, args);
	va_end(args);
	
	if( size >= 1024 ) {
		// Allocate buffer
		buf = (char*)malloc(size+1);
		if(buf) {
			write(_stdout, 29, "PRINTF ERROR: malloc() failed");
			return 0;
		}
		buf[size] = '\0';
	
		// Fill Buffer
		va_start(args, format);
		vsnprintf(buf, size+1, (char*)format, args);
		va_end(args);
	}
	
	// Send to stdout
	write(_stdout, size+1, buf);
	
	// Free buffer
	free(buf);
	// Return
	return size;
	
	#else
	
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = fprintfv(stdout, (char*)format, args);
	va_end(args);
	return ret;
	#endif
}

/**
 * \fn EXPORT int sprintf(const char *buf, char *format, ...)
 * \brief Print a formatted string to a buffer
 */
EXPORT int sprintf(char *buf, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsprintf((char*)buf, (char*)format, args);
	va_end(args);
	return ret;
}

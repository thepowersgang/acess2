/*
AcessOS Basic C Library
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
EXPORT void	itoa(char *buf, unsigned long num, int base, int minLength, char pad);
struct sFILE	*get_file_struct();

// === GLOBALS ===
struct sFILE	_iob[STDIO_MAX_STREAMS];	// IO Buffer
struct sFILE	*stdin = &_iob[0];	// Standard Input
struct sFILE	*stdout = &_iob[1];	// Standard Output
struct sFILE	*stderr = &_iob[2];	// Standard Error

// === CODE ===
/**
 * \fn FILE *freopen(FILE *fp, char *file, char *mode)
 */
EXPORT FILE *freopen(FILE *fp, char *file, char *mode)
{
	 int	openFlags = 0;
	 int	i;
	
	// Sanity Check Arguments
	if(!fp || !file || !mode)	return NULL;
	
	if(fp->Flags) {
		fflush(fp);
		close(fp->FD);
	}
	
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
	fp->FD = reopen(fp->FD, file, openFlags);
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
 \fn FILE *fopen(char *file, char *mode)
 \brief Opens a file and returns the pointer
 \param file	String - Filename to open
 \param mode	Mode to open in
*/
EXPORT FILE *fopen(char *file, char *mode)
{
	FILE	*retFile;
	
	// Sanity Check Arguments
	if(!file || !mode)	return NULL;
	
	// Create Return Structure
	retFile = get_file_struct();
	
	return freopen(retFile, file, mode);
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

/**
 * \fn int fprintfv(FILE *fp, const char *format, va_list args)
 * \brief Print to a file from a variable argument list
 */
EXPORT int fprintfv(FILE *fp, const char *format, va_list args)
{
	va_list	tmpList = args;
	 int	size;
	char	*buf;
	 
	if(!fp || !format)	return -1;
	
	size = ssprintfv((char*)format, tmpList);
	
	buf = (char*)malloc(size+1);
	buf[size] = '\0';
	
	// Print
	sprintfv(buf, (char*)format, args);
	
	// Write to stream
	write(fp->FD, size+1, buf);
	
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
	ret = fprintfv(fp, (char*)format, args);
	va_end(args);
	
	return ret;
}

/**
 */
EXPORT size_t fwrite(void *ptr, size_t size, size_t num, FILE *fp)
{
	 int	ret;
	if(!fp || !fp->FD)	return -1;
	
	ret = write(fp->FD, size*num, ptr);
	
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

//sprintfv
/**
 \fn EXPORT void sprintfv(char *buf, const char *format, va_list args)
 \brief Prints a formatted string to a buffer
 \param buf	Pointer - Destination Buffer
 \param format	String - Format String
 \param args	VarArgs List - Arguments
*/
EXPORT void sprintfv(char *buf, const char *format, va_list args)
{
	char	tmp[33];
	 int	c, arg, minSize;
	 int	pos = 0;
	char	*p;
	char	pad;

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		//SysDebug("c = '%c'\n", c);
		if (c != '%') {
			buf[pos++] = c;
			continue;
		}
		
		c = *format++;
		if(c == '%') {
			buf[pos++] = '%';
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
	
		p = tmp;
	
		// Get Argument
		arg = va_arg(args, int);
		// Get Type
		switch (c) {
		case 'd':
		case 'i':
			if(arg < 0) {
				buf[pos++] = '-';
				arg = -arg;
			}
			itoa(tmp, arg, 10, minSize, pad);
			goto sprintf_puts;
		//	break;
		case 'u':
			itoa(tmp, arg, 10, minSize, pad);
			goto sprintf_puts;
		//	break;
		case 'x':
			itoa(tmp, arg, 16, minSize, pad);
			goto sprintf_puts;
		//	break;
		case 'o':
			itoa(tmp, arg, 8, minSize, pad);
			goto sprintf_puts;
		//	break;
		case 'b':
			itoa(tmp, arg, 2, minSize, pad);
			goto sprintf_puts;
		//	break;

		case 's':
			p = (void*)arg;
		sprintf_puts:
			if(!p)	p = "(null)";
			while(*p)	buf[pos++] = *p++;
			break;

		default:
			buf[pos++] = arg;
			break;
		}
    }
	buf[pos++] = '\0';
}
/*
ssprintfv
- Size, Stream, Print Formated, Variable Argument List
*/
/**
 \fn EXPORT int ssprintfv(char *format, va_list args)
 \brief Gets the total character count from a formatted string
 \param format	String - Format String
 \param args	VarArgs - Argument List
*/
EXPORT int ssprintfv(char *format, va_list args)
{
	char	tmp[33];
	 int	c, arg, minSize;
	 int	len = 0;
	char	*p;
	char	pad;

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		if (c != '%') {
			len++;
			continue;
		}
		
		c = *format++;
		
		// Literal '%'
		if(c == '%') {
			len++;
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
		
		p = tmp;
		arg = va_arg(args, int);
		switch (c) {			
		case 'd':
		case 'i':
			if(arg < 0) {
				len ++;
				arg = -arg;
			}
			itoa(tmp, arg, 10, minSize, pad);
			goto sprintf_puts;
		case 'u':
			itoa(tmp, arg, 10, minSize, pad);
			goto sprintf_puts;
		case 'x':
			itoa(tmp, arg, 16, minSize, pad);
			goto sprintf_puts;
		case 'o':
			itoa(tmp, arg, 8, minSize, pad);
			p = tmp;
			goto sprintf_puts;
		case 'b':
			itoa(tmp, arg, 2, minSize, pad);
			goto sprintf_puts;

		case 's':
			p = (char*)arg;
		sprintf_puts:
			if(!p)	p = "(null)";
			while(*p)	len++, p++;
			break;

		default:
			len ++;
			break;
		}
    }
	return len;
}

const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn static void itoa(char *buf, unsigned long num, int base, int minLength, char pad)
 * \brief Convert an integer into a character string
 */
EXPORT void itoa(char *buf, unsigned long num, int base, int minLength, char pad)
{
	char	tmpBuf[32];
	 int	pos=0, i;

	if(!buf)	return;
	if(base > 16) {
		buf[0] = 0;
		return;
	}
	
	while(num > base-1) {
		tmpBuf[pos] = cUCDIGITS[ num % base ];
		num = (long) num / base;		//Shift {number} right 1 digit
		pos++;
	}

	tmpBuf[pos++] = cUCDIGITS[ num % base ];		//Last digit of {number}
	i = 0;
	minLength -= pos;
	while(minLength-- > 0)	buf[i++] = pad;
	while(pos-- > 0)		buf[i++] = tmpBuf[pos];	//Reverse the order of characters
	buf[i] = 0;
}

/**
 * \fn EXPORT int printf(const char *format, ...)
 * \brief Print a string to stdout
 */
EXPORT int printf(const char *format, ...)
{
	#if 0
	 int	size;
	char	*buf;
	va_list	args;
	
	va_start(args, format);
	size = ssprintfv((char*)format, args);
	va_end(args);
	
	buf = (char*)malloc(size+1);
	buf[size] = '\0';
	
	va_start(args, format);
	sprintfv(buf, (char*)format, args);
	va_end(args);
	
	
	write(_stdout, size+1, buf);
	
	free(buf);
	return size;
	#endif
	
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = fprintfv(stdout, (char*)format, args);
	va_end(args);
	return ret;
}

/**
 * \fn EXPORT int sprintf(const char *buf, char *format, ...)
 * \brief Print a formatted string to a buffer
 */
EXPORT int sprintf(char *buf, const char *format, ...)
{
	va_list	args;
	va_start(args, format);
	sprintfv((char*)buf, (char*)format, args);
	va_end(args);
	return 1;
}

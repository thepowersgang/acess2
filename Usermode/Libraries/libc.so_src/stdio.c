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

#define WRITE_STR(_fd, _str)	write(_fd, _str, sizeof(_str))

#define DEBUG_BUILD	0

// === CONSTANTS ===
#define	_stdin	0
#define	_stdout	1

// === PROTOTYPES ===
EXPORT void	itoa(char *buf, uint64_t num, size_t base, int minLength, char pad, int bSigned);
struct sFILE	*get_file_struct();

// === GLOBALS ===
struct sFILE	_iob[STDIO_MAX_STREAMS];	// IO Buffer
struct sFILE	*stdin;	// Standard Input
struct sFILE	*stdout;	// Standard Output
struct sFILE	*stderr;	// Standard Error
///\note Initialised in SoMain

// === CODE ===
int _fopen_modetoflags(const char *mode)
{
	int flags = 0;
	
	// Get main mode
	switch(*mode)
	{
	case 'r':	flags = FILE_FLAG_MODE_READ;	break;
	case 'w':	flags = FILE_FLAG_MODE_WRITE;	break;
	case 'a':	flags = FILE_FLAG_MODE_APPEND;	break;
	case 'x':	flags = FILE_FLAG_MODE_EXEC;	break;	// Acess addon
	default:
		return -1;
	}
	mode ++;

	// Get Modifiers
	for( ; *mode; mode ++ )
	{
		switch(*mode)
		{
		case 'b':	flags |= FILE_FLAG_M_BINARY;	break;
		case '+':	flags |= FILE_FLAG_M_EXT;	break;
		default:
			return -1;
		}
	}
	
	return flags;
}

/**
 * \fn FILE *freopen(char *file, char *mode, FILE *fp)
 */
EXPORT FILE *freopen(const char *file, const char *mode, FILE *fp)
{
	 int	openFlags = 0;
	
	// Sanity Check Arguments
	if(!fp || !file || !mode)	return NULL;
	
	if(fp->FD != -1) {
		fflush(fp);
	}

	// Get stdio flags
	fp->Flags = _fopen_modetoflags(mode);
	if(fp->Flags == -1)
		return NULL;
	
	// Get Open Flags
	switch(fp->Flags & FILE_FLAG_MODE_MASK)
	{
	// Read
	case FILE_FLAG_MODE_READ:
		openFlags = OPENFLAG_READ;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_WRITE;
		break;
	// Write
	case FILE_FLAG_MODE_WRITE:
		openFlags = OPENFLAG_WRITE;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case FILE_FLAG_MODE_APPEND:
		openFlags = OPENFLAG_APPEND;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case FILE_FLAG_MODE_EXEC:
		openFlags = OPENFLAG_EXEC;
		break;
	}

	//Open File
	if(fp->FD != -1)
		fp->FD = _SysReopen(fp->FD, file, openFlags);
	else
		fp->FD = _SysOpen(file, openFlags);
	if(fp->FD == -1) {
		fp->Flags = 0;
		return NULL;
	}
	
	if( (fp->Flags & FILE_FLAG_MODE_MASK) == FILE_FLAG_MODE_APPEND ) {
		_SysSeek(fp->FD, 0, SEEK_END);	//SEEK_END
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

EXPORT FILE *fmemopen(void *buffer, size_t length, const char *mode)
{
	FILE	*ret;
	
	if( !buffer || !mode )	return NULL;
	
	ret = get_file_struct();
	
	ret->FD = -2;
	ret->Flags = _fopen_modetoflags(mode);
	if(ret->Flags == -1) {
		ret->Flags = 0;
		return NULL;
	}
	
	ret->Buffer = buffer;
	ret->BufferStart = 0;
	ret->BufferSize = length;
	
	return ret;
}

EXPORT int fclose(FILE *fp)
{
	fflush(fp);
	if( fp->FD != -1 ) {
		_SysClose(fp->FD);
	}
	fp->Flags = 0;
	fp->FD = -1;
	return 0;
}

EXPORT void fflush(FILE *fp)
{
	if( !fp || fp->FD == -1 )
		return ;
	
	if( !(fp->Flags & FILE_FLAG_DIRTY) )
		return ;
	
	// Nothing to do for memory files
	if( fp->FD == -2 )
		return ;
}

EXPORT void clearerr(FILE *fp)
{
	if( !fp || fp->FD == -1 )
		return ;
	
	// TODO: Impliment clearerr()
}

EXPORT int feof(FILE *fp)
{
	if( !fp || fp->FD == -1 )
		return 0;
	return !!(fp->Flags & FILE_FLAG_EOF);
}

EXPORT int ferror(FILE *fp)
{
	if( !fp || fp->FD == -1 )
		return 0;
	return 0;
}
EXPORT int fileno(FILE *stream)
{
	return stream->FD;
}

EXPORT off_t ftell(FILE *fp)
{
	if(!fp || fp->FD == -1)	return -1;

	if( fp->FD == -2 )
		return fp->Pos;	
	else
		return _SysTell(fp->FD);
}

EXPORT int fseek(FILE *fp, long int amt, int whence)
{
	if(!fp || fp->FD == -1)	return -1;

	if( fp->FD == -2 ) {
		switch(whence)
		{
		case SEEK_CUR:
			fp->Pos += amt;
			break;
		case SEEK_SET:
			fp->Pos = amt;
			break;
		case SEEK_END:
			if( fp->BufferSize < (size_t)amt )
				fp->Pos = 0;
			else
				fp->Pos = fp->BufferSize - amt;
			break;
		}
		if(fp->Pos > (off_t)fp->BufferSize) {
			fp->Pos = fp->BufferSize;
			fp->Flags |= FILE_FLAG_EOF;
		}
		return 0;
	}
	else
		return _SysSeek(fp->FD, amt, whence);
}


/**
 * \fn EXPORT int vfprintf(FILE *fp, const char *format, va_list args)
 * \brief Print to a file from a variable argument list
 */
EXPORT int vfprintf(FILE *fp, const char *format, va_list args)
{
	va_list	tmpList;
	 int	size;

	if(!fp || !format)	return -1;

	va_copy(tmpList, args);
	
	size = vsnprintf(NULL, 0, (char*)format, tmpList);
	char	buf[size+1];
	vsnprintf(buf, size+1, (char*)format, args);
	
	// Write to stream
	fwrite(buf, size, 1, fp);
	
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
EXPORT size_t fwrite(const void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	ret;
	
	if(!fp || fp->FD == -1)
		return -1;

	if( fp->FD == -2 ) {
		size_t	avail = (fp->BufferSize - fp->Pos) / size;
		if( avail == 0 )
			fp->Flags |= FILE_FLAG_EOF;
		if( num > avail )	num = avail;
		size_t	bytes = num * size;
		memcpy((char*)fp->Buffer + fp->Pos, ptr, bytes);
		fp->Pos += bytes;
		ret = num;
	}
	else {	
		ret = _SysWrite(fp->FD, ptr, size*num);
		ret /= size;
	}
	
	return ret;
}

/**
 * \fn EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
 * \brief Read from a stream
 */
EXPORT size_t fread(void *ptr, size_t size, size_t num, FILE *fp)
{
	size_t	ret;
	
	if(!fp || fp->FD == -1)
		return -1;

	if( fp->FD == -2 ) {
		size_t	avail = (fp->BufferSize - fp->Pos) / size;
		if( avail == 0 )
			fp->Flags |= FILE_FLAG_EOF;
		if( num > avail )	num = avail;
		size_t	bytes = num * size;
		memcpy(ptr, (char*)fp->Buffer + fp->Pos, bytes);
		fp->Pos += bytes;
		ret = num;
	}
	else {
		ret = _SysRead(fp->FD, ptr, size*num);
		if( ret == (size_t)-1)
			return -1;
		if( ret == 0 && size*num > 0 ) {
			fp->Flags |= FILE_FLAG_EOF;
			return 0;
		}
		ret /= size;
	}
		
	return ret;
}

/**
 * \brief Write a string to a stream (without trailing \n)
 */
EXPORT int fputs(const char *s, FILE *fp)
{
	int len = strlen(s);
	return fwrite(s, 1, len, fp);
}

/**
 * \brief Read a line (and possible trailing \n into a buffer)
 */
EXPORT char *fgets(char *s, int size, FILE *fp)
{
	int ofs = 0;
	char	ch = '\0';
	while( ofs < size && ch != '\n' )
	{
		if( fread(&ch, 1, 1, fp) != 1 )
			break;
		s[ofs ++] = ch;
	}
	if( ofs < size )
		s[ofs] = '\0';
	return s;
}

/**
 * \fn EXPORT int fputc(int c, FILE *fp)
 * \brief Write a single character to the stream
 */
EXPORT int fputc(int c, FILE *fp)
{
	return fwrite(&c, 1, 1, fp);
}

EXPORT int putchar(int c)
{
	c &= 0xFF;
	return _SysWrite(_stdout, &c, 1);
}

/**
 * \fn EXPORT int fgetc(FILE *fp)
 * \brief Read a character from the stream
 */
EXPORT int fgetc(FILE *fp)
{
	char	ret = 0;
	if( fread(&ret, 1, 1, fp) != 1 )
		return -1;
	return ret;
}

EXPORT int getchar(void)
{
	char	ret = 0;
	if(_SysRead(_stdin, &ret, 1) != 1)	return -1;
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
		if(_iob[i].Flags & FILE_FLAG_ALLOC)
			continue ;
		_iob[i].Flags |= FILE_FLAG_ALLOC;
		_iob[i].FD = -1;
		_iob[i].Pos = 0;
		return &_iob[i];
	}
	return NULL;
}

EXPORT int puts(const char *str)
{
	 int	len;
	
	if(!str)	return 0;
	len = strlen(str);
	
	len = _SysWrite(_stdout, str, len);
	_SysWrite(_stdout, "\n", 1);
	return len;
}

EXPORT int vsprintf(char * __s, const char *__format, va_list __args)
{
	return vsnprintf(__s, 0x7FFFFFFF, __format, __args);
}

//sprintfv
/**
 * \fn EXPORT void vsnprintf(char *buf, const char *format, va_list args)
 * \brief Prints a formatted string to a buffer
 * \param buf	Pointer - Destination Buffer
 * \param format	String - Format String
 * \param args	VarArgs List - Arguments
 */
EXPORT int vsnprintf(char *buf, size_t __maxlen, const char *format, va_list args)
{
	char	tmp[65];
	 int	c, minSize, precision, len;
	size_t	pos = 0;
	char	*p;
	char	pad;
	uint64_t	arg;
	 int	bLongLong, bPadLeft;

	void _addchar(char ch)
	{
		if(buf && pos < __maxlen)	buf[pos] = ch;
		pos ++;
	}

	tmp[32] = '\0';
	
	while((c = *format++) != 0)
	{
		// Non-control character
		if (c != '%') {
			_addchar(c);
			continue;
		}
		
		// Control Character
		c = *format++;
		if(c == '%') {	// Literal %
			_addchar('%');
			continue;
		}
		
		bPadLeft = 0;
		bLongLong = 0;
		minSize = 0;
		precision = -1;
		pad = ' ';
		
		// Padding Character
		if(c == '0') {
			pad = '0';
			c = *format++;
		}
		// Padding length
		if( c == '*' ) {
			// Variable length
			minSize = va_arg(args, size_t);
			c = *format++;
		}
		else {
			if('1' <= c && c <= '9')
			{
				minSize = 0;
				while('0' <= c && c <= '9')
				{
					minSize *= 10;
					minSize += c - '0';
					c = *format++;
				}
			}
		}

		// Precision
		if(c == '.') {
			c = *format++;
			if(c == '*') {
				precision = va_arg(args, size_t);
				c = *format++;
			}
			else if('1' <= c && c <= '9')
			{
				precision = 0;
				while('0' <= c && c <= '9')
				{
					precision *= 10;
					precision += c - '0';
					c = *format++;
				}
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
		
		// Just help things along later
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
			precision = -1;
			goto sprintf_puts;
		
		// Unsigned Integer
		case 'u':
			// Get Argument
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 10, minSize, pad, 0);
			precision = -1;
			goto sprintf_puts;
		
		// Pointer
		case 'p':
			_addchar('*');
			_addchar('0');
			_addchar('x');
			arg = va_arg(args, intptr_t);
			itoa(tmp, arg, 16, minSize, pad, 0);
			precision = -1;
			goto sprintf_puts;
		// Unsigned Hexadecimal
		case 'x':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 16, minSize, pad, 0);
			precision = -1;
			goto sprintf_puts;
		
		// Unsigned Octal
		case 'o':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 8, minSize, pad, 0);
			precision = -1;
			goto sprintf_puts;
		
		// Unsigned binary
		case 'b':
			if(bLongLong)	arg = va_arg(args, uint64_t);
			else			arg = va_arg(args, uint32_t);
			itoa(tmp, arg, 2, minSize, pad, 0);
			precision = -1;
			goto sprintf_puts;

		// String
		case 's':
			p = va_arg(args, char*);
		sprintf_puts:
			if(!p)	p = "(null)";
			//_SysDebug("vsnprintf: p = '%s'", p);
			if(precision >= 0)
				len = strnlen(p, precision);
			else
				len = strlen(p);
			if(bPadLeft)	while(minSize > len++)	_addchar(pad);
			while( *p ) {
				if(precision >= 0 && precision -- == 0)
					break;
				_addchar(*p++);
			}
			if(!bPadLeft)	while(minSize > len++)	_addchar(pad);
			break;

		// Unknown, just treat it as a character
		default:
			arg = va_arg(args, uint32_t);
			_addchar(arg);
			break;
		}
	}
	_addchar('\0');
	pos --;
	
	//_SysDebug("vsnprintf: buf = '%s'", buf);
	
	return pos;
}

const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn static void itoa(char *buf, uint64_t num, int base, int minLength, char pad, int bSigned)
 * \brief Convert an integer into a character string
 * \param buf	Destination Buffer
 * \param num	Number to convert
 * \param base	Base-n number output
 * \param minLength	Minimum length of output
 * \param pad	Padding used to ensure minLength
 * \param bSigned	Signed number output?
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
	
	// Encode into reversed string
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
	 int	size;
	va_list	args;
	
	// Get final size
	va_start(args, format);
	size = vsnprintf(NULL, 0, (char*)format, args);
	va_end(args);
	char buf[size+1];
	// Fill Buffer
	va_start(args, format);
	vsnprintf(buf, size+1, (char*)format, args);
	va_end(args);
	
	// Send to stdout
	_SysWrite(_stdout, buf, size+1);
	
	// Free buffer
	free(buf);
	// Return
	return size;
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

/**
 * \fn EXPORT int snprintf(const char *buf, size_t maxlen, char *format, ...)
 * \brief Print a formatted string to a buffer
 */
EXPORT int snprintf(char *buf, size_t maxlen, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsnprintf((char*)buf, maxlen, (char*)format, args);
	va_end(args);
	return ret;
}

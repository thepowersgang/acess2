/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * scanf.c
 * - *scanf family of functions
 */
#include "lib.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// === TYPES ===
typedef void	(*printf_putch_t)(void *h, char ch);

// === PROTOTYPES ===
void	itoa(char *buf, uint64_t num, size_t base, int minLength, char pad, int bSigned);

// === CODE ===
/**
 * \fn EXPORT void vsnprintf(char *buf, const char *format, va_list args)
 * \brief Prints a formatted string to a buffer
 * \param buf	Pointer - Destination Buffer
 * \param format	String - Format String
 * \param args	VarArgs List - Arguments
 */
EXPORT int _vcprintf_int(printf_putch_t putch_cb, void *putch_h, const char *format, va_list args)
{
	char	tmp[65];
	 int	c, minSize, precision, len;
	size_t	pos = 0;
	char	*p;
	char	pad;
	uint64_t	arg;
	 int	bLongLong, bPadLeft;

	#define _addchar(ch) do { \
		putch_cb(putch_h, ch); \
		pos ++; \
	} while(0)

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
	#undef _addchar
	
	return pos;
}

struct s_sprintf_info {
	char	*dest;
	size_t	ofs;
	size_t	maxlen;
};

void _vsnprintf_putch(void *h, char ch)
{
	struct s_sprintf_info	*info = h;
	if(info->ofs < info->maxlen)
		info->dest[info->ofs++] = ch;
}

EXPORT int vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list __args)
{
	struct s_sprintf_info	info = {__s, 0, __maxlen};
	int ret;
	ret = _vcprintf_int(_vsnprintf_putch, &info, __format, __args);
	_vsnprintf_putch(&info, '\0');
	return ret;
}

EXPORT int snprintf(char *buf, size_t maxlen, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsnprintf((char*)buf, maxlen, (char*)format, args);
	va_end(args);
	return ret;
}


EXPORT int vsprintf(char * __s, const char *__format, va_list __args)
{
	return vsnprintf(__s, 0x7FFFFFFF, __format, __args);
}
EXPORT int sprintf(char *buf, const char *format, ...)
{
	 int	ret;
	va_list	args;
	va_start(args, format);
	ret = vsprintf((char*)buf, (char*)format, args);
	va_end(args);
	return ret;
}

void _vfprintf_putch(void *h, char ch)
{
	fputc(ch, h);
}

EXPORT int vfprintf(FILE *__fp, const char *__format, va_list __args)
{
	return _vcprintf_int(_vfprintf_putch, __fp, __format, __args);
}

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

EXPORT int vprintf(const char *__format, va_list __args)
{
	return vfprintf(stdout, __format, __args);
}

EXPORT int printf(const char *format, ...)
{
	va_list	args;
	 int	ret;
	
	// Get final size
	va_start(args, format);
	ret = vprintf(format, args);
	va_end(args);
	
	// Return
	return ret;
}

const char cUCDIGITS[] = "0123456789ABCDEF";
/**
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

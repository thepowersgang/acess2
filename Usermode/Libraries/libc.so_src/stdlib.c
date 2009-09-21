/*
AcessOS Basic C Library

stdlib.c
*/
#include <acess/sys.h>
#include <stdlib.h>
#include "lib.h"

int _stdout = 1;
int _stdin = 2;

EXPORT int	puts(const char *str);
EXPORT void	itoa(char *buf, unsigned long num, int base, int minLength, char pad);
EXPORT int	atoi(const char *str);
EXPORT int	puts(const char *str);
EXPORT int	ssprintfv(char *format, va_list args);
EXPORT void	sprintfv(char *buf, char *format, va_list args);
EXPORT int	printf(const char *format, ...);
EXPORT int	strlen(const char *str);
EXPORT int	strcmp(char *str1, char *str2);
EXPORT int	strncmp(char *str1, char *str2, size_t len);
EXPORT char	*strcpy(char *dst, const char *src);

// === CODE ===
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
 \fn EXPORT void sprintfv(char *buf, char *format, va_list args)
 \brief Prints a formatted string to a buffer
 \param buf	Pointer - Destination Buffer
 \param format	String - Format String
 \param args	VarArgs List - Arguments
*/
EXPORT void sprintfv(char *buf, char *format, va_list args)
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
 */
EXPORT int atoi(const char *str)
{
	 int	neg = 0;
	 int	ret = 0;
	
	// NULL Check
	if(!str)	return 0;
	
	while(*str == ' ' || *str == '\t')	str++;
	
	// Check for negative
	if(*str == '-') {
		neg = 1;
		str++;
	}
	
	if(*str == '0') {
		str ++;
		if(*str == 'x') {
			str++;
			// Hex
			while( ('0' <= *str && *str <= '9')
				|| ('A' <= *str && *str <= 'F' )
				|| ('a' <= *str && *str <= 'f' )
				)
			{
				ret *= 16;
				if(*str <= '9') {
					ret += *str - '0';
				} else if (*str <= 'F') {
					ret += *str - 'A' + 10;
				} else {
					ret += *str - 'a' + 10;
				}
				str++;
			}
		} else {
			// Octal
			while( '0' <= *str && *str <= '7' )
			{
				ret *= 8;
				ret += *str - '0';
				str++;
			}
		}
	} else {
		// Decimal
		while( '0' <= *str && *str <= '9' )
		{
			ret *= 10;
			ret += *str - '0';
			str++;
		}
	}
	
	// Negate if needed
	if(neg)	ret = -ret;
	return ret;
}

//printf
EXPORT int printf(const char *format, ...)
{
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
}

EXPORT int sprintf(const char *buf, char *format, ...)
{
	va_list	args;
	va_start(args, format);
	sprintfv((char*)buf, (char*)format, args);
	va_end(args);
	return 1;
}


//MEMORY
EXPORT int strcmp(char *s1, char *s2)
{
	while(*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}
EXPORT char *strcpy(char *dst, const char *src)
{
	char *_dst = dst;
	while(*src) {
		*dst = *src;
		src++; dst++;
	}
	*dst = '\0';
	return _dst;
}
EXPORT int strlen(const char *str)
{
	int retval;
	for(retval = 0; *str != '\0'; str++)
		retval++;
	return retval;
}

EXPORT int strncmp(char *s1, char *s2, size_t len)
{
	while(--len && *s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

EXPORT void *memcpy(void *dest, void *src, unsigned int count)
{
    char *sp = (char *)src;
    char *dp = (char *)dest;
    for(;count--;) *dp++ = *sp++;
    return dest;
}

EXPORT void *memmove(void *dest, void *src, unsigned int count)
{
    char *sp = (char *)src;
    char *dp = (char *)dest;
	// Check if corruption will happen
	if( (unsigned int)dest > (unsigned int)src && (unsigned int)dest < (unsigned int)src+count )
		for(;count--;) dp[count] = sp[count];
	else
    	for(;count--;) *dp++ = *sp++;
    return dest;
}

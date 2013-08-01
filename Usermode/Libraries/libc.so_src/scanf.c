/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * scanf.c
 * - *scanf family of functions
 */
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

extern void	_SysDebug(const char *format, ...);

enum e_vcscanf_types
{
	_VCSCANF_NOTYPE,
	_VCSCANF_INT,
	_VCSCANF_REAL,
};

enum e_vcscanf_sizes
{
	_VCSCANF_UNDEF,
	_VCSCANF_CHAR,
	_VCSCANF_SHORT,
	_VCSCANF_LONG,
	_VCSCANF_LONGLONG,
	_VCSCANF_INTMAX,
	_VCSCANF_SIZET,
	_VCSCANF_PTRDIFF,
	_VCSCANF_LONGDOUBLE
};

// === CODE ===
int _vcscanf_int(int (*__getc)(void *), void (*__rewind)(void*), void *h, int base, int maxlen, long long *outval)
{
	char	ich;
	 int	sgn = 1;
	long long int	ret = 0;
	 int	n_read = 0;

	// Initialise output to something sane
	*outval = 0;

	// First character
	// - maxlen == 0 means no limit
	ich = __getc(h);
	n_read ++;
	
	// Sign?
	if( ich == '-' || ich == '+' ) {
		sgn = (ich == '-' ? -1 : 1);
		if( n_read == maxlen )
			return n_read;
		ich = __getc(h);
		n_read ++;
	}
	
	// Determine base
	if( base == 0 ) {
		if( ich != '0' ) {
			base = 10;
		}
		else {
			if( n_read == maxlen )
				return n_read;
			ich = __getc(h);
			n_read ++;
			if( ich != 'x' ) {
				base = 8;
			}
			else {
				base = 16;
				if( n_read == maxlen )
					return n_read;
				ich = __getc(h);
				n_read ++;
			}
		}
	}
	
	if( ich == 0 ) {
		// Oops?
		return n_read;
	}
	
	while( ich )
	{
		 int	next = -1;
		
		// Get the digit value
		if( base <= 10 ) {
			if( '0' <= ich && ich <= '0'+base-1 )
				next = ich - '0';
		}
		else {
			if( '0' <= ich && ich <= '9' )
				next = ich - '0';
			if( 'A' <= ich && ich <= 'A'+base-10-1 )
				next = ich - 'A' + 10;
			if( 'a' <= ich && ich <= 'a'+base-10-1 )
				next = ich - 'a' + 10;
		}
		// if it's not a digit, rewind and break
		if( next < 0 ) {
			__rewind(h);
			n_read --;
			break;
		}
		
		// Add to the result
		ret *= base;
		ret += next;
		//_SysDebug("- %i/%i read, 0x%x val", n_read, maxlen, ret);

		// Check if we've reached the limit
		if( n_read == maxlen )
			break ;

		// Next character
		ich = __getc(h);
		n_read ++;
	}
	
	// Apply sign
	if( sgn == -1 )
		ret = -ret;

	*outval = ret;
	return n_read;
}

int _vcscanf(int (*__getc)(void*), void (*__rewind)(void*), void *h, const char *format, va_list ap)
{
	char	fch, ich;
	 int	ret = 0, nch = 0;
	
	while( (fch = *format++) )
	{
		union {
			void	*_void;
			char	*_char;
			short	*_short;
			 int	*_int;
			long	*_long;
			long long	*_longlong;
		}	ptr;
		long long	ival;
		//long double	rval;
		 int	maxlen = 0;
		// int	offset = -1;
		enum e_vcscanf_sizes	size = _VCSCANF_UNDEF;
		enum e_vcscanf_types	valtype;
		 int	fail = 0;
		 int	nnewch;

		const char	*set_start;
		 int	set_len;		

		// Whitespace matches any ammount of whitespace (including none)
		if( isspace(fch) )
		{
			while( (ich = __getc(h)) && isspace(ich) )
				nch ++;
			if(ich)	__rewind(h);
			continue ;
		}
		
		// Any non-whitespace and non-% characters must match exactly
		if( fch != '%' )
		{
			if( __getc(h) != fch )
				break;
			nch ++;
			continue ;
		}
		
		// Format specifier
		fch = *format++;
		if(!fch)	break;

		// Catch '%%' early
		if( fch == '%' ) {
			if( __getc(h) != '%' )
				break;
			nch ++;
			continue ;
		}		

		// %n$ - Direct offset selection, shouldn't be mixed with just %
		#if 0
		for( ; isdigit(fch); fch = *format++ )
			maxlen = maxlen * 10 + (fch - '0');
		if( fch == '$' ) {
			offset = maxlen;
			maxlen = 0;
		}
		#endif
		
		// Supress assignemnt?
		if( fch == '*' )
		{
			fch = *format++;
			ptr._void = NULL;
			ret --;
		}
		else
			ptr._void = va_arg(ap, void*);
		
		// Max field length
		while( isdigit(fch) )
		{
			maxlen = maxlen * 10 + fch - '0';
			fch = *format++;
		}
		
		// Length modifier
		switch( fch )
		{
		case 'h':
			// 'short'
			size = _VCSCANF_SHORT;
			fch = *format++;
			if( fch == 'h' ) {
				// 'char'
				size = _VCSCANF_CHAR;
				fch = *format++;
			}
			break;
		case 'l':
			// 'long'
			size = _VCSCANF_LONG;
			fch = *format++;
			if( fch == 'l' ) {
				// 'long long'
				size = _VCSCANF_LONGLONG;
				fch = *format++;
			}
			break;
		case 'j':
			// '(u)intmax_t'
			size = _VCSCANF_INTMAX;
			fch = *format++;
			break;
		case 'z':
			// 'size_t'
			size = _VCSCANF_SIZET;
			fch = *format++;
			break;
		case 't':
			// 'ptrdiff_t'
			size = _VCSCANF_PTRDIFF;
			fch = *format++;
			break;
		case 'L':
			// 'long double' (a, A, e, E, f, F, g, G)
			size = _VCSCANF_LONGDOUBLE;
			fch = *format++;
			break;
		}
		
		// Format specifiers
		switch( fch )
		{
		// Decimal integer
		case 'd':
			nnewch = _vcscanf_int(__getc, __rewind, h, 10, maxlen, &ival);
			if(nnewch==0)	fail=1;
			nch += nnewch;
			valtype = _VCSCANF_INT;
			break;
		// variable-base integer
		case 'i':
			nnewch = _vcscanf_int(__getc, __rewind, h, 0, maxlen, &ival);
			if(nnewch==0)	fail=1;
			nch += nnewch;
			valtype = _VCSCANF_INT;
			break;
		// Octal integer
		case 'o':
			nnewch = _vcscanf_int(__getc, __rewind, h, 8, maxlen, &ival);
			if(nnewch==0)	fail=1;
			nch += nnewch;
			valtype = _VCSCANF_INT;
			break;
		// Hexadecimal integer
		case 'x': case 'X':
			nnewch = _vcscanf_int(__getc, __rewind, h, 16, maxlen, &ival);
			if(nnewch==0)	fail=1;
			nch += nnewch;
			valtype = _VCSCANF_INT;
			break;
		// strtod format float
		case 'a': case 'A':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
			valtype = _VCSCANF_REAL;
			break;
 		// `maxlen` or 1 characters
		case 'c':
			if( maxlen == 0 )
				maxlen = 1;
			while( maxlen -- && (ich = __getc(h)) )
			{
				if(ptr._char)	*ptr._char++ = ich;
				nch ++;
			}
			valtype = _VCSCANF_NOTYPE;
			break;
		// sequence of non-whitespace characters
		case 's':
			if( maxlen == 0 )
				maxlen = -1;

			ich = 0;
			while( maxlen -- && (ich = __getc(h)) && !isspace(ich) )
			{
				if(ptr._char)	*ptr._char++ = ich;
				nch ++;
			}
			if( maxlen >= 0 && ich )
				__rewind(h);
			if(ptr._char)	*ptr._char++ = 0;
			valtype = _VCSCANF_NOTYPE;
			break;
		// match a set of characters
		case '[': {
			int invert = 0;
			if( *format++ == '^' ) {
				// Invert
				invert = 1;
			}
			set_start = format;
			set_len = 0;
			// if the first character is ']' it's part of the set
			do {
				// permissable character
				set_len ++;
				fch = *format++;
			} while( fch && fch != ']' );

			if( maxlen == 0 )
				maxlen = -1;
			ich = 0;
			while( maxlen -- && (ich = __getc(h)) && invert == !memchr(set_start, set_len, ich) )
			{
				if(ptr._char)	*ptr._char++ = ich;
				nch ++;
			}
			if( maxlen >= 0 && ich )
				__rewind(h);
			if(ptr._char)	*ptr._char++ = 0;
			valtype = _VCSCANF_NOTYPE;
			break;
			}
		case 'p': // read back printf("%p")
			valtype = _VCSCANF_NOTYPE;
			break;
		case 'n': // number of read characters to this point
			if(ptr._int) {
				*ptr._int = nch;
				ret --;	// negates the ret ++ at the end
			}
			valtype = _VCSCANF_NOTYPE;
			break;
		default:
			// Implimentation defined
			valtype = _VCSCANF_NOTYPE;
			break;
		}

		if(fail)
			break;		

		switch(valtype)
		{
		case _VCSCANF_NOTYPE:
			// Used when assignment is done above
			break;
		case _VCSCANF_INT:
			switch(size)
			{
			case _VCSCANF_UNDEF:	*ptr._int = ival;	break;
			case _VCSCANF_CHAR:	*ptr._char = ival;	break;
			case _VCSCANF_SHORT:	*ptr._short = ival;	break;
			case _VCSCANF_LONG:	*ptr._long = ival;	break;
			case _VCSCANF_LONGLONG:	*ptr._longlong = ival;	break;
			default:	// ???
				break;
			}
			break;
		case _VCSCANF_REAL:
			break;
		}
		
		ret ++;
	}
	
	return ret;
}

int _vsscanf_getc(void *h)
{
	const char	**ibuf = h;
	return *(*ibuf)++;	// Dereference, read for return, increment
}

void _vsscanf_rewind(void *h)
{
	const char	**ibuf = h;
	(*ibuf) --;	// NOTE: Breaks if there's a rewind before a getc, but that shouldn't happen
}

int vscanf(const char *format, va_list ap)
{
	return vfscanf(stdin, format, ap);
}

int vsscanf(const char *str, const char *format, va_list ap)
{
	return _vcscanf(_vsscanf_getc, _vsscanf_rewind, &str, format, ap);
}

int _vfscanf_getc(void *h)
{
	int ch = fgetc(h);
	return ch == -1 ? 0 : ch;
}
void _vfscanf_rewind(void *h)
{
	fseek(h, -1, SEEK_CUR);
}

int vfscanf(FILE *stream, const char *format, va_list ap)
{
	return _vcscanf(_vfscanf_getc, _vfscanf_rewind, stream, format, ap);
}

int scanf(const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vfscanf(stdin, format, args);
	va_end(args);
	return rv;
}
int fscanf(FILE *stream, const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vfscanf(stream, format, args);
	va_end(args);
	return rv;
}
int sscanf(const char *str, const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vsscanf(str, format, args);
	va_end(args);
	return rv;
}


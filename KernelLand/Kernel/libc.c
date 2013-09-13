/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * libc.c
 * - Kernel-land C Library
 */
#include <acess.h>
#include <hal_proc.h>	// For MM_*
#include <utf16.h>

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55052
#define	RANDOM_A	0x00731ADE
#define	RANDOM_C	12345
#define	RANDOM_SPRUCE	0xf12b039

// === PROTOTYPES ===
#if 0
unsigned long long	strtoull(const char *str, char **end, int base);
unsigned long	strtoul(const char *str, char **end, int base);
signed long long	strtoll(const char *str, char **end, int base);
signed long	strtol(const char *str, char **end, int base);
 int	atoi(const char *string);
 int	ParseInt(const char *string, int *Val);
void	itoa(char *buf, Uint64 num, int base, int minLength, char pad);
 int	vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args);
 int	snprintf(char *__s, size_t __n, const char *__format, ...);
 int	sprintf(char *__s, const char *__format, ...);
 int	strucmp(const char *Str1, const char *Str2);
char	*strchr(const char *__s, int __c);
 int	strpos(const char *Str, char Ch);
size_t	strlen(const char *__s);
char	*strcpy(char *__str1, const char *__str2);
char	*strncpy(char *__str1, const char *__str2, size_t max);
char	*strcat(char *dest, const char *source);
 int	strcmp(const char *str1, const char *str2);
 int	strncmp(const char *str1, const char *str2, size_t num);
char	*_strdup(const char *File, int Line, const char *Str);
 int	rand(void);
void	*memmove(void *__dest, const void *__src, size_t len);

 int	CheckString(char *String);
 int	CheckMem(void *Mem, int NumBytes); 
#endif

// === EXPORTS ===
EXPORT(atoi);
EXPORT(itoa);
EXPORT(vsnprintf);
EXPORT(snprintf);
EXPORT(sprintf);
EXPORT(tolower);

EXPORT(strucmp);
EXPORT(strchr);
EXPORT(strrchr);
EXPORT(strpos);
EXPORT(strlen);
EXPORT(strcpy);
EXPORT(strncpy);
EXPORT(strcat);
EXPORT(strncat);
EXPORT(strcmp);
EXPORT(strncmp);
//EXPORT(strdup);
EXPORT(_strdup);	// Takes File/Line too
EXPORT(rand);
EXPORT(memmove);

EXPORT(CheckString);
EXPORT(CheckMem);

// === CODE ===
// - Import userland stroi.c file
#define _LIB_H_
#include "../../Usermode/Libraries/libc.so_src/strtoi.c"

int ParseInt(const char *string, int *Val)
{
	 int	ret = 0;
	 int	bNeg = 0;
	const char *orig_string = string;
	
	//Log("atoi: (string='%s')", string);
	
	// Clear non-numeric characters
	while( !('0' <= *string && *string <= '9') && *string != '-' )	string++;
	if( *string == '-' ) {
		bNeg = 1;
		while( !('0' <= *string && *string <= '9') )	string++;
	}
	
	if(*string == '0')
	{
		string ++;
		if(*string == 'x')
		{
			// Hex
			string ++;
			for( ;; string ++ )
			{
				if('0' <= *string && *string <= '9') {
					ret *= 16;
					ret += *string - '0';
				}
				else if('A' <= *string && *string <= 'F') {
					ret *= 16;
					ret += *string - 'A' + 10;
				}
				else if('a' <= *string && *string <= 'f') {
					ret *= 16;
					ret += *string - 'a' + 10;
				}
				else
					break;
			}
		}
		else	// Octal
		{
			for( ; '0' <= *string && *string <= '7'; string ++ )
			{
				ret *= 8;
				ret += *string - '0';
			}
		}
	}
	else	// Decimal
	{
		for( ; '0' <= *string && *string <= '9'; string++)
		{
			ret *= 10;
			ret += *string - '0';
		}
		// Error check
		if( ret == 0 )	return 0;
	}
	
	if(bNeg)	ret = -ret;
	
	//Log("atoi: RETURN %i", ret);
	
	if(Val)	*Val = ret;
	
	return string - orig_string;
}

static const char cUCDIGITS[] = "0123456789ABCDEF";
/**
 * \fn void itoa(char *buf, Uint64 num, int base, int minLength, char pad)
 * \brief Convert an integer into a character string
 */
void itoa(char *buf, Uint64 num, int base, int minLength, char pad)
{
	char	tmpBuf[64+1];
	 int	pos=0, i;
	Uint64	rem;

	buf[0] = 0;
	ASSERTR(base >= 2, );
	ASSERTR(base <= 16, );

	// Sanity check
	if(!buf)	return;
	
	// Convert 
	while(num > base-1) {
		num = DivMod64U(num, base, &rem);	// Shift `num` and get remainder
		ASSERT(rem >= 0);
		if( rem >= base && base != 16 ) {
			Debug("rem(%llx) >= base(%x), num=%llx", rem, base, num);
		}
		ASSERT(rem < base);
		tmpBuf[pos] = cUCDIGITS[ rem ];
		pos++;
	}
	tmpBuf[pos++] = cUCDIGITS[ num ];		// Last digit of `num`
	
	// Put in reverse
	i = 0;
	minLength -= pos;
	while(minLength-- > 0)	buf[i++] = pad;
	while(pos-- > 0)		buf[i++] = tmpBuf[pos];	// Reverse the order of characters
	buf[i] = 0;
}

/**
 * \brief Append a character the the vsnprintf output
 */
#define PUTCH(ch)	do { \
		if(pos < __maxlen && __s) { \
			__s[pos] = ch; \
		} else { \
			(void)ch;\
		} \
		pos ++; \
	} while(0)
#define GETVAL()	do {\
	if(isLongLong)	val = va_arg(args, Uint64);\
	else	val = va_arg(args, unsigned int);\
	}while(0)
/**
 * \brief VArg String Number Print Formatted
 */
int vsnprintf(char *__s, const size_t __maxlen, const char *__format, va_list args)
{
	char	c, pad = ' ';
	 int	minSize = 0, precision = -1, len;
	char	tmpBuf[34];	// For Integers
	const char	*p = NULL;
	 int	isLongLong = 0, isLong;
	Uint64	val;
	size_t	pos = 0;
	// Flags
	 int	bPadLeft = 0;

	while((c = *__format++) != 0)
	{
		// Non control character
		if(c != '%') { PUTCH(c); continue; }

		c = *__format++;
		if(c == '\0')	break;
		
		// Literal %
		if(c == '%') { PUTCH('%'); continue; }
		
		// Pointer - Done first for debugging
		if(c == 'p') {
			Uint	ptr = va_arg(args, Uint);
			PUTCH('*');	PUTCH('0');	PUTCH('x');
			for( len = BITS/4; len -- && ((ptr>>(len*4))&15) == 0; )
				;
			len ++;
			if( len == 0 )
				PUTCH( '0' );
			else
				while( len -- )
					PUTCH( cUCDIGITS[ (ptr>>(len*4))&15 ] );
			continue ;
		}

		isLongLong = 0;
		isLong = 0;
	
		// - Padding Side Flag
		if(c == '-') {
			bPadLeft = 1;
			c = *__format++;
		}
		
		// - Padding
		if(c == '0') {
			pad = '0';
			c = *__format++;
		}
		else
			pad = ' ';
		
		// - Minimum length
		if(c == '*') {	// Dynamic length
			minSize = va_arg(args, unsigned int);
			c = *__format++;
		}
		else if('1' <= c && c <= '9')
		{
			minSize = 0;
			while('0' <= c && c <= '9')
			{
				minSize *= 10;
				minSize += c - '0';
				c = *__format++;
			}
		}
		else
			minSize = 0;
		
		// - Precision
		precision = -1;
		if( c == '.' ) {
			c = *__format++;
			
			if(c == '*') {	// Dynamic length
				precision = va_arg(args, unsigned int);
				c = *__format++;
			}
			else if('1' <= c && c <= '9')
			{
				precision = 0;
				while('0' <= c && c <= '9')
				{
					precision *= 10;
					precision += c - '0';
					c = *__format++;
				}
			}
		}
		
		// - Default, Long or LongLong?
		isLongLong = 0;
		if(c == 'l')	// Long is actually the default on x86
		{
			isLong = 1;
			c = *__format++;
			if(c == 'l') {
				c = *__format++;
				isLongLong = 1;
			}
		}
		
		// - Now get the format code
		p = tmpBuf;
		switch(c)
		{
		case 'd':
		case 'i':
			GETVAL();
			if( isLongLong && val >> 63 ) {
				PUTCH('-');
				if( val == LLONG_MIN )
					val = LLONG_MAX;
				else
					val = -val;
			}
			else if( !isLongLong && (val >> 31) ) {
				PUTCH('-');
				val = (~val & 0xFFFFFFFF)+1;
			}
			itoa(tmpBuf, val, 10, minSize, pad);
			goto printString;
		case 'u':	// Unsigned
			GETVAL();
			itoa(tmpBuf, val, 10, minSize, pad);
			goto printString;
		case 'P':	// Physical Address
			PUTCH('0');
			PUTCH('x');
			if(sizeof(tPAddr) > 4)	isLongLong = 1;
			GETVAL();
			itoa(tmpBuf, val, 16, minSize, pad);
			goto printString;
		case 'X':	// Hex
			if(BITS == 64)
				isLongLong = 1;	// TODO: Handle non-x86 64-bit archs
			GETVAL();
			itoa(tmpBuf, val, 16, minSize, pad);
			goto printString;
			
		case 'x':	// Lower case hex
			GETVAL();
			itoa(tmpBuf, val, 16, minSize, pad);
			goto printString;
		case 'o':	// Octal
			GETVAL();
			itoa(tmpBuf, val, 8, minSize, pad);
			goto printString;
		case 'b':
			GETVAL();
			itoa(tmpBuf, val, 2, minSize, pad);
			goto printString;

		case 'B':	//Boolean
			val = va_arg(args, unsigned int);
			if(val)	p = "True";
			else	p = "False";
			goto printString;
		
		// String - Null Terminated Array
		case 's':
			if( isLong ) {
				Uint16	*p16 = va_arg(args, Uint16*);
				Uint8	tmp[5];
				while( *p16 && precision-- ) {
					Uint32	cp;
					p16 += ReadUTF16(p16, &cp);
					tmp[WriteUTF8(tmp, cp)] = 0;
					for(int i = 0; tmp[i] && i<5; i ++)
						PUTCH(tmp[i]);
				}
				break;
			}
			p = va_arg(args, char*);	// Get Argument
			if( !p || !CheckString(p) )	p = "(inval)";	// Avoid #PFs  
		printString:
			if(!p)		p = "(null)";
			len = strlen(p);
			if( !bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			while(*p && precision--) { PUTCH(*p); p++;} 
			if( bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			break;
		
		case 'C':	// Non-Null Terminated Character Array
			p = va_arg(args, char*);
			if( !CheckMem(p, minSize) )	continue;	// No #PFs please
			if(!p)	goto printString;
			while(minSize--) {
				PUTCH(*p);
				p ++;
			}
			break;
		
		// Single Character
		case 'c':
		default:
			GETVAL();
			PUTCH( (Uint8)val );
			break;
		}
	}
	
	if(__s && pos < __maxlen)
		__s[pos] = '\0';
	
	return pos;
}
#undef PUTCH

/**
 */
int snprintf(char *__s, size_t __n, const char *__format, ...)
{
	va_list	args;
	 int	ret;
	
	va_start(args, __format);
	ret = vsnprintf(__s, __n, __format, args);
	va_end(args);
	
	return ret;
}

/**
 */
int sprintf(char *__s, const char *__format, ...)
{
	va_list	args;
	 int	ret;
	
	va_start(args, __format);
	ret = vsnprintf(__s, -1, __format, args);
	va_end(args);
	
	return ret;
}

/*
 * ==================
 * ctype.h
 * ==================
 */
int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}
int isalpha(int c)
{
	return isupper(c) || islower(c);
}
int isascii(int c)
{
	return (0 <= c && c < 128);
}
int isblank(int c)
{
	if(c == '\t')	return 1;
	if(c == ' ')	return 1;
	return 0;
}
int iscntrl(int c)
{
	// TODO: Check iscntrl
	if(c < ' ')	return 1;
	return 0;
}
int isdigit(int c)
{
	return ('0' <= c && c <= '9');
}
int isgraph(int c)
{
	// TODO: Check isgraph
	return 0;
}
int islower(int c)
{
	return ('a' <= c && c <= 'z');
}
int isprint(int c)
{
	if( ' ' <= c && c <= 0x7F )	return 1;
	return 0;
}
int ispunct(int c)
{
	switch(c)
	{
	case '.':	case ',':
	case '?':	case '!':
		return 1;
	default:
		return 0;
	}
}
int isspace(int c)
{
	if(c == ' ')	return 1;
	if(c == '\t')	return 1;
	if(c == '\v')	return 1;
	if(c == '\n')	return 1;
	if(c == '\r')	return 1;
	return 0;
}
int isupper(int c)
{
	return ('A' <= c && c <= 'Z');
}
int isxdigit(int c)
{
	return isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

int toupper(int c)
{
	if( islower(c) )
		return c - 0x20;
	else
		return c;
}
int tolower(int c)
{
	if( isupper(c) )
		return c + 0x20;
	else
		return c;
}

/**
 * \fn int strucmp(const char *Str1, const char *Str2)
 * \brief Compare \a Str1 and \a Str2 case-insensitively
 */
int strucmp(const char *Str1, const char *Str2)
{
	while(*Str1 && tolower(*Str1) == tolower(*Str2))
		Str1++, Str2++;
	return tolower(*Str1) - tolower(*Str2);
}

/**
 * \brief Locate a byte in a string
 */
char *strchr(const char *__s, int __c)
{
	for( ; *__s; __s ++ )
	{
		if( *__s == __c )	return (char*)__s;
	}
	return NULL;
}

char *strrchr(const char *__s, int __c)
{
	size_t ofs = strlen(__s);
	while(--ofs && __s[ofs] != __c);
	if( __s[ofs] == __c )
		return (char*)__s + ofs;
	return NULL;
}

/**
 * \fn int strpos(const char *Str, char Ch)
 * \brief Search a string for an ascii character
 */
int strpos(const char *Str, char Ch)
{
	 int	pos;
	for(pos=0;Str[pos];pos++)
	{
		if(Str[pos] == Ch)	return pos;
	}
	return -1;
}

/**
 * \fn size_t strlen(const char *__str)
 * \brief Get the length of string
 */
size_t strlen(const char *__str)
{
	size_t	ret = 0;
	while(*__str++)	ret++;
	return ret;
}

/**
 * \brief Copy a string to a new location
 */
char *strcpy(char *__str1, const char *__str2)
{
	while(*__str2)
		*__str1++ = *__str2++;
	*__str1 = '\0';	// Terminate String
	return __str1;
}

/**
 * \brief Copy a string to a new location
 * \note Copies at most `max` chars
 */
char *strncpy(char *__str1, const char *__str2, size_t __max)
{
	while(*__str2 && __max-- >= 1)
		*__str1++ = *__str2++;
	if(__max)
		*__str1 = '\0';	// Terminate String
	return __str1;
}

/**
 * \brief Append a string to another
 */
char *strcat(char *__dest, const char *__src)
{
	while(*__dest++);
	__dest--;
	while(*__src)
		*__dest++ = *__src++;
	*__dest = '\0';
	return __dest;
}

/**
 * \brief Append at most \a n chars to a string from another
 * \note At most n+1 chars are written (the dest is always zero terminated)
 */
char *strncat(char *__dest, const char *__src, size_t n)
{
	while(*__dest++);
	while(*__src && n-- >= 1)
		*__dest++ = *__src++;
	*__dest = '\0';
	return __dest;
}

/**
 * \fn int strcmp(const char *str1, const char *str2)
 * \brief Compare two strings return the difference between
 *        the first non-matching characters.
 */
int strcmp(const char *str1, const char *str2)
{
	while(*str1 && *str1 == *str2)
		str1++, str2++;
	return *str1 - *str2;
}

/**
 * \fn int strncmp(const char *Str1, const char *Str2, size_t num)
 * \brief Compare strings \a Str1 and \a Str2 to a maximum of \a num characters
 */
int strncmp(const char *Str1, const char *Str2, size_t num)
{
	if(num == 0)	return 0;	// TODO: Check what should officially happen here
	while(--num && *Str1 && *Str1 == *Str2)
		Str1++, Str2++;
	return *Str1-*Str2;
}

#if 0
/**
 * \fn char *strdup(const char *Str)
 * \brief Duplicates a string
 */
char *strdup(const char *Str)
{
	char	*ret;
	ret = malloc(strlen(Str)+1);
	if( !ret )	return NULL;
	strcpy(ret, Str);
	return ret;
}
#else

/**
 * \fn char *_strdup(const char *File, int Line, const char *Str)
 * \brief Duplicates a string
 */
char *_strdup(const char *File, int Line, const char *Str)
{
	char	*ret;
	ret = Heap_Allocate(File, Line, strlen(Str)+1);
	if( !ret )	return NULL;
	strcpy(ret, Str);
	return ret;
}
#endif

/**
 * \fn int rand()
 * \brief Pseudo random number generator
 */
int rand(void)
{
	#if 0
	static Uint	state = RANDOM_SEED;
	Uint	old = state;
	// Get the next state value
	giRandomState = (RANDOM_A*state + RANDOM_C);
	// Check if it has changed, and if it hasn't, change it
	if(state == old)	state += RANDOM_SPRUCE;
	return state;
	#else
	// http://en.wikipedia.org/wiki/Xorshift
	// 2010-10-03
	static Uint32	x = 123456789;
	static Uint32	y = 362436069;
	static Uint32	z = 521288629;
	static Uint32	w = 88675123; 
	Uint32	t;
 
	t = x ^ (x << 11);
	x = y; y = z; z = w;
	return w = w ^ (w >> 19) ^ t ^ (t >> 8); 
	#endif
}

void *memmove(void *__dest, const void *__src, size_t len)
{
	char	*dest = __dest;
	const char	*src = __src;
	void	*ret = __dest;

	if( len == 0 || dest == src )
		return dest;
	
	if( (tVAddr)dest > (tVAddr)src + len )
		return memcpy(dest, src, len);
	if( (tVAddr)dest + len < (tVAddr)src )
		return memcpy(dest, src, len);
	
	// NOTE: Assumes memcpy works forward
	if( (tVAddr)dest < (tVAddr)src )
		return memcpy(dest, src, len);

	#if 0
	size_t	block_size;
	if( (tVAddr)dest < (tVAddr)src )
		block_size = (tVAddr)src - (tVAddr)dest;
	else
		block_size = (tVAddr)dest - (tVAddr)src;
	
	block_size &= ~0xF;
	
	while(len >= block_size)
	{
		memcpy(dest, src, block_size);
		len -= block_size;
		dest += block_size;
		src += block_size;
	}
	memcpy(dest, src, len);
	return ret;
	#else
	for( int i = len; i--; )
	{
		dest[i] = src[i];
	}
	return ret;
	#endif
	
}

// NOTE: Strictly not libc, but lib.c is used by userland code too and hence these two
// can't be in it.
/**
 * \name Memory Validation
 * \{
 */
/**
 * \brief Checks if a string resides fully in valid memory
 */
int CheckString(const char *String)
{
	tVAddr	addr;
	 int	bUser;

	addr = (tVAddr)String;

	if( !MM_GetPhysAddr( (void*)addr ) )
		return 0;
	
	// Check 1st page
	bUser = MM_IsUser( addr );
	
	while( *(char*)addr )
	{
		if( (addr & (PAGE_SIZE-1)) == 0 )
		{
			if(bUser && !MM_IsUser(addr) )
				return 0;
			if(!bUser && !MM_GetPhysAddr((void*)addr) )
				return 0;
		}
		addr ++;
	}
	return 1;
}

/**
 * \brief Check if a sized memory region is valid memory
 * \return Boolean success
 */
int CheckMem(const void *Mem, int NumBytes)
{
	return MM_IsValidBuffer( (tVAddr)Mem, NumBytes );
}
/* *
 * \}
 */


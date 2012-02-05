/*
 * Acess2
 * Common Library Functions
 */
#include <acess.h>
#include <hal_proc.h>

// === CONSTANTS ===
#define	RANDOM_SEED	0xACE55052
#define	RANDOM_A	0x00731ADE
#define	RANDOM_C	12345
#define	RANDOM_SPRUCE	0xf12b039
//                          Jan Feb Mar Apr May  Jun  Jul  Aug  Sept Oct  Nov  Dec
const short DAYS_BEFORE[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years

// === PROTOTYPES ===
#if 0
 int	atoi(const char *string);
void	itoa(char *buf, Uint64 num, int base, int minLength, char pad);
 int	vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args);
 int	sprintf(char *__s, const char *__format, ...);
#endif
 int	tolower(int c);
#if 0
 int	strucmp(const char *Str1, const char *Str2);
char	*strchr(const char *__s, int __c);
 int	strpos(const char *Str, char Ch);
 Uint8	ByteSum(void *Ptr, int Size);
size_t	strlen(const char *__s);
char	*strcpy(char *__str1, const char *__str2);
char	*strncpy(char *__str1, const char *__str2, size_t max);
 int	strcmp(const char *str1, const char *str2);
 int	strncmp(const char *str1, const char *str2, size_t num);
char	*_strdup(const char *File, int Line, const char *Str);
char	**str_split(const char *__str, char __ch);
 int	strpos8(const char *str, Uint32 Search);
 int	ReadUTF8(Uint8 *str, Uint32 *Val);
 int	WriteUTF8(Uint8 *str, Uint32 Val);
 int	DivUp(int num, int dem);
Sint64	timestamp(int sec, int mins, int hrs, int day, int month, int year);
#endif
void	format_date(tTime TS, int *year, int *month, int *day, int *hrs, int *mins, int *sec, int *ms);
#if 0
 int	rand(void);
 
 int	CheckString(char *String);
 int	CheckMem(void *Mem, int NumBytes);
 
 int	ModUtil_LookupString(char **Array, char *Needle);
 int	ModUtil_SetIdent(char *Dest, char *Value);
 
 int	Hex(char *Dest, size_t Size, const Uint8 *SourceData);
 int	UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString);
#endif

// === EXPORTS ===
EXPORT(atoi);
EXPORT(itoa);
EXPORT(vsnprintf);
EXPORT(sprintf);
EXPORT(tolower);
EXPORT(strucmp);
EXPORT(strchr);
EXPORT(strpos);
EXPORT(ByteSum);
EXPORT(strlen);
EXPORT(strcpy);
EXPORT(strncpy);
EXPORT(strcat);
EXPORT(strncat);
EXPORT(strcmp);
EXPORT(strncmp);
//EXPORT(strdup);
EXPORT(_strdup);	// Takes File/Line too
EXPORT(str_split);
EXPORT(strpos8);
EXPORT(DivUp);
EXPORT(ReadUTF8);
EXPORT(WriteUTF8);
EXPORT(timestamp);
EXPORT(CheckString);
EXPORT(CheckMem);
EXPORT(ModUtil_LookupString);
EXPORT(ModUtil_SetIdent);
EXPORT(UnHex);
EXPORT(SwapEndian16);
EXPORT(SwapEndian32);
EXPORT(memmove);

// === CODE ===
/**
 * \brief Convert a string into an integer
 */
int atoi(const char *string)
{
	int ret = 0;
	ParseInt(string, &ret);
	return ret;
}
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

	// Sanity check
	if(!buf)	return;
	
	// Sanity Check
	if(base > 16 || base < 2) {
		buf[0] = 0;
		return;
	}
	
	// Convert 
	while(num > base-1) {
		num = DivMod64U(num, base, &rem);	// Shift `num` and get remainder
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
#define PUTCH(c)	_putch(c)
#define GETVAL()	do {\
	if(isLongLong)	val = va_arg(args, Uint64);\
	else	val = va_arg(args, unsigned int);\
	}while(0)
/**
 * \brief VArg String Number Print Formatted
 */
int vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args)
{
	char	c, pad = ' ';
	 int	minSize = 0, precision = -1, len;
	char	tmpBuf[34];	// For Integers
	const char	*p = NULL;
	 int	isLongLong = 0;
	Uint64	val;
	size_t	pos = 0;
	// Flags
	 int	bPadLeft = 0;
	
	auto void _putch(char ch);

	void _putch(char ch)
	{
		if(pos < __maxlen)
		{
			if(__s) __s[pos] = ch;
			pos ++;
		}
	}

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
			for( len = BITS/4; len --; )
				PUTCH( cUCDIGITS[ (ptr>>(len*4))&15 ] );
			continue ;
		}
		
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
				val = -val;
			}
			else if( !isLongLong && val >> 31 ) {
				PUTCH('-');
				val = -(Sint32)val;
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
			p = va_arg(args, char*);	// Get Argument
			if( !p || !CheckString(p) )	p = "(inval)";	// Avoid #PFs  
		printString:
			if(!p)		p = "(null)";
			len = strlen(p);
			if( !bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			while(*p && precision--)	PUTCH(*p++);
			if( bPadLeft )	while(len++ < minSize)	PUTCH(pad);
			break;
		
		case 'C':	// Non-Null Terminated Character Array
			p = va_arg(args, char*);
			if( !CheckMem(p, minSize) )	continue;	// No #PFs please
			if(!p)	goto printString;
			while(minSize--)	PUTCH(*p++);
			break;
		
		// Single Character
		case 'c':
		default:
			GETVAL();
			PUTCH( (Uint8)val );
			break;
		}
	}
	
	if(__s && pos != __maxlen)
		__s[pos] = '\0';
	
	return pos;
}
#undef PUTCH

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

/**
 * \fn int tolower(int c)
 * \brief Converts a character to lower case
 */
int tolower(int c)
{
	if('A' <= c && c <= 'Z')
		return c - 'A' + 'a';
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
 * \fn Uint8 ByteSum(void *Ptr, int Size)
 * \brief Adds the bytes in a memory region and returns the sum
 */
Uint8 ByteSum(const void *Ptr, int Size)
{
	Uint8	sum = 0;
	const Uint8	*data = Ptr;
	while(Size--)	sum += *(data++);
	return sum;
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
 * \brief Split a string using the passed character
 * \return NULL terminated array of strings on the heap
 * \param __str	String to split
 * \param __ch	Character to split by
 */
char **str_split(const char *__str, char __ch)
{
	 int	i, j;
	 int	len = 1;
	char	**ret;
	char	*start;
	
	for( i = 0; __str[i]; i++ )
	{
		if(__str[i] == __ch)
			len ++;
	}
	
	ret = malloc( sizeof(char*)*(len+1) + (i + 1) );
	if( !ret )	return NULL;
	
	j = 1;
	start = (char *)&ret[len+1];
	ret[0] = start;
	for( i = 0; __str[i]; i++ )
	{
		if(__str[i] == __ch) {
			*start++ = '\0';
			ret[j++] = start;
		}
		else {
			*start++ = __str[i]; 
		}
	}
	*start = '\0';
	ret[j] = NULL;
	
	return ret;
}

/**
 * \fn int DivUp(int num, int dem)
 * \brief Divide two numbers, rounding up
 * \param num	Numerator
 * \param dem	Denominator
 */
int DivUp(int num, int dem)
{
	return (num+dem-1)/dem;
}

/**
 * \fn int strpos8(const char *str, Uint32 search)
 * \brief Search a string for a UTF-8 character
 */
int strpos8(const char *str, Uint32 Search)
{
	 int	pos;
	Uint32	val = 0;
	for(pos=0;str[pos];pos++)
	{
		// ASCII Range
		if(Search < 128) {
			if(str[pos] == Search)	return pos;
			continue;
		}
		if(*(Uint8*)(str+pos) < 128)	continue;
		
		pos += ReadUTF8( (Uint8*)&str[pos], &val );
		if(val == Search)	return pos;
	}
	return -1;
}

/**
 * \fn int ReadUTF8(Uint8 *str, Uint32 *Val)
 * \brief Read a UTF-8 character from a string
 */
int ReadUTF8(const Uint8 *str, Uint32 *Val)
{
	*Val = 0xFFFD;	// Assume invalid character
	
	// ASCII
	if( !(*str & 0x80) ) {
		*Val = *str;
		return 1;
	}
	
	// Middle of a sequence
	if( (*str & 0xC0) == 0x80 ) {
		return 1;
	}
	
	// Two Byte
	if( (*str & 0xE0) == 0xC0 ) {
		*Val = (*str & 0x1F) << 6;	// Upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 2;
	}
	
	// Three Byte
	if( (*str & 0xF0) == 0xE0 ) {
		*Val = (*str & 0x0F) << 12;	// Upper 4 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 3;
	}
	
	// Four Byte
	if( (*str & 0xF1) == 0xF0 ) {
		*Val = (*str & 0x07) << 18;	// Upper 3 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 12;	// Middle-upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle-lower 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 4;
}

/**
 * \fn int WriteUTF8(Uint8 *str, Uint32 Val)
 * \brief Write a UTF-8 character sequence to a string
 */
int WriteUTF8(Uint8 *str, Uint32 Val)
{
	// ASCII
	if( Val < 128 ) {
		if( str ) {
			*str = Val;
		}
		return 1;
	}
	
	// Two Byte
	if( Val < 0x8000 ) {
		if( str ) {
			*str++ = 0xC0 | (Val >> 6);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 2;
	}
	
	// Three Byte
	if( Val < 0x10000 ) {
		if( str ) {
			*str++ = 0xE0 | (Val >> 12);
			*str++ = 0x80 | ((Val >> 6) & 0x3F);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 3;
	}
	
	// Four Byte
	if( Val < 0x110000 ) {
		if( str ) {
			*str++ = 0xF0 | (Val >> 18);
			*str++ = 0x80 | ((Val >> 12) & 0x3F);
			*str++ = 0x80 | ((Val >> 6) & 0x3F);
			*str++ = 0x80 | (Val & 0x3F);
		}
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 0;
}

/**
 * \fn Uint64 timestamp(int sec, int mins, int hrs, int day, int month, int year)
 * \brief Converts a date into an Acess Timestamp
 */
Sint64 timestamp(int sec, int min, int hrs, int day, int month, int year)
{
	 int	is_leap;
	Sint64	stamp;

	if( !(0 <= sec && sec < 60) )	return 0;
	if( !(0 <= min && min < 60) )	return 0;
	if( !(0 <= hrs && hrs < 24) )	return 0;
	if( !(0 <= day && day < 31) )	return 0;
	if( !(0 <= month && month < 12) )	return 0;

	stamp = DAYS_BEFORE[month] + day;

	// Every 4 years
	// - every 100 years
	// + every 400 years
	is_leap = (year % 4 == 0) - (year % 100 == 0) + (year % 400 == 0);
	ASSERT(is_leap == 0 || is_leap == 1);

	if( is_leap && month > 1 )	// Leap year and after feb
		stamp += 1;

	// Get seconds before the first of specified year
	year -= 2000;	// Base off Y2K
	// base year days + total leap year days
	stamp += year*365 + (year/400) - (year/100) + (year/4);
	
	stamp *= 3600*24;
	stamp += UNIX_TO_2K;
	stamp += sec;
	stamp += min*60;
	stamp += hrs*3600;
	
	return stamp * 1000;
}

void format_date(tTime TS, int *year, int *month, int *day, int *hrs, int *mins, int *sec, int *ms)
{
	 int	is_leap = 0, i;

	auto Sint64 DivMod64(Sint64 N, Sint64 D, Sint64 *R);
	
	Sint64 DivMod64(Sint64 N, Sint64 D, Sint64 *R)
	{
		int sign = (N < 0) != (D < 0);
		if(N < 0)	N = -N;
		if(D < 0)	D = -D;
		if(sign)
			return -DivMod64U(N, D, (Uint64*)R);
		else
			return DivMod64U(N, D, (Uint64*)R);
	}

	// Get time
	// TODO: Leap-seconds?
	{
		Sint64	rem;
		TS = DivMod64( TS, 1000, &rem );
		*ms = rem;
		TS = DivMod64( TS, 60, &rem );
		*sec = rem;
		TS = DivMod64( TS, 60, &rem );
		*mins = rem;
		TS = DivMod64( TS, 24, &rem );
		*hrs = rem;
	}

	// Adjust to Y2K
	TS -= UNIX_TO_2K/(3600*24);

	// Year (400 yr blocks) - (400/4-3) leap years
	*year = 400 * DivMod64( TS, 365*400 + (400/4-3), &TS );
	if( TS < 366 )	// First year in 400 is a leap
		is_leap = 1;
	else
	{
		// 100 yr blocks - 100/4-1 leap years
		*year += 100 * DivMod64( TS, 365*100 + (100/4-1), &TS );
		if( TS < 366 )	// First year in 100 isn't a leap
			is_leap = 0;
		else
		{
			*year += 4 * DivMod64( TS, 365*4 + 1, &TS );
			if( TS < 366 )	// First year in 4 is a leap
				is_leap = 1;
			else
			{
				*year += DivMod64( TS, 356, &TS );
			}
		}
	}
	*year += 2000;

	ASSERT(TS >= 0);
	
	*day = 0;
	// Month (if after the first of march, which is 29 Feb in a leap year)
	if( is_leap && TS > DAYS_BEFORE[2] ) {
		TS -= 1;	// Shifts 29 Feb to 28 Feb
		*day = 1;
	}
	// Get what month it is
	for( i = 0; i < 12; i ++ ) {
		if( TS < DAYS_BEFORE[i] )
			break;
	}
	*month = i - 1;
	// Get day
	TS -= DAYS_BEFORE[i-1];
	*day += TS;	// Plus offset from leap handling above
}

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

/* *
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

	if( !MM_GetPhysAddr( addr ) )
		return 0;
	
	// Check 1st page
	bUser = MM_IsUser( addr );
	
	while( *(char*)addr )
	{
		if( (addr & (PAGE_SIZE-1)) == 0 )
		{
			if(bUser && !MM_IsUser(addr) )
				return 0;
			if(!bUser && !MM_GetPhysAddr(addr) )
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

/**
 * \brief Search a string array for \a Needle
 * \note Helper function for eTplDrv_IOCtl::DRV_IOCTL_LOOKUP
 */
int ModUtil_LookupString(const char **Array, const char *Needle)
{
	 int	i;
	if( !CheckString(Needle) )	return -1;
	for( i = 0; Array[i]; i++ )
	{
		if(strcmp(Array[i], Needle) == 0)	return i;
	}
	return -1;
}

int ModUtil_SetIdent(char *Dest, const char *Value)
{
	if( !CheckMem(Dest, 32) )	return -1;
	strncpy(Dest, Value, 32);
	return 1;
}

int Hex(char *Dest, size_t Size, const Uint8 *SourceData)
{
	 int	i;
	for( i = 0; i < Size; i ++ )
	{
		sprintf(Dest + i*2, "%02x", SourceData[i]);
	}
	return i*2;
}

/**
 * \brief Convert a string of hexadecimal digits into a byte stream
 */
int UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString)
{
	 int	i;
	
	for( i = 0; i < DestSize*2; i += 2 )
	{
		Uint8	val = 0;
		
		if(SourceString[i] == '\0')	break;
		
		if('0' <= SourceString[i] && SourceString[i] <= '9')
			val |= (SourceString[i]-'0') << 4;
		else if('A' <= SourceString[i] && SourceString[i] <= 'F')
			val |= (SourceString[i]-'A'+10) << 4;
		else if('a' <= SourceString[i] && SourceString[i] <= 'f')
			val |= (SourceString[i]-'a'+10) << 4;
			
		if(SourceString[i+1] == '\0')	break;
		
		if('0' <= SourceString[i+1] && SourceString[i+1] <= '9')
			val |= (SourceString[i+1] - '0');
		else if('A' <= SourceString[i+1] && SourceString[i+1] <= 'F')
			val |= (SourceString[i+1] - 'A' + 10);
		else if('a' <= SourceString[i+1] && SourceString[i+1] <= 'f')
			val |= (SourceString[i+1] - 'a' + 10);
		
		Dest[i/2] = val;
	}
	return i/2;
}

Uint16 SwapEndian16(Uint16 Val)
{
	return ((Val&0xFF)<<8) | ((Val>>8)&0xFF);
}
Uint32 SwapEndian32(Uint32 Val)
{
	return ((Val&0xFF)<<24) | ((Val&0xFF00)<<8) | ((Val>>8)&0xFF00) | ((Val>>24)&0xFF);
}

void *memmove(void *__dest, const void *__src, size_t len)
{
	size_t	block_size;
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
	
}

